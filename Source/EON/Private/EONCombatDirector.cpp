#include "EONCombatDirector.h"
#include "EONSfx.h"
#include "EONFx.h"
#include "EONPickup.h"
#include "EONMeta.h"
#include "EONProjectile.h"
#include "EONCharacter.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONEnemy.h"
#include "EONEnemyProjectile.h"
#include "EONFloorManager.h"
#include "EONPlayerCameraManager.h"
#include "EONStatics.h"
#include "EONTileRoom.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "DrawDebugHelpers.h"
#include "Misc/PackageName.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float TS = 16.f;
	const FColor ColCy(62, 242, 224), ColAm(255, 194, 61), ColRed(255, 61, 85), ColMg(255, 61, 139);

	template <typename V> const V& PickOne(const TArray<V>& A) { return A[FMath::Min(A.Num() - 1, FMath::FloorToInt(FMath::FRand() * A.Num()))]; }

	/** pHit(x, y, rad) du proto : Jira est une capsule debout (pieds P.y, tête P.y − 24, rayon P.r = 4). */
	bool PHit(const FVector2D& B, const FVector2D& P, float Rad)
	{
		const float Cy = FMath::Clamp(B.Y, P.Y - 24.f, P.Y - 1.f);
		return FMath::Sqrt(FMath::Square(B.X - P.X) + FMath::Square(B.Y - Cy)) < 4.f + Rad;
	}

	// ---------------------------------------------------------------- console
	AEONTileRoom* EnsureRoom(AEONCombatDirector* D)
	{
		return D ? D->GetRoom() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GEONCmdWave(TEXT("eon.wave"), TEXT("eon.wave [mul] : lance une vague (spawnWave) dans la salle de Jira"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONCombatDirector* D = AEONCombatDirector::Get(World);
			if (!EnsureRoom(D)) { UE_LOG(LogEON, Warning, TEXT("eon.wave : Jira n'est dans aucune salle en tuiles")); return; }
			if (D->GetCombatN() < 1) { D->CombatNOverride = 1; }   // combatN 0 : aucun type autorisé (min 1)
			D->SpawnWave(Args.Num() ? FCString::Atof(*Args[0]) : 1.f);
		}));
	// Outil de test des anims (25/11) : un ennemi d'un type donné, sur du sol libre à DIST px de Jira.
	FAutoConsoleCommandWithWorldAndArgs GEONCmdSpawn(TEXT("eon.spawn"), TEXT("eon.spawn <type> [nombre=1] [distance px=140] : fait apparaître des ennemis de ce type (DT_Ennemis) près de Jira"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONCombatDirector* D = AEONCombatDirector::Get(World);
			AEONTileRoom* Rm = EnsureRoom(D);
			if (!Rm || !Args.Num()) { UE_LOG(LogEON, Warning, TEXT("eon.spawn : type manquant ou Jira hors salle")); return; }
			if (D->GetCombatN() < 1) { D->CombatNOverride = 1; }
			const int32 N = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 12) : 1;
			const float Dist = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 140.f;
			const FVector2D J = D->GetJiraPx();
			int32 Fait = 0;
			for (int32 I = 0; I < 24 && Fait < N; ++I)
			{
				const float A = PI * 0.5f + I * (2.f * PI / 24.f) * 5.f;   // directions espacées
				const FVector2D C = J + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Dist;
				const FVector W = Rm->RoomPxToWorld(C, 0.f);
				FHitResult H;
				if (Rm->PitAtPx(C) || !World->LineTraceSingleByChannel(H, W + FVector(0, 0, 150.f), W - FVector(0, 0, 150.f), ECC_Visibility)) { continue; }
				if (D->SpawnEnemyAt(FName(*Args[0]), C, false)) { ++Fait; }
			}
			UE_LOG(LogEON, Display, TEXT("EON_SPAWN %s : %d / %d"), *Args[0], Fait, N);
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdCombat(TEXT("eon.combat"), TEXT("eon.combat [vagues] : transforme la salle de Jira en salle Combat verrouillée (2 vagues par défaut)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONCombatDirector* D = AEONCombatDirector::Get(World);
			if (!EnsureRoom(D)) { UE_LOG(LogEON, Warning, TEXT("eon.combat : Jira n'est dans aucune salle en tuiles")); return; }
			if (D->GetCombatN() < 1) { D->CombatNOverride = 1; }
			D->StartCombat(Args.Num() ? FMath::Max(1, FCString::Atoi(*Args[0])) : 2);
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdKill(TEXT("eon.kill"), TEXT("eon.kill : tue tous les ennemis de la salle"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AEONCombatDirector* D = AEONCombatDirector::Get(World)) { D->KillAll(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdCombatN(TEXT("eon.combatn"), TEXT("eon.combatn <n> : plancher de combatN (types et PV des ennemis) ; -1 = suivre l'étage"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AEONCombatDirector* D = AEONCombatDirector::Get(World))
			{
				D->CombatNOverride = Args.Num() ? FCString::Atoi(*Args[0]) : -1;
				UE_LOG(LogEON, Display, TEXT("EON combatN = %d"), D->GetCombatN());
			}
		}));
}

AEONCombatDirector::AEONCombatDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;   // après Jira et l'étage
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
}

AEONCombatDirector* AEONCombatDirector::Get(UWorld* World)
{
	if (!World) { return nullptr; }
	for (TActorIterator<AEONCombatDirector> It(World); It; ++It) { return *It; }
	if (!World->IsGameWorld()) { return nullptr; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AEONCombatDirector>(AEONCombatDirector::StaticClass(), FTransform::Identity, P);
}

void AEONCombatDirector::BeginPlay()
{
	Super::BeginPlay();
	BindFloor();
}

void AEONCombatDirector::EndPlay(const EEndPlayReason::Type Reason)
{
	if (AEONFloorManager* F = BoundFloor.Get()) { F->OnRoomLoaded.Remove(FloorHandle); }
	Super::EndPlay(Reason);
}

AEONCharacter* AEONCombatDirector::GetJira() const
{
	return Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

AEONFloorManager* AEONCombatDirector::GetFloor() const
{
	return BoundFloor.Get();
}

void AEONCombatDirector::BindFloor()
{
	if (BoundFloor.IsValid()) { return; }
	AEONFloorManager* F = AEONFloorManager::Get(this);
	if (!F) { return; }
	BoundFloor = F;
	FloorHandle = F->OnRoomLoaded.AddUObject(this, &AEONCombatDirector::OnFloorRoomLoaded);
	// L'étage a pu charger sa première salle avant nous.
	if (F->GetRoom() && F->GetFloorN() > 0) { OnFloorRoomLoaded(F); }
}

int32 AEONCombatDirector::GetCombatN() const
{
	const AEONFloorManager* F = GetFloor();
	return FMath::Max(F ? F->GetCombatN() : CombatN, CombatNOverride);
}

int32 AEONCombatDirector::GetFloorN() const
{
	const AEONFloorManager* F = GetFloor();
	return F ? FMath::Max(1, F->GetFloorN()) : FloorN;
}

// ---------------------------------------------------------------- salles

bool AEONCombatDirector::IsQuiet() const
{
	if (Spawns.Num() || (!Cur.bCleared && Cur.Waves > 0)) { return false; }
	for (const AEONEnemy* E : Enemies) { if (E && !E->bDead) { return false; } }
	return true;
}

void AEONCombatDirector::ClearRoomContents()
{
	// loadRoom : enemies = [], ebullets = [], spawns = [], eggs = [] (les chiffres flottants aussi)
	for (AEONEnemy* E : Enemies) { if (E) { E->Destroy(); } }
	for (AEONEnemyProjectile* B : Bullets) { if (B) { B->Destroy(); } }
	for (AEONEnemyEgg* Q : Eggs) { if (Q) { Q->Destroy(); } }
	Enemies.Reset();
	Bullets.Reset();
	Eggs.Reset();
	Spawns.Reset();
	Nums.Reset();
}

void AEONCombatDirector::OnFloorRoomLoaded(AEONFloorManager* F)
{
	ClearRoomContents();
	AEONTileRoom* Rm = F ? F->GetRoom() : nullptr;
	CurRoom = Rm;
	Cur = FRoomState();
	if (!Rm) { return; }
	Cur.Role = F->GetCurrentRole();
	Cur.Waves = F->GetRoomWaves();
	Cur.StartT = F->GetRoomStartDelay();
	Cur.bCleared = F->IsCurrentRoomCleared() || Cur.Waves <= 0;
	Cur.bLocked = Rm->IsLocked();
	Cur.bCounted = true;
}

void AEONCombatDirector::EnterRoom(AEONTileRoom* Rm)
{
	// Mode sans étage : loadRoom (partie combat) appliqué aux salles posées dans la carte.
	ClearRoomContents();
	CurRoom = Rm;
	Cur = FRoomState();
	if (!Rm) { return; }
	FRoomState& S = States.FindOrAdd(Rm);
	if (S.Role.IsNone())
	{
		S.Role = Rm->RoomRole;
		const FName R = S.Role;
		S.bCleared = R == TEXT("DEPART") || R == TEXT("CALME") || R == TEXT("OPPORTUNITE") || R == TEXT("MARCHAND") || R == TEXT("TRESOR")
			|| R == TEXT("TRAINING") || R == TEXT("COM") || R == TEXT("SECURITE");
	}
	FloorN = FMath::Max(1, Rm->FloorN);
	S.Waves = 0; S.Wave = 0; S.StartT = 0.f; S.bLocked = false;
	if (!S.bCleared)
	{
		const FName R = S.Role;
		if (R == TEXT("COMBAT")) { S.bLocked = true; S.Waves = (GetCombatN() < 2 || FloorN == 4) ? 1 : 2; S.StartT = 0.6f; }
		else if (R == TEXT("TENSION")) { S.Waves = 1; S.StartT = 0.4f; }
		else if (R == TEXT("CORRIDOR")) { S.bLocked = true; S.Waves = 3; S.StartT = 0.9f; }
		else if (R == TEXT("FAILLE")) { S.bLocked = true; S.Waves = 1; S.StartT = 0.7f; }
		else
		{
			// ASCENSEUR (gardien, jalon 7), EVENEMENT et le reste : salle sans combat pour l'instant.
			S.bCleared = true;
			UE_LOG(LogEON, Display, TEXT("EON salle %s : rôle sans combat au jalon 3, marquée nettoyée"), *R.ToString());
		}
		if (!S.bCounted && !S.bCleared) { ++CombatN; S.bCounted = true; }
	}
	Rm->SetLocked(S.bLocked);
	Cur = S;
}

void AEONCombatDirector::StartCombat(int32 Waves, float StartT)
{
	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	Spawns.Reset();
	Cur.Role = TEXT("COMBAT");
	Cur.bCleared = false;
	Cur.Waves = FMath::Max(1, Waves);
	Cur.Wave = 0;
	Cur.StartT = StartT;
	Cur.bLocked = true;
	Rm->SetLocked(true);
}

float AEONCombatDirector::RoleMul(bool bFirst) const
{
	if (const AEONFloorManager* F = GetFloor()) { return F->GetWaveMul(bFirst); }
	const FName R = Cur.Role;
	if (bFirst) { return R == TEXT("TENSION") ? 0.6f : R == TEXT("ASCENSEUR") ? 1.2f : R == TEXT("CORRIDOR") ? 1.15f : R == TEXT("FAILLE") ? 0.9f : 1.f; }
	return R == TEXT("ASCENSEUR") ? 1.2f : R == TEXT("CORRIDOR") ? 1.5f : 1.f;
}

void AEONCombatDirector::Banner(const FString& Text, const FColor& Col) const
{
	if (AEONFloorManager* F = GetFloor()) { F->ShowBanner(Text, FLinearColor(Col)); return; }
	if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 1.8f, Col, Text, true, FVector2D(2.f, 2.f)); }
}

// ---------------------------------------------------------------- spawnWave

void AEONCombatDirector::WalkableTiles(float MinD, TArray<FVector2D>& Out) const
{
	Out.Reset();
	const AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	const FEONTileGrid& G = Rm->GetGrid();
	const int32 W = G.W, H = G.H;
	if (W <= 0 || H <= 0) { return; }
	// reach() : remplissage depuis les tuiles de porte, sur le sol et les portes (4 voisins).
	TArray<uint8> Reach;
	Reach.SetNumZeroed(W * H);
	TArray<int32> Q;
	for (uint8 D : { EONDoor::N, EONDoor::S, EONDoor::E, EONDoor::W })
	{
		if (!(G.Doors & D)) { continue; }
		FIntPoint T2[2];
		G.GetDoorTiles(D, T2);
		for (const FIntPoint& P : T2) { if (G.IsValidTile(P.X, P.Y)) { const int32 I = P.Y * W + P.X; if (!Reach[I]) { Reach[I] = 1; Q.Add(I); } } }
	}
	if (Q.Num() == 0)
	{
		// Salle sans porte (carte de test) : tout le sol compte comme atteignable.
		for (int32 I = 0; I < W * H; ++I) { if ((EEONTile)G.T[I] == EEONTile::Floor) { Reach[I] = 1; } }
	}
	static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	while (Q.Num())
	{
		const int32 I = Q.Pop(EAllowShrinking::No);
		const int32 X = I % W, Y = I / W;
		for (const FIntPoint& D : Dirs)
		{
			const int32 Nx = X + D.X, Ny = Y + D.Y;
			if (!G.IsValidTile(Nx, Ny)) { continue; }
			const int32 J = Ny * W + Nx;
			const EEONTile Tj = (EEONTile)G.T[J];
			if (!Reach[J] && (Tj == EEONTile::Floor || Tj == EEONTile::Door)) { Reach[J] = 1; Q.Add(J); }
		}
	}
	for (int32 I = 0; I < W * H; ++I)
	{
		if (!Reach[I] || (EEONTile)G.T[I] != EEONTile::Floor) { continue; }
		const int32 X = I % W, Y = I / W;
		if (G.Get(X + 1, Y) == EEONTile::Pit || G.Get(X - 1, Y) == EEONTile::Pit || G.Get(X, Y + 1) == EEONTile::Pit || G.Get(X, Y - 1) == EEONTile::Pit) { continue; }
		const FVector2D C((X + 0.5f) * TS, (Y + 0.5f) * TS);
		if (FVector2D::Distance(C, JiraPx) < MinD) { continue; }
		Out.Add(C);
	}
}

void AEONCombatDirector::SpawnWave(float Mul)
{
	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	Mul *= UEONStatics::GetGameFeel(this).WaveMul;
	// Le corridor : que du corps à corps, et tout arrive du fond du couloir
	const bool bCor = Cur.Role == TEXT("CORRIDOR");
	TArray<FVector2D> Cand;
	WalkableTiles(bCor ? 120.f : 95.f, Cand);
	if (Cand.Num() < 4) { WalkableTiles(55.f, Cand); }
	if (!Cand.Num())
	{
		// Le proto ressort sans compter la vague (la salle resterait fermée) : on la compte pour ne jamais bloquer.
		UE_LOG(LogEON, Warning, TEXT("EON spawnWave : aucune tuile libre, vague sautée"));
		++Cur.Wave;
		return;
	}
	// la surface se calcule sur TOUTE la salle (avant de ne garder que le fond du corridor)
	const int32 Span = Cand.Num();
	if (bCor)
	{
		const FVector2D P = JiraPx;
		Cand.Sort([P](const FVector2D& A, const FVector2D& B) { return FVector2D::Distance(A, P) > FVector2D::Distance(B, P); });
		Cand.SetNum(FMath::Min(Cand.Num(), FMath::Max(3, FMath::CeilToInt(Cand.Num() * 0.35f))));
	}
	const int32 CN = GetCombatN(), FN = GetFloorN();
	const float Area = FMath::Clamp(Span / 150.f, 0.85f, 1.35f);
	float Budget = FMath::Min(13.f, (5.f + CN * 0.75f) * Area * Mul) * (FN == 4 ? 0.5f : 1.f);

	struct FOpt { const TCHAR* T; float C; int32 Min; };
	static const FOpt CorOpts[] = { { TEXT("goule"), 3.2f, 1 }, { TEXT("kamikaze"), 1.1f, 1 }, { TEXT("slasher"), 2.1f, 1 }, { TEXT("brainless"), 1.8f, 1 } };
	static const FOpt StdOpts[] = { { TEXT("goule"), 3.2f, 1 }, { TEXT("kamikaze"), 1.1f, 1 }, { TEXT("meduse"), 1.7f, 2 }, { TEXT("sniper"), 1.9f, 2 },
		{ TEXT("brainless"), 1.8f, 2 }, { TEXT("slasher"), 2.1f, 3 }, { TEXT("invocateur"), 2.6f, 3 } };
	TArray<FOpt> Opts;
	// vertical slice : les types isolés (VSType) ne sont pas tirés dans les vagues
	if (bCor) { for (const FOpt& O : CorOpts) { if (CN >= O.Min && VSType(FName(O.T)) == FName(O.T)) { Opts.Add(O); } } }
	else { for (const FOpt& O : StdOpts) { if (CN >= O.Min && VSType(FName(O.T)) == FName(O.T)) { Opts.Add(O); } } }
	if (FN >= 2 && !bCor)
	{
		// étages 2+ : Brainless, méduses et Ubume deux fois plus probables
		const int32 N0 = Opts.Num();
		for (int32 I = 0; I < N0; ++I)
		{
			const FString T = Opts[I].T;
			if (T == TEXT("brainless") || T == TEXT("meduse") || T == TEXT("invocateur")) { const FOpt Copy = Opts[I]; Opts.Add(Copy); }
		}
	}

	struct FItem { FName T; FVector2D P; bool bElite = false; };
	TArray<FItem> List;
	int32 Guard = 0;
	while (Opts.Num() && Budget > 0.9f && Guard++ < 40)
	{
		const FOpt& O = PickOne(Opts);
		if (O.C > Budget + 0.5f && Budget < 2.f) { break; }
		Budget -= O.C;
		const FVector2D C = PickOne(Cand);
		if (FCString::Strcmp(O.T, TEXT("goule")) == 0)
		{
			// Nuke : arrivent en meute de 3 ou 4
			const int32 N = 3 + (FMath::FRand() < 0.45f ? 1 : 0);
			for (int32 I = 0; I < N; ++I)
			{
				FVector2D P(C.X + (FMath::FRand() - 0.5f) * 20.f, C.Y + (FMath::FRand() - 0.5f) * 20.f);
				if (Rm->TileAtPx(P) != EEONTile::Floor) { P = C; }
				List.Add({ FName(TEXT("goule")), P });
			}
		}
		else { List.Add({ FName(O.T), C }); }
	}
	// Élites : jamais enchaînées. Max 1 par salle, puis ELITE_PAUSE (2) salles de combat sans élite.
	if (Cur.Wave == 0 && !bCor && Cur.Role != TEXT("FAILLE"))
	{
		if (EliteCool > 0) { --EliteCool; }
		else if (CN >= 3 && FMath::FRand() < 0.4f)
		{
			TArray<int32> C;
			for (int32 I = 0; I < List.Num(); ++I)
			{
				const FName T = List[I].T;
				if (T == TEXT("slasher") || T == TEXT("sniper") || T == TEXT("brainless") || T == TEXT("invocateur")) { C.Add(I); }
			}
			if (C.Num()) { List[PickOne(C)].bElite = true; EliteCool = 2; }
		}
	}
	for (int32 I = 0; I < List.Num(); ++I)
	{
		FSpawn S;
		S.Type = List[I].T;
		S.Pos = List[I].P;
		S.T = 0.7f + I * 0.05f;
		S.bElite = List[I].bElite;
		Spawns.Add(S);
	}
	++Cur.Wave;
	EONSfx::Event(this, TEXT("tele"));
	UE_LOG(LogEON, Display, TEXT("EON vague %d/%d (%s, mul %.2f, combatN %d) : %d ennemis"), Cur.Wave, Cur.Waves, *Cur.Role.ToString(), Mul, CN, List.Num());
}

namespace { constexpr bool bVerticalSlice = true; }

FName AEONCombatDirector::VSType(FName Type)
{
	if (!bVerticalSlice) { return Type; }
	if (Type == TEXT("gouleP")) { return FName(TEXT("goule")); }
	if (Type == TEXT("brainless")) { return FName(TEXT("slasher")); }
	if (Type == TEXT("meduse")) { return NAME_None; }
	return Type;
}

void AEONCombatDirector::SpawnEnemyNow(const FSpawn& In)
{
	FSpawn S = In;
	S.Type = VSType(In.Type);
	if (S.Type.IsNone()) { return; }
	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEONEnemy* E = GetWorld()->SpawnActor<AEONEnemy>(AEONEnemy::StaticClass(), Rm->RoomPxToWorld(S.Pos), FRotator::ZeroRotator, P);
	if (!E) { return; }
	if (!E->Init(this, Rm, S.Type, S.Pos, GetCombatN())) { E->Destroy(); return; }
	UE_LOG(LogEON, Verbose, TEXT("EON apparition %s (demandé %s)"), *S.Type.ToString(), *In.Type.ToString());
	// COM-V8 : poussière d'apparition, 12 grains gris-violet qui s'étalent au sol (60 px/s, 0,45 s) et un anneau pâle.
	{
		const FVector Feet = Rm->RoomPxToWorld(S.Pos, EON::PxToCm(1.f));
		EONFx::Gerbe(this, Feet, 12, EONFx::Hex(TEXT("8a84b8")), 60.f, 0.45f, 1.5f, 4.f, 4.f);
		EONFx::Anneau(this, Rm->RoomPxToWorld(S.Pos, 0.f), 12.f, 0.3f, EONFx::Hex(TEXT("b8b4d8")));
	}
	if (S.bElite) { E->MakeElite(); }
	if (const AEONFloorManager* F = GetFloor(); F && F->IsKidDefending() && FMath::FRand() < 0.5f) { E->bKidT = true; }
	Enemies.Add(E);
}

// ---------------------------------------------------------------- appels des ennemis

void AEONCombatDirector::KillEnemy(AEONEnemy* E, float Dx, float Dy)
{
	if (!E || E->bDead) { return; }
	E->MarkDead();
	++Kills;
	if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->OnKill(); }
	const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
	Hitstop(F.Hitstop);
	Shake(0.18f);
	EONSfx::Event(E, TEXT("kill"));   // joué depuis l'ennemi : placé à sa position
	// Gerbe de mort (killEnemy) : 18 éclats dans le sens du coup (±63°), 40 à 170 px/s, 0,4 à 0,8 s, 1 sur 3 blanc, frein 5.
	if (const AEONTileRoom* Rm0 = CurRoom.Get())
	{
		const TCHAR* Hx = E->Type == TEXT("gen") ? TEXT("9acd32") : E->Type == TEXT("goule") ? TEXT("e8a24a") : E->Type == TEXT("gouleP") ? TEXT("6fbf4a")
			: E->Type == TEXT("kamikaze") ? TEXT("ff7a1a") : E->Type == TEXT("meduse") ? TEXT("8f7be0") : E->Type == TEXT("sniper") ? TEXT("e8923a")
			: E->Type == TEXT("slasher") ? TEXT("e0405a") : E->Type == TEXT("invocateur") ? TEXT("8a6aaa") : TEXT("ffffff");   // Ubume violette (Vincent, 24/11)
		EONFx::GerbeDir2(this, Rm0->RoomPxToWorld(E->Pos, EON::PxToCm(4.f)), FVector(Dx, Dy, 0.f), 18, 3, EONFx::Hex(Hx), FLinearColor::White, 170.f, 0.6f, 1.3f, 5.f);
	}
	if (E->Type == TEXT("kamikaze")) { Aoe(E->Pos, 26.f, 2.f, false, EONFx::Hex(TEXT("ff9a3d"))); }
	// Mort d'élite : gros coup (bichrome rouge, traits de vitesse, « BAM »).
	if (E->bElite) { EONFx::GrosCoup(this, E->GetActorLocation(), FLinearColor(1.f, 0.12f, 0.15f), 0); }
	// ADN (= XP) à chaque mort ; scrap rare, proportionnel à la valeur du mob. Ramassables : jalon 6, crédités d'office.
	const int32 Av = E->Adn;
	AEONTileRoom* Rm = CurRoom.Get();
	const AEONCharacter* J = GetJira();
	if (Av) { AdnGained += Av + (E->bElite ? 6 : 0); AEONPickup::DropADN(GetWorld(), Rm, E->Pos, Av + (E->bElite ? 6 : 0)); }
	if (E->bElite) { AEONPickup::Spawn(GetWorld(), Rm, EEONPickup::Donnee, E->Pos, FVector2D(FMath::FRandRange(-30.f, 30.f), -40.f), 2.f); }
	const int32 Nsc = ((Av && FMath::FRand() < 0.12f * Av * (J ? J->GetScrapM() : 1.f)) ? 1 : 0) + (E->bElite ? 3 : 0);
	Scrap += Nsc;
	for (int32 I = 0; I < Nsc; ++I) { AEONPickup::Spawn(GetWorld(), Rm, EEONPickup::Scrap, E->Pos, FVector2D(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f))); }
	// Énergie 25 %, cœur 4 %.
	const float R = FMath::FRand();
	if (R < 0.25f) { AEONPickup::Spawn(GetWorld(), Rm, EEONPickup::Energy, E->Pos, FVector2D(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f))); }
	else if (R < 0.29f) { AEONPickup::Spawn(GetWorld(), Rm, EEONPickup::Heart, E->Pos, FVector2D::ZeroVector); }
	(void)Dx; (void)Dy;
}

