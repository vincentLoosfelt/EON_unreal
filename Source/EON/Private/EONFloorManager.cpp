#include "EONFloorManager.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONTileRoom.h"
#include "EONCombatDirector.h"
#include "EONEnemy.h"
#include "EONItems.h"
#include "EONPedestal.h"
#include "EONPickup.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "EONTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	const uint8 DoorOrder[4] = { EONDoor::N, EONDoor::S, EONDoor::E, EONDoor::W };
	int32 R(FRandomStream& Rng, int32 A, int32 B) { return A + FMath::FloorToInt(Rng.GetFraction() * (B - A + 1)); }
	template <typename V> const V& Pick(FRandomStream& Rng, const TArray<V>& A) { return A[FMath::Min(A.Num() - 1, FMath::FloorToInt(Rng.GetFraction() * A.Num()))]; }

	/** Arête non orientée entre deux cases (ek du proto). */
	uint64 Ek(FIntPoint A, FIntPoint B)
	{
		if (B.X < A.X || (B.X == A.X && B.Y < A.Y)) { Swap(A, B); }
		return ((uint64)(A.X & 0xFF) << 24) | ((uint64)(A.Y & 0xFF) << 16) | ((uint64)(B.X & 0xFF) << 8) | (uint64)(B.Y & 0xFF);
	}

	/** Noms affichés des rôles (ROLE du proto). */
	FString RoleName(FName Role)
	{
		static const TMap<FName, FString> N = {
			{ TEXT("DEPART"), TEXT("C.O.M") }, { TEXT("COMBAT"), TEXT("Combat") }, { TEXT("TENSION"), TEXT("Tension") },
			{ TEXT("CALME"), TEXT("Calme") }, { TEXT("OPPORTUNITE"), TEXT("Opportunite") }, { TEXT("ASCENSEUR"), TEXT("Gardien") },
			{ TEXT("SECURITE"), TEXT("Securite") }, { TEXT("EVENEMENT"), TEXT("Evenement") }, { TEXT("MARCHAND"), TEXT("Marchand") },
			{ TEXT("CORRIDOR"), TEXT("Corridor") }, { TEXT("FAILLE"), TEXT("Faille") }, { TEXT("TRESOR"), TEXT("Tresor") },
			{ TEXT("IRRUPTION"), TEXT("Calme") }, { TEXT("CARTO"), TEXT("Cartographe") } };
		const FString* S = N.Find(Role);
		return S ? *S : Role.ToString();
	}


	FAutoConsoleCommandWithWorldAndArgs GEONCmdEtage(TEXT("eon.etage"), TEXT("eon.etage : étage suivant (nouvel étage généré, entrée par le C.O.M)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AEONFloorManager* F = AEONFloorManager::Get(World)) { F->StartFloor(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdMap(TEXT("eon.map"), TEXT("eon.map : plan de l'étage dans le log"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AEONFloorManager* F = AEONFloorManager::Get(World)) { UE_LOG(LogEON, Display, TEXT("%s"), *F->DescribeFloor()); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdGo(TEXT("eon.go"), TEXT("eon.go [N|S|E|W] : passe une porte de la salle (sans argument : vers une case pas encore visitée)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			uint8 D = 0;
			if (Args.Num())
			{
				const FString A = Args[0].ToUpper();
				D = A == TEXT("N") ? EONDoor::N : A == TEXT("S") ? EONDoor::S : A == TEXT("E") ? EONDoor::E : A == TEXT("W") ? EONDoor::W : 0;
			}
			if (AEONFloorManager* F = AEONFloorManager::Get(World)) { F->AutoExit(D); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdSalle(TEXT("eon.salle"), TEXT("eon.salle ROLE : va dans la salle de ce rôle (MARCHAND, OPPORTUNITE, TRESOR...)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONFloorManager* F = AEONFloorManager::Get(World);
			if (F && Args.Num() && !F->GoToRole(FName(*Args[0].ToUpper()))) { UE_LOG(LogEON, Warning, TEXT("eon.salle : pas de salle %s sur cet étage"), *Args[0]); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdAdn(TEXT("eon.adn"), TEXT("eon.adn N : donne N d'ADN à Jira"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(World, 0))) { J->GainADN(Args.Num() ? FCString::Atof(*Args[0]) : 50.f); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdScrap(TEXT("eon.scrap"), TEXT("eon.scrap N : donne N de scrap à Jira"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(World, 0))) { J->AddScrap(Args.Num() ? FCString::Atoi(*Args[0]) : 30); }
		}));
	TAutoConsoleVariable<FString> CVarEONDebut(TEXT("eon.debut"), TEXT(""), TEXT("eon.debut \"cmd1;cmd2\" : commandes lancées 2 s après le début (tests, ExecCmds trop tôt)"));
	TAutoConsoleVariable<int32> CVarEONTrace(TEXT("eon.trace"), 0, TEXT("eon.trace 1 : eon.ou toutes les 2 s"));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdOu(TEXT("eon.ou"), TEXT("eon.ou : position de Jira, de son mesh et de la caméra dans le log"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			APawn* P = UGameplayStatics::GetPlayerPawn(World, 0);
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			const ACharacter* Ch = Cast<ACharacter>(P);
			UE_LOG(LogEON, Display, TEXT("EON ou : pion %s en %s | tete %s | mesh %s visible %d echelle %s | camera %s"),
				P ? *P->GetClass()->GetName() : TEXT("aucun"), P ? *P->GetActorLocation().ToCompactString() : TEXT("-"),
				Ch ? *Ch->GetMesh()->GetSocketLocation(TEXT("head")).ToCompactString() : TEXT("-"),
				Ch ? *Ch->GetMesh()->GetComponentLocation().ToCompactString() : TEXT("-"), Ch ? (int32)Ch->GetMesh()->IsVisible() : -1,
				Ch ? *Ch->GetMesh()->GetComponentScale().ToCompactString() : TEXT("-"),
				(PC && PC->PlayerCameraManager) ? *PC->PlayerCameraManager->GetCameraLocation().ToCompactString() : TEXT("-"));
			// Ennemis : position en px de salle, vivant, ciblable.
			AEONTileRoom* Rm = P ? AEONTileRoom::FindTileRoomAt(World, P->GetActorLocation()) : nullptr;
			for (TActorIterator<AEONEnemy> It(World); It; ++It)
			{
				const FVector2D Px = Rm ? Rm->WorldToRoomPx(It->GetActorLocation()) : FVector2D::ZeroVector;
				UE_LOG(LogEON, Display, TEXT("EON ou : ennemi %s px (%.0f, %.0f) z %.0f mort %d ciblable %d solide %d vide %d"), *It->GetName(), Px.X, Px.Y,
					It->GetActorLocation().Z, (int32)It->bDead, (int32)It->IsTargetable(), Rm ? (int32)Rm->SolidAtPx(Px) : -1, Rm ? (int32)Rm->PitAtPx(Px) : -1);
			}
		}));
}

AEONFloorManager::AEONFloorManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarteRouge"));
	CardMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MCard(TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"));
	CardMesh->SetStaticMesh(Cube.Object);
	CardMesh->SetMobility(EComponentMobility::Movable);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardMesh->SetWorldScale3D(FVector(0.32f, 0.22f, 0.06f));
	CardMaterial = MCard.Object;
	CardMesh->SetVisibility(false);
}

AEONFloorManager* AEONFloorManager::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) { return nullptr; }
	for (TActorIterator<AEONFloorManager> It(World); It; ++It) { return *It; }
	return nullptr;
}