void AEONCombatDirector::Aoe(const FVector2D& At, float RPx, float Dmg, bool bHurtsPlayer, const FLinearColor& Col)
{
	EONSfx::Event(this, TEXT("explode"));
	for (int32 I = 0; I < Enemies.Num(); ++I)
	{
		AEONEnemy* E = Enemies[I];
		if (!E || E->IsFalling()) { continue; }
		const float D = FMath::Sqrt(FMath::Square(E->Pos.X - At.X) + FMath::Square(E->Ecy() - At.Y));
		if (D < RPx + E->Ehr())
		{
			const float Dd = D > 0.f ? D : 1.f;
			E->HitPx(Dmg, (E->Pos.X - At.X) / Dd, (E->Pos.Y - At.Y) / Dd, 3.f);
		}
	}
	if (bHurtsPlayer && FVector2D::Distance(JiraPx, At) < RPx * 0.8f) { HurtJira(1, At); }
	// 34 éclats (40 à 180 px/s, 0,35 à 0,65 s, 1 sur 3 blanc, 1 sur 4 de 2 px, frein 5) et l'anneau du rayon (0,25 s).
	if (const AEONTileRoom* Rm = CurRoom.Get())
	{
		const FVector W = Rm->RoomPxToWorld(At, EON::PxToCm(4.f));
		EONFx::Gerbe2(this, W, 34, 3, Col, FLinearColor::White, 180.f, 0.5f, 1.25f, 5.f);
		EONFx::Anneau(this, Rm->RoomPxToWorld(At, 0.f), RPx, 0.25f, Col);
		// ONI-V3 + style animé 90 (Vincent, 24/11) : dôme blanc bordé de la couleur de l'explosion, éclair de lumière,
		// puis fumée sombre qui monte (EONFx::Explosion, qui porte la lumière).
		EONFx::Explosion(this, Rm->RoomPxToWorld(At, 0.f), EON::PxToCm(RPx) * 0.8f, Col);
	}
	Shake(0.45f);
	Hitstop(UEONStatics::GetGameFeel(this).Hitstop * 0.6f);
}