void AEONFloorManager::BeginPlay()
{
	if (CardMesh && CardMaterial)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(CardMaterial, this);
		M->SetVectorParameterValue(TEXT("BC1"), FLinearColor(1.f, 0.24f, 0.33f));
		M->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(3.f, 0.2f, 0.35f));
		CardMesh->SetMaterial(0, M);
	}
	Super::BeginPlay();
	Rng.Initialize(FPlatformTime::Cycles());
	AEONPedestal::OnTaken.AddUObject(this, &AEONFloorManager::OnPedestalTaken);
	LoadTemplates();
	// Une salle unique, au pivot du gestionnaire, reconstruite à chaque entrée.
	for (TActorIterator<AEONTileRoom> It(GetWorld()); It; ++It) { Room = *It; break; }
	if (!Room)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Room = GetWorld()->SpawnActor<AEONTileRoom>(AEONTileRoom::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, P);
	}
	// Laisse le temps au pion d'apparaître.
	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, this, &AEONFloorManager::NewRun, 0.05f, false);
}

AEONCharacter* AEONFloorManager::GetJira() const
{
	return Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

FName AEONFloorManager::GetCurrentRole() const
{
	const FEONFloorCell* C = CellAt(Cur);
	return C ? C->Role : NAME_None;
}

bool AEONFloorManager::IsCurrentRoomCleared() const
{
	const FEONFloorCell* C = CellAt(Cur);
	return !C || C->bCleared;
}

void AEONFloorManager::MarkCurrentRoomCleared()
{
	if (FEONFloorCell* C = CellAt(Cur)) { C->bCleared = true; }
	RoomWaves = 0;
	if (Room) { Room->SetLocked(false); }
}

float AEONFloorManager::GetWaveMul(bool bFirst) const
{
	const FName Ro = GetCurrentRole();
	// update() : 1re vague spawnWave(TENSION 0,6 / ASCENSEUR 1,2 / CORRIDOR 1,15 / FAILLE 0,9 / 1), suivantes (ASCENSEUR 1,2 / CORRIDOR 1,5 / 1).
	if (bFirst)
	{
		return Ro == TEXT("TENSION") ? 0.6f : Ro == TEXT("ASCENSEUR") ? 1.2f : Ro == TEXT("CORRIDOR") ? 1.15f : Ro == TEXT("FAILLE") ? 0.9f : 1.f;
	}
	return Ro == TEXT("ASCENSEUR") ? 1.2f : Ro == TEXT("CORRIDOR") ? 1.5f : 1.f;
}

void AEONFloorManager::NewRun()
{
	FloorN = 0;
	CombatN = 0;
	RoomN = 0;
	if (AEONCharacter* J = GetJira()) { J->ResetRun(); }
	StartFloor();
}

void AEONFloorManager::StartFloor()
{
	++FloorN;
	bRedCard = false;
	bFloorDone = false;
	AmbushT = -1.f;
	ComTravelT = -1.f;
	bComGo = false;
	if (AEONCharacter* J = GetJira()) { J->RefillShield(); }
	GenFloor();
	EnterCell(Start, 0, true);
	ShowBanner(FloorN == 2 ? TEXT("Etage 2 - Les cuves") : FloorN == 3 ? TEXT("Etage 3 - Les labos") : FloorN == 4 ? TEXT("Etage 4 - Les serres") : FString::Printf(TEXT("Etage %d"), FloorN), FLinearColor(FColor(62, 242, 224)));
}

// ---------------------------------------------------------------- genFloor

void AEONFloorManager::GenFloor()
{
	TSet<uint64> E;
	TArray<FIntPoint> Path, Dead;
	auto Nbr = [this](FIntPoint C)
	{
		TArray<FIntPoint> Out;
		for (uint8 D : DoorOrder) { const FIntPoint N = C + EONDoor::Dir(D); if (Cells.Contains(N)) { Out.Add(N); } }
		return Out;
	};
	auto Keys = [this]() { TArray<FIntPoint> K; Cells.GetKeys(K); return K; };

	for (int32 Attempt = 0; Attempt < 400; ++Attempt)
	{
		Cells.Reset(); E.Reset(); Path.Reset(); Dead.Reset();
		Start = FIntPoint(3, 3);
		Cells.Add(Start).Pos = Start;
		TArray<FIntPoint> Arms;
		for (uint8 D : DoorOrder) { const FIntPoint N = Start + EONDoor::Dir(D); Cells.Add(N).Pos = N; E.Add(Ek(Start, N)); Arms.Add(N); }
		Boss = Pick(Rng, Arms);
		const int32 Target = FloorN == 4 ? 8 + R(Rng, 0, 1) : 12 + FMath::Min(5, FloorN * 2) + R(Rng, 0, 2);
		TArray<FIntPoint> Q;
		for (const FIntPoint& A : Arms) { if (A != Boss) { Q.Add(A); } }
		int32 Guard = 0;
		while (Cells.Num() < Target && Guard++ < 600)
		{
			if (!Q.Num())
			{
				TArray<FIntPoint> Pool;
				for (const FIntPoint& K : Keys()) { if (K != Start && K != Boss) { Pool.Add(K); } }
				Q.Add(Pick(Rng, Pool));
			}
			const FIntPoint C = Q[0]; Q.RemoveAt(0);
			TArray<uint8> Ds = { EONDoor::N, EONDoor::S, EONDoor::E, EONDoor::W };
			for (int32 I = Ds.Num() - 1; I > 0; --I) { Ds.Swap(I, Rng.RandRange(0, I)); }
			for (uint8 D : Ds)
			{
				const FIntPoint N = C + EONDoor::Dir(D);
				if (N.X < 0 || N.Y < 0 || N.X > 6 || N.Y > 6 || Cells.Contains(N) || Cells.Num() >= Target) { continue; }
				if (Rng.GetFraction() < 0.5f) { continue; }
				Cells.Add(N).Pos = N; E.Add(Ek(C, N)); Q.Add(N);
			}
		}
		if (Cells.Num() < Target - 1) { continue; }
		// Quelques boucles (10 %), jamais vers le C.O.M ni le gardien.
		for (const FIntPoint& C : Keys())
		{
			if (C == Start || C == Boss) { continue; }
			for (const FIntPoint& N : Nbr(C)) { if (N == Start || N == Boss || E.Contains(Ek(C, N))) { continue; } if (Rng.GetFraction() < 0.1f) { E.Add(Ek(C, N)); } }
		}
		// Chemin vers l'autre entrée du gardien : au moins 3 salles, sans passer par lui.
		if (FloorN != 4)
		{
			TMap<FIntPoint, FIntPoint> Prev; Prev.Add(Start, FIntPoint(-1, -1));
			TArray<FIntPoint> Bq = { Start };
			for (int32 I = 0; I < Bq.Num(); ++I)
			{
				const FIntPoint C = Bq[I];
				for (const FIntPoint& N : Nbr(C)) { if (N == Boss || Prev.Contains(N) || !E.Contains(Ek(C, N))) { continue; } Prev.Add(N, C); Bq.Add(N); }
			}
			TArray<TPair<FIntPoint, TArray<FIntPoint>>> Cand;
			for (const FIntPoint& N : Nbr(Boss))
			{
				if (N == Start || !Prev.Contains(N)) { continue; }
				TArray<FIntPoint> P;
				for (FIntPoint C = N; C != Start && C.X >= 0; C = Prev[C]) { P.Insert(C, 0); }
				if (P.Num() >= 3) { Cand.Add({ N, P }); }
			}
			if (!Cand.Num()) { continue; }
			const auto& Ch = Pick(Rng, Cand);
			E.Add(Ek(Boss, Ch.Key)); Path = Ch.Value;
		}
		for (const FIntPoint& C : Keys())
		{
			if (C == Start || C == Boss || Path.Contains(C)) { continue; }
			int32 N = 0;
			for (const FIntPoint& M : Nbr(C)) { if (M != Boss && E.Contains(Ek(C, M))) { ++N; } }
			if (N == 1) { Dead.Add(C); }
		}
		if (Dead.Num() >= (FloorN == 4 ? 1 : 3)) { break; }
	}

	for (auto& KV : Cells)
	{
		FEONFloorCell& C = KV.Value;
		C.Doors = 0;
		for (uint8 D : DoorOrder) { const FIntPoint N = C.Pos + EONDoor::Dir(D); if (Cells.Contains(N) && E.Contains(Ek(C.Pos, N))) { C.Doors |= D; } }
	}
	bInfest = FloorN != 4 && FMath::CountBits(Cells[Boss].Doors) >= 2;

	// Distances depuis le C.O.M ; culs-de-sac du plus loin au plus près.
	TMap<FIntPoint, int32> Dist; Dist.Add(Start, 0);
	{
		TArray<FIntPoint> Bq = { Start };
		for (int32 I = 0; I < Bq.Num(); ++I)
		{
			const FEONFloorCell& C = Cells[Bq[I]];
			for (uint8 D : DoorOrder) { if (!(C.Doors & D)) { continue; } const FIntPoint N = C.Pos + EONDoor::Dir(D); if (!Dist.Contains(N)) { Dist.Add(N, Dist[C.Pos] + 1); Bq.Add(N); } }
		}
	}
	Dead.Sort([&Dist](const FIntPoint& A, const FIntPoint& B) { return Dist.FindRef(A) > Dist.FindRef(B); });
	Cells[Start].Role = TEXT("DEPART");
	Cells[Boss].Role = TEXT("ASCENSEUR");
	if (FloorN != 4)
	{
		FIntPoint Sc(-1, -1);
		if (Path.Num() && Rng.GetFraction() < 0.35f)
		{
			TArray<FIntPoint> Br;
			for (auto& KV : Cells)
			{
				const FIntPoint C = KV.Key;
				if (C == Start || C == Boss || Path.Contains(C) || !KV.Value.Role.IsNone()) { continue; }
				for (const FIntPoint& N : Nbr(C)) { if (Path.Contains(N) && E.Contains(Ek(C, N))) { Br.Add(C); break; } }
			}
			if (Br.Num()) { Sc = Pick(Rng, Br); }
		}
		if (Sc.X < 0 && Path.Num()) { Sc = Path[1 + FMath::FloorToInt(Rng.GetFraction() * (Path.Num() - 1))]; }
		if (Sc.X < 0)
		{
			TArray<FIntPoint> Pool = Dead;
			if (!Pool.Num()) { for (const FIntPoint& K : Keys()) { if (K != Start && K != Boss) { Pool.Add(K); } } }
			Sc = Pick(Rng, Pool);
		}
		Cells[Sc].Role = TEXT("SECURITE");
		Dead.Remove(Sc);
	}
	TArray<FIntPoint> Pool = Dead;
	auto Take = [&]() -> FIntPoint
	{
		if (Pool.Num()) { const FIntPoint P = Pool[0]; Pool.RemoveAt(0); return P; }
		for (auto& KV : Cells) { if (KV.Value.Role.IsNone() && !Path.Contains(KV.Key)) { return KV.Key; } }
		for (auto& KV : Cells) { if (KV.Value.Role.IsNone()) { return KV.Key; } }
		return FIntPoint(-1, -1);
	};
	const TArray<FName> Fixed = FloorN == 4 ? TArray<FName>{ TEXT("OPPORTUNITE"), TEXT("CALME") } : TArray<FName>{ TEXT("EVENEMENT"), TEXT("MARCHAND"), TEXT("OPPORTUNITE") };
	for (const FName& Ro : Fixed) { const FIntPoint C = Take(); if (C.X >= 0) { Cells[C].Role = Ro; } }
	if (Rng.GetFraction() < (FloorN == 4 ? 0.5f : 0.7f)) { const FIntPoint C = Take(); if (C.X >= 0 && Cells[C].Role.IsNone()) { Cells[C].Role = TEXT("TRESOR"); } }
	if (FloorN <= 3 && Rng.GetFraction() < 0.5f) { const FIntPoint C = Take(); if (C.X >= 0 && Cells[C].Role.IsNone()) { Cells[C].Role = TEXT("CARTO"); } }
	for (auto& KV : Cells)
	{
		if (!KV.Value.Role.IsNone()) { continue; }
		const float Rr = Rng.GetFraction();
		KV.Value.Role = FloorN == 4 ? (Rr < 0.4f ? TEXT("COMBAT") : Rr < 0.75f ? TEXT("TENSION") : TEXT("CALME"))
			: (Rr < 0.62f ? TEXT("COMBAT") : Rr < 0.9f ? TEXT("TENSION") : TEXT("CALME"));
	}
	auto IsLine = [](uint8 D) { return D == (EONDoor::N | EONDoor::S) || D == (EONDoor::E | EONDoor::W); };
	if (FloorN != 4)
	{
		TArray<FIntPoint> Cand;
		for (auto& KV : Cells) { if ((KV.Value.Role == TEXT("COMBAT") || KV.Value.Role == TEXT("TENSION")) && IsLine(KV.Value.Doors)) { Cand.Add(KV.Key); } }
		if (Cand.Num())
		{
			const FIntPoint C = Pick(Rng, Cand);
			Cells[C].Role = TEXT("CORRIDOR");
			TArray<FIntPoint> Nb, Calme;
			for (uint8 D : DoorOrder)
			{
				if (!(Cells[C].Doors & D)) { continue; }
				const FIntPoint N = C + EONDoor::Dir(D);
				const FEONFloorCell* NC = Cells.Find(N);
				if (NC && (NC->Role == TEXT("COMBAT") || NC->Role == TEXT("TENSION") || NC->Role == TEXT("CALME"))) { Nb.Add(N); if (NC->Role == TEXT("CALME")) { Calme.Add(N); } }
			}
			if (Calme.Num()) { Cells[Pick(Rng, Calme)].Role = TEXT("OPPORTUNITE"); }
			else if (Nb.Num()) { Cells[Pick(Rng, Nb)].Role = TEXT("OPPORTUNITE"); }
		}
		TArray<FIntPoint> Ct;
		for (auto& KV : Cells) { if (KV.Value.Role == TEXT("COMBAT") || KV.Value.Role == TEXT("TENSION")) { Ct.Add(KV.Key); } }
		if (Ct.Num()) { Cells[Pick(Rng, Ct)].Role = TEXT("FAILLE"); }
	}
	if (FloorN <= 3 && Rng.GetFraction() < 0.5f)
	{
		TArray<FIntPoint> Ct;
		for (auto& KV : Cells) { if (KV.Value.Role == TEXT("COMBAT") || KV.Value.Role == TEXT("TENSION")) { Ct.Add(KV.Key); } }
		if (Ct.Num()) { Cells[Pick(Rng, Ct)].Role = TEXT("IRRUPTION"); }
	}
	for (auto& KV : Cells)
	{
		FEONFloorCell& C = KV.Value;
		const FName Ro = C.Role;
		const bool bLine = IsLine(C.Doors);
		const bool bNS = C.Doors == (EONDoor::N | EONDoor::S);
		if ((Ro == TEXT("COMBAT") || Ro == TEXT("TENSION") || Ro == TEXT("CALME")) && bLine && Rng.GetFraction() < 0.6f) { C.Shape = bNS ? TEXT("corV") : TEXT("corH"); }
		else if (Ro == TEXT("ASCENSEUR")) { C.Shape = TEXT("std"); }
		else if (Ro == TEXT("OPPORTUNITE")) { C.Shape = TEXT("small"); }
		else if (Ro == TEXT("CORRIDOR")) { C.Shape = bNS ? TEXT("corV") : TEXT("corH"); }
		else if (Ro == TEXT("FAILLE")) { C.Shape = Pick(Rng, TArray<FName>{ TEXT("std"), TEXT("large") }); }
		else if (Ro == TEXT("COMBAT")) { C.Shape = Pick(Rng, TArray<FName>{ TEXT("std"), TEXT("std"), TEXT("large"), TEXT("small") }); }
		else if (Ro == TEXT("TENSION")) { C.Shape = Pick(Rng, TArray<FName>{ TEXT("std"), TEXT("large") }); }
		else if (Ro == TEXT("CALME")) { C.Shape = Pick(Rng, TArray<FName>{ TEXT("small"), TEXT("std") }); }
		else { C.Shape = TEXT("std"); }
	}
	Cur = Start;
	UE_LOG(LogEON, Display, TEXT("EON étage %d : %d salles, gardien en (%d,%d), infestation %d\n%s"), FloorN, Cells.Num(), Boss.X, Boss.Y, bInfest ? 1 : 0, *DescribeFloor());
}

FString AEONFloorManager::DescribeFloor() const
{
	FString S;
	for (int32 Y = 0; Y < 7; ++Y)
	{
		for (int32 X = 0; X < 7; ++X)
		{
			const FEONFloorCell* C = CellAt(FIntPoint(X, Y));
			if (!C) { S += TEXT("  .  "); continue; }
			const FString R = C->Role.ToString().Left(3);
			S += FString::Printf(TEXT("%s%s%s "), FIntPoint(X, Y) == Cur ? TEXT("[") : TEXT(" "), *R, FIntPoint(X, Y) == Cur ? TEXT("]") : TEXT(" "));
		}
		S += TEXT("\n");
	}
	return S;
}

// ---------------------------------------------------------------- enterCell / loadRoom

void AEONFloorManager::EnterCell(FIntPoint P, uint8 FromDir, bool bFromCom)
{
	FEONFloorCell* C = CellAt(P);
	if (!C) { return; }
	Cur = P;
	if (!C->bVisited) { C->bVisited = true; ++RoomN; }
	C->bSeen = true;
	for (uint8 D : DoorOrder) { if (C->Doors & D) { if (FEONFloorCell* N = CellAt(P + EONDoor::Dir(D))) { N->bSeen = true; } } }
	if (!C->bBuilt)
	{
		// buildRoom : DEPART d'un étage = salle du C.O.M ; Trésor/Irruption -> Calme, Cartographe -> Événement (genTiles).
		const FName R = C->Role;
		if (R == TEXT("DEPART")) { C->Grid = FEONTileGrid::ComTiles(C->Doors); }
		else
		{
			const FName G = R == TEXT("TRESOR") || R == TEXT("IRRUPTION") ? FName(TEXT("CALME")) : R == TEXT("CARTO") ? FName(TEXT("EVENEMENT")) : R;
			// Salle dessinée à la main si un Blueprint convient, sinon génération du proto.
			C->RoomClass = PickTemplate(R, C->Shape);
			if (!C->RoomClass) { C->RoomClass = PickTemplate(G, C->Shape); }
			FString Err;
			if (!C->RoomClass || !AEONTileRoom::LayoutToGrid(C->RoomClass->GetDefaultObject<AEONTileRoom>()->Layout, C->Doors, C->Grid, &Err))
			{
				if (C->RoomClass) { UE_LOG(LogEON, Warning, TEXT("Salle %s inutilisable (%s)"), *C->RoomClass->GetName(), *Err); }
				C->RoomClass = nullptr;
				C->Grid = FEONTileGrid::GenTiles(G, C->Shape, C->Doors, Rng, FloorN);
			}
		}
		// setInfest : chair sur la passerelle du C.O.M vers le gardien, et sur la porte côté gardien (la porte et 2 tuiles devant).
		if (bInfest && !bFloorDone && (R == TEXT("DEPART") || R == TEXT("ASCENSEUR")))
		{
			const bool bCom = R == TEXT("DEPART");
			const FIntPoint To = bCom ? Boss : Start;
			uint8 D = 0;
			for (uint8 Dd : DoorOrder) { if (P + EONDoor::Dir(Dd) == To) { D = Dd; } }
			if (D && (C->Doors & D))
			{
				const FEONTileGrid& G = C->Grid;
				const int32 W = G.W, H = G.H, Cx = W >> 1, Cy = H >> 1;
				TSet<int32> Set;
				FIntPoint DT[2]; FEONTileGrid::DoorTiles(D, W, H, DT);
				for (const FIntPoint& Q : DT) { Set.Add(Q.Y * W + Q.X); }
				if (bCom)
				{
					for (int32 Y = 1; Y < H - 1; ++Y) for (int32 X = 1; X < W - 1; ++X)
					{
						const int32 I = Y * W + X;
						if (G.T[I] != (uint8)EEONTile::Floor) { continue; }
						const float Ex = (X + 0.5f - Cx) / 6.3f, Ey = (Y + 0.5f - Cy) / 5.3f;
						if (Ex * Ex + Ey * Ey <= 1.05f) { continue; }
						if ((D == EONDoor::N && Y < Cy && FMath::Abs(X - Cx + 0.5f) < 3) || (D == EONDoor::S && Y > Cy && FMath::Abs(X - Cx + 0.5f) < 3)
							|| (D == EONDoor::W && X < Cx && FMath::Abs(Y - Cy + 0.5f) < 3) || (D == EONDoor::E && X > Cx && FMath::Abs(Y - Cy + 0.5f) < 3)) { Set.Add(I); }
					}
				}
				else
				{
					const FIntPoint Dv = EONDoor::Dir(D);
					for (const FIntPoint& Q : DT) for (int32 K = 1; K <= 2; ++K)
					{
						const int32 Xx = Q.X - Dv.X * K, Yy = Q.Y - Dv.Y * K;
						if (Xx > 0 && Yy > 0 && Xx < W - 1 && Yy < H - 1 && G.T[Yy * W + Xx] == (uint8)EEONTile::Floor) { Set.Add(Yy * W + Xx); }
					}
				}
				C->Infest = Set.Array();
				C->InfestK = 1.f;
			}
		}
		// Socles (buildRoom) : Opportunité 3 gratuits, Marchand 3 payants, Trésor (stock) 1 disquette.
		{
			const AEONCharacter* J = GetJira();
			const float Cx = (C->Grid.W >> 1) * 16.f, Cy = (C->Grid.H >> 1) * 16.f;
			if (R == TEXT("OPPORTUNITE"))
			{
				const TArray<FName> It = EONItems::Roll(3, J, false);
				for (int32 I = 0; I < It.Num(); ++I) { C->PedItems.Add(It[I]); C->PedPrice.Add(0); C->PedPx.Add(FVector2D(Cx + (I - 1) * 56.f, Cy - 4.f)); }
			}
			else if (R == TEXT("MARCHAND"))
			{
				const TArray<FName> It = EONItems::Roll(3, J, false);
				for (int32 I = 0; I < It.Num(); ++I) { C->PedItems.Add(It[I]); C->PedPrice.Add(EONItems::Price(*EONItems::Find(It[I]))); C->PedPx.Add(FVector2D(Cx + (I - 1) * 44.f, 4 * 16.f + 10.f)); }
			}
			else if (R == TEXT("TRESOR") && Rng.GetFraction() < 0.5f)
			{
				FName Pick = NAME_None;
				for (const FName& Id : EONItems::Roll(4, J, false)) { if (EONItems::Find(Id)->Cat == TEXT("DISQUETTE")) { Pick = Id; break; } }
				if (Pick.IsNone()) { const TArray<FName> One = EONItems::Roll(1, J, false); if (One.Num()) { Pick = One[0]; } }
				if (!Pick.IsNone()) { C->PedItems.Add(Pick); C->PedPrice.Add(0); C->PedPx.Add(FVector2D(Cx, Cy - 18.f)); }
				C->bStock = true;   // butin « stock » (10 scrap + disquette) ; sinon « carnage » (ADN et énergie)
			}
		}
		C->bCleared = R == TEXT("DEPART") || R == TEXT("CALME") || R == TEXT("OPPORTUNITE") || R == TEXT("MARCHAND") || R == TEXT("TRESOR")
			|| R == TEXT("SECURITE");   // la carte rouge et son embuscade : jalon 5
		C->bBuilt = true;
	}
	LoadRoom(*C, FromDir ? EONDoor::Opp(FromDir) : 0, bFromCom);
}

void AEONFloorManager::LoadRoom(FEONFloorCell& C, uint8 Entry, bool bFromCom)
{
	if (!Room) { return; }
	// enterCell : l'ADN encore en vol est crédité (on ne le perd pas en sortant vite), le reste disparaît.
	AEONPickup::ClearAll(GetWorld(), true);
	// Salle dessinée : on remplace l'acteur salle par son Blueprint (décor posé à la main compris).
	UClass* Want = C.RoomClass ? C.RoomClass.Get() : AEONTileRoom::StaticClass();
	if (Room->GetClass() != Want)
	{
		const FTransform Xf = Room->GetActorTransform();
		Room->Destroy();
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Room = GetWorld()->SpawnActor<AEONTileRoom>(Want, Xf, P);
		UE_LOG(LogEON, Display, TEXT("Salle : %s"), *Want->GetName());
	}
	Room->SetGrid(C.Grid);
	RoomWaves = 0;
	RoomStartT = 0.f;
	bool bLocked = false;
	const FName R = C.Role;
	if (!C.bCleared)
	{
		if (R == TEXT("COMBAT")) { bLocked = true; RoomWaves = (CombatN < 2 || FloorN == 4) ? 1 : 2; RoomStartT = 0.6f; }
		else if (R == TEXT("TENSION")) { RoomWaves = 1; RoomStartT = 0.4f; }
		else if (R == TEXT("CORRIDOR")) { bLocked = true; RoomWaves = 3; RoomStartT = 0.9f; }
		else if (R == TEXT("FAILLE")) { bLocked = true; RoomWaves = 1; RoomStartT = 0.7f; }   // la brèche et l'oxygène : jalon 9
		else if (R == TEXT("ASCENSEUR")) { C.bCleared = true; bFloorDone = true; }    // gardien : jalon 7 (salle vide, le levier s'active)
		else if (R == TEXT("EVENEMENT") || R == TEXT("CARTO") || R == TEXT("IRRUPTION")) { C.bCleared = true; }   // events : jalons 8–9
		if (!C.bCounted && !C.bCleared) { ++CombatN; C.bCounted = true; }
	}
	Room->SetLocked(bLocked);
	// Porte rouge (Doom) : la porte vers la salle du gardien reste fermée sans la carte (sauf étage 4).
	uint8 Red = 0;
	if (!bRedCard && FloorN != 4)
	{
		for (uint8 D : DoorOrder)
		{
			const FEONFloorCell* N = (C.Doors & D) ? CellAt(C.Pos + EONDoor::Dir(D)) : nullptr;
			if (N && N->Role == TEXT("ASCENSEUR")) { Red = D; }
		}
	}
	Room->SetRedDoor(Red);
	Room->SetInfest(C.Infest, C.InfestK);
	// Socles et butin du trésor.
	AEONPedestal::ClearAll(GetWorld());
	for (int32 I = 0; I < C.PedItems.Num(); ++I) { AEONPedestal::Spawn(GetWorld(), Room, C.PedItems[I], C.PedPx[I], C.PedPrice[I], I); }
	if (R == TEXT("TRESOR") && !C.bLootDone)
	{
		C.bLootDone = true;
		const float Cx = (C.Grid.W >> 1) * 16.f, Cy = (C.Grid.H >> 1) * 16.f;
		if (C.bStock)
		{
			for (int32 K = 0; K < 10; ++K) { AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Scrap, FVector2D(Cx + FMath::FRandRange(-30.f, 30.f), Cy + 14.f + FMath::FRandRange(-12.f, 12.f)), FVector2D::ZeroVector, 1.f, 1.f); }
		}
		else
		{
			for (int32 K = 0; K < 9; ++K) { AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Adn, FVector2D(Cx + FMath::FRandRange(-40.f, 40.f), Cy + FMath::FRandRange(-25.f, 25.f)), FVector2D::ZeroVector, 3.f); }
			for (int32 K = 0; K < 2; ++K) { AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Energy, FVector2D(Cx + (K ? 24.f : -24.f), Cy + 24.f), FVector2D::ZeroVector, 1.f, 1.f); }
		}
	}
	AmbushT = -1.f;
	if (CardMesh)
	{
		const bool bCard = R == TEXT("SECURITE") && !C.bCardTaken;
		CardMesh->SetVisibility(bCard);
		if (bCard) { CardMesh->SetWorldLocation(Room->RoomPxToWorld(FVector2D(C.Grid.W * 8.f, C.Grid.H * 8.f), 40.f)); }
	}

	// Entrée : par le C.O.M (centre + 40 px), sinon devant la porte d'arrivée, sinon la première porte.
	uint8 Side = (Entry && (C.Doors & Entry)) ? Entry : 0;
	if (!Side) { for (uint8 D : DoorOrder) { if (C.Doors & D) { Side = D; break; } } }
	const FVector2D E = bFromCom ? FVector2D(C.Grid.W * 8.f, C.Grid.H * 8.f + 40.f) : Room->DoorCenterPx(Side);
	if (AEONCharacter* J = GetJira())
	{
		const float HalfH = J->GetSimpleCollisionHalfHeight();
		J->ResetForRoom(Room->RoomPxToWorld(E, HalfH + 2.f));
		UE_LOG(LogEON, Display, TEXT("EON salle %s (%d,%d) : Jira en %s, salle en %s"), *C.Role.ToString(), C.Pos.X, C.Pos.Y,
			*J->GetActorLocation().ToCompactString(), *Room->GetActorLocation().ToCompactString());
		if (APlayerController* PC = Cast<APlayerController>(J->GetController()))
		{
			if (PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); }
		}
	}
	const bool bCor = C.Shape == TEXT("corH") || C.Shape == TEXT("corV");
	if (R == TEXT("ASCENSEUR")) { ShowBanner(TEXT("Retourne au C.O.M"), FLinearColor(FColor(62, 242, 224))); }   // le gardien : jalon 7
	else { ShowBanner(RoleName(R) + (bCor ? TEXT(" // Couloir") : TEXT("")), FLinearColor(bLocked ? FColor(255, 61, 85) : FColor(216, 212, 240))); }
	OnRoomLoaded.Broadcast(this);
}

void AEONFloorManager::StartTrans(TFunction<void()> Fn)
{
	if (TransT > 0.f) { return; }
	TransT = 0.36f;
	bTransDone = false;
	TransFn = MoveTemp(Fn);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager) { PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.18f, FLinearColor(0.027f, 0.024f, 0.06f), false, true); }
	}
}

void AEONFloorManager::ShowBanner(const FString& Text, FLinearColor Color)
{
	BannerText = Text;
	BannerColor = Color;
	BannerT = 1.8f;
}

void AEONFloorManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BannerT = FMath::Max(0.f, BannerT - DeltaSeconds);
	// eon.debut : commandes de test différées (le premier étage et Jira existent alors).
	static float DebutT = 0.f;
	if (DebutT >= 0.f && (DebutT += DeltaSeconds) > 2.f)
	{
		DebutT = -1.f;
		TArray<FString> Cmds;
		CVarEONDebut.GetValueOnGameThread().ParseIntoArray(Cmds, TEXT(";"));
		for (const FString& Cm : Cmds) { if (GEngine) { GEngine->Exec(GetWorld(), *Cm.TrimStartAndEnd()); } }
	}
	// Trace de diagnostic (eon.trace 1) : position de Jira et de la caméra toutes les 2 s.
	static float TraceT = 0.f;
	if (CVarEONTrace.GetValueOnGameThread() > 0 && (TraceT -= DeltaSeconds) <= 0.f)
	{
		TraceT = 2.f;
		if (GEngine) { GEngine->Exec(GetWorld(), TEXT("eon.ou")); }
	}
	// Transition : à mi-course (0,18 s) on charge la salle, puis on rouvre.
	if (TransT > 0.f)
	{
		TransT -= DeltaSeconds;
		if (TransT <= 0.18f && !bTransDone)
		{
			bTransDone = true;
			TFunction<void()> Fn = MoveTemp(TransFn);
			TransFn = nullptr;
			if (Fn) { Fn(); }
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (PC->PlayerCameraManager) { PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.18f, FLinearColor(0.027f, 0.024f, 0.06f), false, false); }
			}
		}
		return;
	}
	// Le C.O.M descend : secousses 1,7 s, puis fondu et étage suivant (comTravel).
	if (ComTravelT >= 0.f)
	{
		ComTravelT += DeltaSeconds;
		if (FMath::FRand() < DeltaSeconds * 14.f) { if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->Shake(0.12f + ComTravelT * 0.12f); } }
		if (ComTravelT > 1.7f && !bComGo) { bComGo = true; StartTrans([this]() { ComTravelT = -1.f; StartFloor(); }); }
		return;
	}
	AEONCharacter* J = GetJira();
	FEONFloorCell* CM = CellAt(Cur);
	if (!J || !CM || !Room || J->IsDead()) { return; }
	const FVector2D P = Room->WorldToRoomPx(J->GetActorLocation());
	// Carte rouge : ramassage à moins de 10 px, puis embuscade (la salle se verrouille, 2 vagues).
	if (CM->Role == TEXT("SECURITE") && !CM->bCardTaken && !J->IsFallingIntoVoid() && FVector2D::Distance(P, GetCardPx()) < 10.f)
	{
		CM->bCardTaken = true;
		bRedCard = true;
		Room->SetRedDoor(0);
		if (CardMesh) { CardMesh->SetVisibility(false); }
		ShowBanner(TEXT("Carte rouge"), FLinearColor(FColor(255, 61, 85)));
		AmbushT = 1.4f;   // le temps du dialogue du proto (« ... Ce silence ne me plaît pas. »)
	}
	if (AmbushT > 0.f && (AmbushT -= DeltaSeconds) <= 0.f)
	{
		CM->bCleared = false;
		Room->SetLocked(true);
		if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->StartCombat(2, 0.35f); D->Shake(0.4f); }
		ShowBanner(TEXT("Embuscade !"), FLinearColor(FColor(255, 61, 85)));
	}
	// Chair : se rétracte en 1,6 s quand le gardien est neutralisé ; sinon, s'en approcher (24 px) = refus.
	if (CM->Infest.Num())
	{
		if (bFloorDone)
		{
			CM->InfestK = FMath::Max(0.f, CM->InfestK - DeltaSeconds / 1.6f);
			Room->SetInfestK(CM->InfestK);
			if (CM->InfestK <= 0.f) { CM->Infest.Reset(); Room->SetInfest(CM->Infest, 0.f); }
		}
		else if ((InfBumpT -= DeltaSeconds) <= 0.f)
		{
			for (const int32 I : CM->Infest)
			{
				const FVector2D Tp((I % CM->Grid.W) * 16.f + 8.f, (I / CM->Grid.W) * 16.f + 8.f);
				if (FVector2D::Distance(P, Tp) < 24.f)
				{
					InfBumpT = 3.f;
					if (!bInfHint)
					{
						bInfHint = true;
						ShowBanner(CM->Role == TEXT("DEPART") ? TEXT("IA : La passerelle est envahie. Rien ne passe par la.") : TEXT("Jira : La chair a mure cette porte. Elle bat... comme un coeur."), FLinearColor(FColor(200, 74, 122)));
					}
					else { ShowBanner(TEXT("La mutation bloque le passage"), FLinearColor(FColor(255, 61, 85))); }
					break;
				}
			}
		}
	}
	// Porte rouge : à moins de 20 px, refus (le premier, Jira le dit).
	RedBumpT -= DeltaSeconds;
	if (Room->GetRedDoor() && RedBumpT <= 0.f && FVector2D::Distance(P, Room->DoorCenterPx(Room->GetRedDoor())) < 20.f)
	{
		RedBumpT = 2.5f;
		if (!bRedHint) { bRedHint = true; ShowBanner(TEXT("Verrou de securite rouge. Il me faut la carte."), FLinearColor(FColor(255, 61, 85))); }
		else { ShowBanner(TEXT("Carte rouge requise"), FLinearColor(FColor(255, 61, 85))); }
	}
	// Sortie par une porte : Jira à moins de 10 px du bord, salle ouverte, pas en chute.
	const FEONFloorCell* C = CM;
	if (Room->IsLocked() || J->IsFallingIntoVoid()) { return; }
	const float RW = C->Grid.W * 16.f, RH = C->Grid.H * 16.f;
	uint8 Dir = 0;
	if (P.Y < 10.f && (C->Doors & EONDoor::N)) { Dir = EONDoor::N; }
	else if (P.Y > RH - 10.f && (C->Doors & EONDoor::S)) { Dir = EONDoor::S; }
	else if (P.X < 10.f && (C->Doors & EONDoor::W)) { Dir = EONDoor::W; }
	else if (P.X > RW - 10.f && (C->Doors & EONDoor::E)) { Dir = EONDoor::E; }
	if (!Dir || Dir == Room->GetRedDoor()) { return; }
	const FIntPoint N = Cur + EONDoor::Dir(Dir);
	if (IsInfestEdge(Cur, N)) { return; }
	if (!CellAt(N)) { return; }
	StartTrans([this, N, Dir]() { EnterCell(N, Dir, false); });
}