void AEONCombatDirector::FireEnemyBullet(const FVector2D& ScreenPx, const FVector2D& VelPx, float Life, bool bBig, float HeightPx)
{
	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEONEnemyProjectile* B = GetWorld()->SpawnActor<AEONEnemyProjectile>(AEONEnemyProjectile::StaticClass(), Rm->RoomPxToWorld(ScreenPx), FRotator::ZeroRotator, P))
	{
		B->Init(Rm, ScreenPx, VelPx, Life, bBig, HeightPx);
		Bullets.Add(B);
		// Une volée = un son (écart minimal 80 ms) ; le gros tir orange plus fort.
		EONSfx::Event(this, TEXT("eshot"), bBig ? 1.3f : 0.85f);
	}
}

void AEONCombatDirector::LayEgg(const FVector2D& FromScreenPx, const FVector2D& ToPx)
{
	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEONEnemyEgg* Q = GetWorld()->SpawnActor<AEONEnemyEgg>(AEONEnemyEgg::StaticClass(), Rm->RoomPxToWorld(ToPx), FRotator::ZeroRotator, P))
	{
		Q->Init(Rm, FromScreenPx, ToPx);
		Eggs.Add(Q);
	}
}

void AEONCombatDirector::OnEnemyFell(AEONEnemy* E)
{
	if (!E) { return; }
	++Kills;
	if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->OnKill(); }
	Scrap += 1;
	AdnGained += E->Adn + (E->bElite ? 6 : 0);
	// Tombé dans le vide : +1 scrap garanti, son ADN est crédité directement.
	if (AEONCharacter* J = GetJira()) { J->AddScrap(1); J->GainADN(E->Adn + (E->bElite ? 6 : 0)); }
	if (const AEONTileRoom* Rm = CurRoom.Get())
	{
		AddNum(Rm->RoomPxToWorld(E->Pos, EON::PxToCm(6.f)), TEXT("Chute +1"), ColAm, 0.9f, -30.f);
	}
}