bool AEONFloorManager::AutoExit(uint8 Dir)
{
	const FEONFloorCell* C = CellAt(Cur);
	if (!C || TransT > 0.f || ComTravelT >= 0.f || AmbushT > 0.f) { return false; }
	auto Blocked = [this](const FIntPoint& From, const FIntPoint& Q) { const FEONFloorCell* N = CellAt(Q); return !N || (N->Role == TEXT("ASCENSEUR") && !bRedCard && FloorN != 4) || IsInfestEdge(From, Q); };
	if (!Dir)
	{
		// Objectif de l'étage : la carte rouge, puis le gardien, puis le levier du C.O.M.
		auto Goal = [this](const FEONFloorCell& N)
		{
			if (!bRedCard && FloorN != 4) { return N.Role == TEXT("SECURITE") && !N.bCardTaken; }
			if (!bFloorDone) { return N.Role == TEXT("ASCENSEUR"); }
			return N.Role == TEXT("DEPART");
		};
		if (Goal(*C))
		{
			if (C->Role == TEXT("DEPART") && bFloorDone) { return TryInteract(); }
			return false;   // la carte : le pilote va la chercher
		}
		// Parcours en largeur : d'abord une case jamais visitée sur le chemin si elle est voisine, sinon le plus court vers l'objectif.
		TMap<FIntPoint, uint8> First;
		TArray<FIntPoint> Q;
		First.Add(Cur, 0);
		TArray<uint8> Any;
		for (uint8 D : DoorOrder)
		{
			const FIntPoint P = Cur + EONDoor::Dir(D);
			if (!(C->Doors & D) || Blocked(Cur, P)) { continue; }
			Any.Add(D); First.Add(P, D); Q.Add(P);
		}
		if (!Any.Num()) { return false; }
		Dir = Any[Rng.RandRange(0, Any.Num() - 1)];
		for (int32 I = 0; I < Q.Num(); ++I)
		{
			const FEONFloorCell* QC = CellAt(Q[I]);
			if (Goal(*QC)) { Dir = First[Q[I]]; break; }
			for (uint8 D : DoorOrder)
			{
				const FIntPoint P = Q[I] + EONDoor::Dir(D);
				if ((QC->Doors & D) && !Blocked(Q[I], P) && !First.Contains(P)) { First.Add(P, First[Q[I]]); Q.Add(P); }
			}
		}
	}
	if (!(C->Doors & Dir)) { return false; }
	const FIntPoint N = Cur + EONDoor::Dir(Dir);
	if (Blocked(Cur, N)) { return false; }
	if (Room) { Room->SetLocked(false); }
	StartTrans([this, N, Dir]() { EnterCell(N, Dir, false); });
	return true;
}

bool AEONFloorManager::IsInfestEdge(const FIntPoint& A, const FIntPoint& B) const
{
	return bInfest && !bFloorDone && ((A == Start && B == Boss) || (A == Boss && B == Start));
}

void AEONFloorManager::LoadTemplates()
{
	Templates.Reset();
	IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	// En jeu lancé depuis l'éditeur, le registre n'a pas toujours fini son scan : on scanne le dossier tout de suite.
	AR.ScanPathsSynchronous({ TEXT("/Game/EON/Salles") }, false);
	TArray<FAssetData> As;
	AR.GetAssetsByPath(FName(TEXT("/Game/EON/Salles")), As, true);
	for (const FAssetData& A : As)
	{
		UClass* Cl = LoadObject<UClass>(nullptr, *(A.GetObjectPathString() + TEXT("_C")));
		if (!Cl || !Cl->IsChildOf(AEONTileRoom::StaticClass())) { continue; }
		const AEONTileRoom* D = Cl->GetDefaultObject<AEONTileRoom>();
		Templates.Add(Cl);
		UE_LOG(LogEON, Display, TEXT("Salle dessinée : %s (%s %s, %d lignes)"), *Cl->GetName(), *D->RoomRole.ToString(), *D->Shape.ToString(), D->Layout.Num());
	}
}