void AEONCombatDirector::AddNum(const FVector& World, const FString& Text, const FColor& Col, float Life, float VyPx, float Taille)
{
	FNum N;
	N.Loc = World;
	N.VelZ = -VyPx;   // vy < 0 = monte à l'écran = +Z
	N.Life = Life;
	N.Life0 = Life;
	N.Taille = Taille;
	N.Text = Text;
	N.Col = Col;
	Nums.Add(N);
}

void AEONCombatDirector::Shake(float V) const
{
	if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AEONPlayerCameraManager* Cam = Cast<AEONPlayerCameraManager>(PC->PlayerCameraManager)) { Cam->AddTrauma(V); }
	}
}

void AEONCombatDirector::Hitstop(float Ms) const
{
	if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(Ms); }
}

bool AEONCombatDirector::JiraHittable() const
{
	const AEONCharacter* J = GetJira();
	return J && !J->IsDead() && !J->IsInvulnerable() && !J->IsDashing() && !J->IsFallingIntoVoid();
}

void AEONCombatDirector::HurtJira(int32 Amount, const FVector2D& SourcePx, const FVector2D& PushPx)
{
	AEONCharacter* J = GetJira();
	const AEONTileRoom* Rm = CurRoom.Get();
	if (!J || !Rm) { return; }
	// Menu Tweak : dégâts des ennemis en coeurs (0 = inoffensifs).
	Amount = FMath::RoundToInt(Amount * UEONStatics::GetGameFeel(this).EnemyDmg);
	if (Amount <= 0) { return; }
	if (!PushPx.IsNearlyZero() && JiraHittable())
	{
		// Brainless : P.kx += lx × 170 en plus du recul de hurtPlayer
		J->GetCharacterMovement()->AddImpulse(FVector(EON::PxToCm(PushPx.X), EON::PxToCm(PushPx.Y), 0.f), true);
	}
	J->HurtPlayer(Amount, Rm->RoomPxToWorld(SourcePx));
}