TSubclassOf<AEONTileRoom> AEONFloorManager::PickTemplate(FName InRole, FName InShape)
{
	float Tot = 0.f;
	for (const TSubclassOf<AEONTileRoom>& Cl : Templates) { const AEONTileRoom* D = Cl->GetDefaultObject<AEONTileRoom>(); if (D->CanServe(InRole, InShape)) { Tot += D->Weight; } }
	if (Tot <= 0.f) { return nullptr; }
	float R = Rng.GetFraction() * Tot;
	for (const TSubclassOf<AEONTileRoom>& Cl : Templates)
	{
		const AEONTileRoom* D = Cl->GetDefaultObject<AEONTileRoom>();
		if (!D->CanServe(InRole, InShape)) { continue; }
		R -= D->Weight;
		if (R <= 0.f) { return Cl; }
	}
	return nullptr;
}

bool AEONFloorManager::GoToRole(FName InRole)
{
	for (const TPair<FIntPoint, FEONFloorCell>& KV : Cells)
	{
		if (KV.Value.Role == InRole) { EnterCell(KV.Key, 0, false); return true; }
	}
	return false;
}

void AEONFloorManager::OnPedestalTaken(AEONPedestal* P, bool bPaid)
{
	FEONFloorCell* C = CellAt(Cur);
	if (!C || !P) { return; }
	if (!bPaid) { C->PedItems.Reset(); C->PedPrice.Reset(); C->PedPx.Reset(); return; }
	// Chez le marchand : seul le socle acheté disparaît (repéré par sa position).
	for (int32 I = 0; I < C->PedPx.Num(); ++I)
	{
		if (C->PedPx[I].Equals(P->Px, 0.5f)) { C->PedItems.RemoveAt(I); C->PedPrice.RemoveAt(I); C->PedPx.RemoveAt(I); break; }
	}
}