void AEONCombatDirector::KillAll()
{
	for (int32 I = 0; I < Enemies.Num(); ++I)
	{
		AEONEnemy* E = Enemies[I];
		if (E && E->IsBoss()) { E->HitPx(E->Hp + 1.f, 0.f, 1.f); continue; }   // le gardien passe par sa mort (butin, fin d'étage) ; rien pendant l'intro
		if (E && !E->bDead && !E->IsFalling()) { KillEnemy(E, 0.f, 1.f); }
	}
	Spawns.Reset();
}

// ---------------------------------------------------------------- boucle

void AEONCombatDirector::UpdateWaves(float Dt)
{
	if (Cur.bCleared || Cur.Waves <= 0) { return; }
	if (Cur.StartT > 0.f)
	{
		Cur.StartT -= Dt;
		if (Cur.StartT <= 0.f) { SpawnWave(RoleMul(true)); }
		return;
	}
	// la vague suivante part dès qu'il ne reste qu'un ennemi : la pression ne retombe plus entre deux vagues
	int32 Alive = 0;
	for (const AEONEnemy* E : Enemies) { if (E && !E->bDead) { ++Alive; } }
	if (Spawns.Num() || Alive > (Cur.Wave < Cur.Waves ? 1 : 0)) { return; }
	if (Cur.Wave < Cur.Waves) { SpawnWave(RoleMul(false)); return; }
	// La faille ne se libère qu'au confinement, pas à la mort du dernier ennemi (la console le fait, sans bannière ni énergie).
	if (AEONFloorManager* F = GetFloor())
	{
		if (F->HoldsRoomClear()) { return; }
		if (F->GetCurrentRole() == TEXT("FAILLE") && F->IsCurrentRoomCleared()) { Cur.bCleared = true; Cur.bLocked = false; return; }
	}
	Cur.bCleared = true;
	const bool bWasLocked = Cur.bLocked;
	Cur.bLocked = false;
	if (AEONFloorManager* F = GetFloor()) { F->MarkCurrentRoomCleared(); }
	else if (AEONTileRoom* Rm = CurRoom.Get())
	{
		Rm->SetLocked(false);
		if (FRoomState* S = States.Find(Rm)) { S->bCleared = true; S->bLocked = false; }
	}
	if (AEONTileRoom* Rm = CurRoom.Get()) { Rm->SetLocked(false); }
	if (bWasLocked)
	{
		// « Retourne au C.O.M » (gardien) : jalon 7. Énergie au centre : ramassable du jalon 6.
		Banner(Cur.Role == TEXT("CORRIDOR") ? TEXT("Le corridor est franchi") : TEXT("Salle nettoyée"), ColCy);
		// Une énergie au centre de la salle.
		if (AEONTileRoom* Rm = CurRoom.Get()) { AEONPickup::Spawn(GetWorld(), Rm, EEONPickup::Energy, FVector2D(Rm->GetGrid().W * 8.f, Rm->GetGrid().H * 8.f - 20.f), FVector2D::ZeroVector, 1.f, 1.f); }
	}
}