bool AEONFloorManager::HasCardInRoom() const
{
	const FEONFloorCell* C = CellAt(Cur);
	return C && C->Role == TEXT("SECURITE") && !C->bCardTaken;
}

FVector2D AEONFloorManager::GetCardPx() const
{
	const FEONFloorCell* C = CellAt(Cur);
	return C ? FVector2D(C->Grid.W * 8.f, C->Grid.H * 8.f) : FVector2D::ZeroVector;
}

FString AEONFloorManager::GuardDir() const
{
	const FIntPoint D = Boss - Start;
	return D.Y < 0 ? TEXT("nord") : D.Y > 0 ? TEXT("sud") : D.X < 0 ? TEXT("ouest") : TEXT("est");
}

bool AEONFloorManager::IsNearComTerminal() const
{
	const FEONFloorCell* C = CellAt(Cur);
	const AEONCharacter* J = GetJira();
	if (!C || !J || !Room || C->Role != TEXT("DEPART")) { return false; }
	return FVector2D::Distance(Room->WorldToRoomPx(J->GetActorLocation()), FVector2D(C->Grid.W * 8.f, C->Grid.H * 8.f)) < 56.f;
}

bool AEONFloorManager::TryInteract()
{
	if (!IsNearComTerminal() || ComTravelT >= 0.f || TransT > 0.f) { return false; }
	if (bFloorDone)
	{
		// pullCOM : le levier de l'onglet Descente.
		ComTravelT = 0.f;
		bComGo = false;
		if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->Shake(0.5f); }
		ShowBanner(TEXT("Le C.O.M descend"), FLinearColor(FColor(62, 242, 224)));
		return true;
	}
	ShowBanner(FString::Printf(TEXT("Verrouille - gardien : %s du C.O.M - carte rouge %s"), *GuardDir(), bRedCard ? TEXT("OK") : TEXT("a trouver")), FLinearColor(FColor(140, 134, 180)));
	return false;
}