void AEONCombatDirector::UpdateSpawns(float Dt)
{
	for (int32 I = Spawns.Num() - 1; I >= 0; --I)
	{
		Spawns[I].T -= Dt;
		if (Spawns[I].T <= 0.f)
		{
			const FSpawn S = Spawns[I];
			Spawns.RemoveAt(I);
			SpawnEnemyNow(S);
		}
	}
}

void AEONCombatDirector::UpdateBullets(float Dt)
{
	const bool bHittable = JiraHittable();
	for (int32 I = Bullets.Num() - 1; I >= 0; --I)
	{
		AEONEnemyProjectile* B = Bullets[I];
		if (!B) { Bullets.RemoveAt(I); continue; }
		bool bKill = B->Step(Dt);
		// Hologramme : un tir qui le touche lui enlève 1 PV (decoy.y − 10 : hauteur du leurre).
		if (!bKill && bDecoy && FVector2D::Distance(B->Pos, DecoyPx - FVector2D(0.f, 10.f)) < 7.f)
		{
			if (AEONCharacter* J = GetJira()) { J->DamageDecoy(1.f); }
			bKill = true;
		}
		if (!bKill && bHittable && PHit(B->Pos, JiraPx, 1.5f))
		{
			HurtJira(1, B->Pos);
			bKill = true;
		}
		if (bKill) { B->Destroy(); Bullets.RemoveAt(I); }
	}
}

void AEONCombatDirector::UpdateEggs(float Dt)
{
	for (int32 I = Eggs.Num() - 1; I >= 0; --I)
	{
		AEONEnemyEgg* Q = Eggs[I];
		if (!Q) { Eggs.RemoveAt(I); continue; }
		bool bHatch = false;
		if (Q->Step(Dt, bHatch))
		{
			if (bHatch)
			{
				// Éclosion : 16 éclats vert-jaune / vert (50 à 160 px/s, 0,35 s).
				EONFx::Gerbe2(this, CurRoom->RoomPxToWorld(Q->To, EON::PxToCm(2.f)), 16, 2, EONFx::Hex(TEXT("8aa82a")), EONFx::Hex(TEXT("d8f06a")), 160.f, 0.35f);
			}
			if (bHatch && !CurRoom->PitAtPx(Q->To))
			{
				FSpawn S;
				S.Type = TEXT("gouleP");
				S.Pos = Q->To;
				S.T = 0.05f;
				Spawns.Add(S);
			}
			Q->Destroy();
			Eggs.RemoveAt(I);
		}
	}
}

void AEONCombatDirector::UpdateNums(float Dt)
{
	UWorld* W = GetWorld();
	for (int32 I = Nums.Num() - 1; I >= 0; --I)
	{
		FNum& N = Nums[I];
		N.Life -= Dt;
		N.Loc.Z += EON::PxToCm(N.VelZ) * Dt;
		N.VelZ -= 60.f * Dt;   // n.vy += 60 dt
		if (N.Life <= 0.f) { Nums.RemoveAt(I); continue; }
	}
#if ENABLE_DRAW_DEBUG
	// Marqueurs d'apparition (carré magenta qui se resserre et clignote) et anneau d'AOE : placeholders en debug.
	if (const AEONTileRoom* Rm = CurRoom.Get())
	{
		for (const FSpawn& S : Spawns)
		{
			const float K = 1.f - S.T / 0.7f, R = EON::PxToCm(10.f - K * 6.f);
			if (FMath::Sin(Time * 30.f) > -0.3f)
			{
				DrawDebugBox(W, Rm->RoomPxToWorld(S.Pos, 2.f), FVector(R, R, 1.f), ColMg, false, -1.f, 0, 2.f);
			}
		}
	}
#endif
}

void AEONCombatDirector::RestartRun()
{
	ClearRoomContents();
	AEONPickup::ClearAll(GetWorld(), false);
	Kills = Scrap = AdnGained = Donnees = 0;
	CombatN = 0;
	EliteCool = 0;
	DeadSince = -1.0;
	States.Reset();
	Cur = FRoomState();
	// Nouvelle run : on repart du Kibo (HUB), comme le proto. Sans HUB (carte de test), on recharge la carte.
	if (GetFloor() && FPackageName::DoesPackageExist(TEXT("/Game/EON/Maps/L_HUB")))
	{
		UGameplayStatics::OpenLevel(this, FName(TEXT("L_HUB")));
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void AEONCombatDirector::UpdateDeathScreen()
{
	// drawDead : le titre « Signal perdu » est dessiné par AEONHUD ; ici le bilan de la run et la relance.
	const double Now = FPlatformTime::Seconds();
	if (DeadSince < 0.0)
	{
		DeadSince = Now;
		if (const AEONCharacter* J = GetJira()) { UE_LOG(LogEON, Display, TEXT("EON signal perdu : étage %d, %d ennemis, %d chutes, %d scrap"), GetFloorN(), Kills, J->GetFallCount(), Scrap); }
	}
	const AEONCharacter* J = GetJira();
	const AEONFloorManager* F = GetFloor();
	if (GEngine)
	{
		const FColor Wh = FColor::White;
		// Clés fixes : les lignes se remplacent d'une image à l'autre (ordre inverse d'affichage).
		GEngine->AddOnScreenDebugMessage(9105, 0.f, ColCy, TEXT("Clic ou R pour relancer  /  (A) pour relancer"), true, FVector2D(1.6f, 1.6f));
		GEngine->AddOnScreenDebugMessage(9104, 0.f, Wh, FString::Printf(TEXT("Chutes : %d   Scrap : %d   ADN : %d"), J ? J->GetFallCount() : 0, Scrap, AdnGained), true, FVector2D(1.4f, 1.4f));
		GEngine->AddOnScreenDebugMessage(9103, 0.f, Wh, FString::Printf(TEXT("Ennemis éliminés : %d"), Kills), true, FVector2D(1.4f, 1.4f));
		GEngine->AddOnScreenDebugMessage(9102, 0.f, Wh, FString::Printf(TEXT("Salles traversées : %d"), F ? F->GetRoomN() : 0), true, FVector2D(1.4f, 1.4f));
		GEngine->AddOnScreenDebugMessage(9101, 0.f, Wh, FString::Printf(TEXT("Étage atteint : %d"), FMath::Max(1, GetFloorN())), true, FVector2D(1.4f, 1.4f));
		GEngine->AddOnScreenDebugMessage(9100, 0.f, ColRed, TEXT("Signal perdu"), true, FVector2D(2.5f, 2.5f));
	}
	if (Now - DeadSince < 0.5) { return; }   // pas de relance sur un clic de tir encore enfoncé
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->WasInputKeyJustPressed(EKeys::R) || PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
		{
			RestartRun();
		}
	}
}

void AEONCombatDirector::Tick(float Dt)
{
	Super::Tick(Dt);
	Time += Dt;
	BindFloor();

	AEONCharacter* J = GetJira();
	if (!J) { return; }
	if (J->IsDead())
	{
		UpdateDeathScreen();
		return;
	}
	DeadSince = -1.0;

	// Mode sans étage : Jira entre dans une salle posée (à 20 px des murs, pas sur le seuil).
	if (!GetFloor())
	{
		if (AEONTileRoom* R = AEONTileRoom::FindTileRoomAt(this, J->GetActorLocation()))
		{
			if (R != CurRoom.Get())
			{
				const FVector2D P = R->WorldToRoomPx(J->GetActorLocation());
				const FEONTileGrid& G = R->GetGrid();
				if (P.X > 20.f && P.Y > 20.f && P.X < G.W * TS - 20.f && P.Y < G.H * TS - 20.f) { EnterRoom(R); }
			}
		}
	}

	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { UpdateNums(Dt); return; }
	JiraPx = Rm->WorldToRoomPx(J->GetActorLocation());
	Rm->ComputeFlow(JiraPx);
	bDecoy = J->HasDecoy();
	DecoyPx = bDecoy ? Rm->WorldToRoomPx(J->GetDecoyLocation()) : JiraPx;

	// Ordre du proto : tirs ennemis, vagues, apparitions, ennemis, œufs.
	UpdateBullets(Dt);
	UpdateWaves(Dt);
	UpdateSpawns(Dt);
	for (int32 I = 0; I < Enemies.Num(); ++I)
	{
		AEONEnemy* E = Enemies[I];
		if (!E) { continue; }
		// Cible : l'hologramme s'il est à moins de 240 px de l'ennemi, sinon Jira. Au contact, l'ennemi l'use (5 PV/s mêlée, 2 sinon).
		const bool bOnDecoy = bDecoy && FVector2D::Distance(DecoyPx, E->Pos) < 240.f;
		FVector2D KidPx;
		const AEONFloorManager* Fk = E->bKidT ? GetFloor() : nullptr;
		if (Fk && Fk->GetKidTarget(KidPx)) { E->UpdateEnemy(Dt, KidPx); continue; }
		E->UpdateEnemy(Dt, bOnDecoy ? DecoyPx : JiraPx);
		if (bOnDecoy && FVector2D::Distance(DecoyPx, E->Pos) < E->R + 8.f) { J->DamageDecoy(Dt * (E->bContact ? 5.f : 2.f)); }
	}
	for (int32 I = Enemies.Num() - 1; I >= 0; --I)
	{
		AEONEnemy* E = Enemies[I];
		if (!E || E->bDead)
		{
			// Cadavre : l'ennemi joue sa mort et se détruit seul (AEONEnemy::BeginCorpse) ; sinon il disparaît tout de suite.
			if (E && !E->BeginCorpse()) { E->Destroy(); }
			Enemies.RemoveAt(I);
			continue;
		}
		E->UpdateVisual(Dt, JiraPx);
	}
	UpdateEggs(Dt);
	UpdateNums(Dt);
}

void AEONCombatDirector::ShieldBurst(bool bMirror)
{
	AEONTileRoom* Rm = CurRoom.Get();
	if (!Rm) { return; }
	const FVector2D P = JiraPx;
	if (!bMirror)
	{
		ClearBullets([&P](const FVector2D& B) { return FVector2D::Distance(B, P) < 110.f; });
		for (int32 I = 0; I < Enemies.Num(); ++I)
		{
			AEONEnemy* E = Enemies[I];
			if (!E || E->bDead || E->IsFalling()) { continue; }
			const float D = FVector2D::Distance(E->Pos, P);
			if (D < 80.f) { const float Dd = D > 0.f ? D : 1.f; E->HitPx(E->IsBoss() ? 8.f : 5.f, (E->Pos.X - P.X) / Dd, (E->Pos.Y - P.Y) / Dd, 2.f); }
		}
		EONFx::Anneau(this, Rm->RoomPxToWorld(P, 0.f), 80.f, 0.35f, EONFx::Hex(TEXT("3ef2e0")));
		Shake(0.4f);
		return;
	}
	// Miroir : chaque tir proche repart 1,6× plus vite dans l'autre sens, 2,5 dégâts.
	int32 N = 0;
	for (int32 I = Bullets.Num() - 1; I >= 0; --I)
	{
		AEONEnemyProjectile* B = Bullets[I];
		if (!B || FVector2D::Distance(B->Pos, P) >= 120.f) { continue; }
		const FVector2D Ground(B->Pos.X, B->Pos.Y + B->HeightPx);
		const FVector Loc = Rm->RoomPxToWorld(Ground, EON::PxToCm(B->HeightPx));
		const FVector Vel(-B->Vel.X * 1.6f, -B->Vel.Y * 1.6f, 0.f);
		const FTransform Xf(Vel.Rotation(), Loc);
		if (AEONProjectile* Pb = GetWorld()->SpawnActorDeferred<AEONProjectile>(AEONProjectile::StaticClass(), Xf, GetJira(), GetJira(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Pb->Setup(Vel * EON::PxToCm(1.f), 2.5f, 2.5f, 2.f, FLinearColor(FColor(62, 242, 224)));
			Pb->FinishSpawning(Xf);
		}
		B->Destroy();
		Bullets.RemoveAt(I);
		++N;
	}
	EONFx::Anneau(this, Rm->RoomPxToWorld(P, 0.f), 46.f, 0.25f, EONFx::Hex(TEXT("e8f6ff")));
	if (N) { Banner(FString::Printf(TEXT("Miroir : %d tirs renvoyés"), N), FColor(62, 242, 224)); }
}

int32 AEONCombatDirector::ClearBullets(TFunctionRef<bool(const FVector2D&)> Pred)
{
	int32 N = 0;
	for (int32 I = Bullets.Num() - 1; I >= 0; --I)
	{
		AEONEnemyProjectile* B = Bullets[I];
		if (B && Pred(B->Pos)) { B->Destroy(); Bullets.RemoveAt(I); ++N; }
	}
	return N;
}

bool AEONCombatDirector::IsFightOver() const
{
	if (Cur.Waves <= 0 || Cur.StartT > 0.f || Cur.Wave < Cur.Waves || Spawns.Num()) { return false; }
	for (const AEONEnemy* E : Enemies) { if (E && !E->bDead) { return false; } }
	return true;
}

AEONEnemy* AEONCombatDirector::SpawnEnemyAt(FName Type, const FVector2D& Px, bool bElite)
{
	FSpawn S;
	S.Type = Type;
	S.Pos = Px;
	S.bElite = bElite;
	const int32 N = Enemies.Num();
	SpawnEnemyNow(S);
	return Enemies.Num() > N ? Enemies.Last().Get() : nullptr;
}

void AEONCombatDirector::StartRoomWaves(int32 Waves, float StartT)
{
	Cur.Waves = Waves;
	Cur.Wave = 0;
	Cur.StartT = StartT;
	Cur.bCleared = false;
	Cur.bLocked = true;
}
