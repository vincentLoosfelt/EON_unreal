#include "EONFloorManager.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONTileRoom.h"
#include "EONCombatDirector.h"
#include "EONBoss.h"
#include "EONEnemy.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONUI.h"
#include "EONPedestal.h"
#include "EONPickup.h"
#include "EONSfx.h"
#include "EONSlidingDoor.h"
#include "EONTrainingDummy.h"
#include "EONWakePod.h"
#include "EONNpc.h"
#include "EONItems.h"
#include "EONChampForce.h"
#include "EONElectricArc.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMeshActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "EONTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Containers/Ticker.h"
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
			{ TEXT("CORRIDOR"), TEXT("Le corridor") }, { TEXT("FAILLE"), TEXT("La faille") }, { TEXT("TRESOR"), TEXT("Tresor") },
			{ TEXT("IRRUPTION"), TEXT("Irruption") }, { TEXT("CARTO"), TEXT("Cartographe") } };
		const FString* S = N.Find(Role);
		return S ? *S : Role.ToString();
	}


	FAutoConsoleCommandWithWorldAndArgs GEONCmdEtage(TEXT("eon.etage"), TEXT("eon.etage : étage suivant (nouvel étage généré, entrée par le C.O.M)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AEONFloorManager* F = AEONFloorManager::Get(World)) { F->StartFloor(); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdQuete(TEXT("eon.quete"), TEXT("eon.quete ulysse 0|1 | gamine 0|1 | peluche | poche N | log : quêtes du jalon 9 (tests ; modifie la sauvegarde)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AEONFloorManager* F = AEONFloorManager::Get(World)) { F->TestQuete(Args); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdKibo(TEXT("eon.kibo"), TEXT("eon.kibo reveil|passerelle|quai : va dans cette salle du Kibo (nouvelle run si besoin)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const FString A = Args.Num() ? Args[0].ToLower() : FString(TEXT("reveil"));
			if (AEONFloorManager* F = AEONFloorManager::Get(World)) { F->GoKibo(A.StartsWith(TEXT("pass")) ? 2 : A.StartsWith(TEXT("quai")) ? 3 : 1); }
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdVue(TEXT("eon.vue"), TEXT("eon.vue x y z vx vy vz [fov] | off : caméra fixe à (x,y,z) du centre du C.O.M, visant (vx,vy,vz) ; outil de tournage"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONFloorManager* F = AEONFloorManager::Get(World);
			if (!F) { return; }
			if (Args.Num() < 6) { F->bDevView = false; return; }
			auto V = [&Args](int32 I) { return FCString::Atof(*Args[I]); };
			F->SetDevView(FVector(V(0), V(1), V(2)), FVector(V(3), V(4), V(5)), Args.Num() > 6 ? V(6) : 60.f);
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
	FAutoConsoleCommandWithWorldAndArgs GEONCmdPlace(TEXT("eon.place"), TEXT("eon.place X Y : pose Jira en (X, Y) px de la salle ; eon.place console : sur la console de confinement de la Faille (tests)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONFloorManager* F = AEONFloorManager::Get(World);
			AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			if (!F || !J || !F->GetRoom() || !Args.Num()) { return; }
			FVector2D Px;
			if (Args[0].Equals(TEXT("console"), ESearchCase::IgnoreCase))
			{
				const FEONFloorCell* C = F->GetCells().Find(F->GetCurrentCell());
				if (!C || !C->RiftTiles.Num()) { return; }
				Px = C->ConfPx;
			}
			else if (Args.Num() >= 2) { Px = FVector2D(FCString::Atof(*Args[0]), FCString::Atof(*Args[1])); }
			else { return; }
			J->SetActorLocation(F->GetRoom()->RoomPxToWorld(Px, J->GetSimpleCollisionHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdForme(TEXT("eon.forme"), TEXT("eon.forme FORME : va dans la première salle de cette forme (std, carree, large, small, corH, corV) ; s'il n'y en a pas, une salle Combat ou Tension prend cette forme"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AEONFloorManager* F = AEONFloorManager::Get(World);
			if (F && Args.Num() && !F->GoToShape(FName(*Args[0]))) { UE_LOG(LogEON, Warning, TEXT("eon.forme : pas de salle %s sur cet étage"), *Args[0]); }
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
	FAutoConsoleCommandWithWorldAndArgs GEONCmdApres(TEXT("eon.apres"), TEXT("eon.apres SECONDES CMD... : lance CMD plus tard dans la même partie (tests avant / après au même endroit)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() < 2) { return; }
			const float Delay = FMath::Max(0.01f, FCString::Atof(*Args[0]));
			const FString Cmd = FString::Join(TArrayView<const FString>(Args).RightChop(1), TEXT(" "));
			// Temps réel : la commande part même si le jeu est en pause (dialogue, terminal, choix de niveau).
			TWeakObjectPtr<UWorld> W(World);
			double* Left = new double(Delay);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([W, Cmd, Left](float Dt)
			{
				if (!W.IsValid()) { delete Left; return false; }
				if ((*Left -= Dt) > 0.0) { return true; }
				delete Left;
				UE_LOG(LogEON, Display, TEXT("eon.apres : %s"), *Cmd);
				// Par le contrôleur : les commandes du viewport (show, viewmode...) passent aussi.
				if (APlayerController* PC = W->GetFirstPlayerController()) { PC->ConsoleCommand(Cmd); }
				else if (GEngine) { GEngine->Exec(W.Get(), *Cmd); }
				return false;
			}));
		}));
	FAutoConsoleCommandWithWorldAndArgs GEONCmdCache(TEXT("eon.cache"), TEXT("eon.cache NOM [0|1] : masque (0, défaut) ou remontre (1) l'ISM NOM de toutes les salles (Sol, Murs, Decor, Cuves, Portes, PorteRouge, BordsDuVide, MursHUB, Chair)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || !Args.Num()) { return; }
			const bool bShow = Args.Num() > 1 && FCString::Atoi(*Args[1]) != 0;
			int32 N = 0;
			for (TActorIterator<AEONTileRoom> It(World); It; ++It)
			{
				TArray<UInstancedStaticMeshComponent*> Comps;
				It->GetComponents(Comps);
				for (UInstancedStaticMeshComponent* C : Comps)
				{
					if (C->GetName() == Args[0]) { C->SetVisibility(bShow); ++N; }
				}
			}
			UE_LOG(LogEON, Display, TEXT("eon.cache %s %d : %d composant(s)"), *Args[0], bShow ? 1 : 0, N);
		}));
	TAutoConsoleVariable<FString> CVarEONDebut(TEXT("eon.debut"), TEXT(""), TEXT("eon.debut \"cmd1;cmd2\" : commandes lancées 2 s après le début (tests, ExecCmds trop tôt)"));
	TAutoConsoleVariable<int32> CVarEONTresor(TEXT("eon.tresor"), 0, TEXT("eon.tresor 1 : les trésors sont des stocks piégés (le Dormeur des pièces) ; 2 : des carnages ; 0 : au hasard (tests)"));
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
	// Console de confinement de la Faille (14 x 14 px dans le proto : 0,9 m de côté, 1 m de haut).
	ConfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleConfinement"));
	ConfMesh->SetupAttachment(RootComponent);
	ConfMesh->SetStaticMesh(Cube.Object);
	ConfMesh->SetMobility(EComponentMobility::Movable);
	ConfMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ConfMesh->SetWorldScale3D(FVector(0.875f, 0.875f, 1.f));
	ConfMesh->SetVisibility(false);
	// Irruption : la lampe de Jira dans le noir (58 px) et les yeux de la chose (10 px en rôde, 22 px quand elle charge).
	LampLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LampeJira"));
	LampLight->SetupAttachment(RootComponent);
	LampLight->SetMobility(EComponentMobility::Movable);
	LampLight->SetAttenuationRadius(EON::PxToCm(58.f));
	LampLight->SetIntensity(60000.f);
	LampLight->SetCastShadows(false);
	LampLight->SetVisibility(false);
	EyeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("YeuxChose"));
	EyeLight->SetupAttachment(RootComponent);
	EyeLight->SetMobility(EComponentMobility::Movable);
	EyeLight->SetLightColor(FLinearColor(FColor(255, 61, 85)));
	EyeLight->SetCastShadows(false);
	EyeLight->SetVisibility(false);
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
	if (ConfMesh && CardMaterial)
	{
		ConfMID = UMaterialInstanceDynamic::Create(CardMaterial, this);
		ConfMesh->SetMaterial(0, ConfMID);
	}
	Super::BeginPlay();
	Rng.Initialize(FPlatformTime::Cycles());
	AEONPedestal::OnTaken.AddUObject(this, &AEONFloorManager::OnPedestalTaken);
	LoadTemplates();
	SetupHub();
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
	GetWorldTimerManager().SetTimer(H, this, &AEONFloorManager::StartOrTitle, 0.05f, false);
	// eon.debut : commandes de test lancées 2 s (temps réel) après le début, une seule fois par lancement.
	static bool bDebutDone = false;
	if (!bDebutDone)
	{
		bDebutDone = true;
		TWeakObjectPtr<UWorld> W(GetWorld());
		double* Left = new double(2.0);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([W, Left](float Dt)
		{
			if (!W.IsValid()) { delete Left; return false; }
			if ((*Left -= Dt) > 0.0) { return true; }
			delete Left;
			TArray<FString> Cmds;
			CVarEONDebut.GetValueOnGameThread().ParseIntoArray(Cmds, TEXT(";"));
			for (const FString& Cm : Cmds) { if (GEngine) { GEngine->Exec(W.Get(), *Cm.TrimStartAndEnd()); } }
			return false;
		}));
	}
}

void AEONFloorManager::StartOrTitle()
{
	NewRun();
	// Écran titre : au lancement seulement, s'il existe une progression. Nouvelle partie = tout effacer puis nouvelle run.
	UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	if (MS && !MS->bTitleDone)
	{
		MS->bTitleDone = true;
		if (MS->HasProgress())
		{
			if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->OpenTitle([this]() { NewRun(); }); }
		}
	}
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
	bIntroDone = false;
	bRedHint = bInfHint = false;
	Greeted.Reset();
	bHasDoudou = false;
	MetUlysse = 0;
	if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->OnRunStart(); }
	if (AEONCharacter* J = GetJira()) { J->ResetRun(); }
	// newRun : la run commence au Kibo, C.O.M à quai (floor absent jusqu'au levier). Sans hub 3D : directement l'étage 1.
	if (bHub) { BeginKibo(); EnterTraining(true); } else { StartFloor(); }
}

void AEONFloorManager::BeginKibo()
{
	bDocked = true;
	bFloorDone = true;   // le levier est déjà prêt
	bRedCard = false;
	AmbushT = -1.f;
	ComTravelT = -1.f;
	bComGo = false;
	bInfest = false;
	Cells.Reset();
	Start = Cur = FIntPoint(3, 3);
	Boss = FIntPoint(3, 2);
	AEONPickup::ClearAll(GetWorld(), true);
	AEONPedestal::ClearAll(GetWorld());
	if (CardMesh) { CardMesh->SetVisibility(false); }
	if (ConfMesh) { ConfMesh->SetVisibility(false); }
	RoomWaves = 0; RoomStartT = 0.f;
	if (Room) { Room->SetLocked(false); }
	if (AEONCharacter* J = GetJira()) { J->RefillShield(); }
	// Le quai n'a qu'une porte, au sud, vers la passerelle : les 3 autres passerelles sont retirées et murées.
	SetHubDoorsForced(0);
	for (AEONSlidingDoor* D : HubDoorActors) { if (D && D->GetActorLocation().Y - HubCentre.Y > 300.f) { D->ForceState = -1; } }
	SetDockLayout(true);
}

void AEONFloorManager::LoadDock()
{
	ClearKiboActors();
	Kibo = 3;
	AEONPickup::ClearAll(GetWorld(), true);
	Room->SetLocked(false);
	LoadHub(EONDoor::S, false);
	ShowBanner(TEXT("Kibo - C.O.M a quai"), FLinearColor(FColor(62, 242, 224)));
	UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	UEONSaveGame* Sv = MS ? MS->M() : nullptr;
	if (Sv && !Sv->bDockSeen)
	{
		Sv->bDockSeen = true;
		MS->Save();
		if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("dock")); }
	}
	RoomReadyAt = FPlatformTime::Seconds();
	OnRoomLoaded.Broadcast(this);
	UE_LOG(LogEON, Display, TEXT("EON Kibo : C.O.M a quai, levier pret"));
}

void AEONFloorManager::ClearKiboActors()
{
	for (AActor* A : KiboActors) { if (A) { A->Destroy(); } }
	KiboActors.Reset();
	ClearRoomActors();
	JiraPod = nullptr;
	WakeT = -1.f;
	WakeLift = 0.f;
}

void AEONFloorManager::LoadKiboGrid(const FEONTileGrid& G, uint8 Entry, const FVector2D* At)
{
	if (!Room) { return; }
	ClearKiboActors();
	AEONPickup::ClearAll(GetWorld(), true);
	AEONPedestal::ClearAll(GetWorld());
	if (CardMesh) { CardMesh->SetVisibility(false); }
	if (ConfMesh) { ConfMesh->SetVisibility(false); }
	if (Room->GetClass() != AEONTileRoom::StaticClass())
	{
		const FTransform Xf = Room->GetActorTransform();
		Room->Destroy();
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Room = GetWorld()->SpawnActor<AEONTileRoom>(AEONTileRoom::StaticClass(), Xf, P);
	}
	KiboGrid = G;
	Room->SetGrid(G);
	Room->SetLocked(false);
	Room->SetRedDoor(0);
	Room->SetInfest(TArray<int32>(), 0.f);
	RoomWaves = 0; RoomStartT = 0.f; AmbushT = -1.f;
	if (AEONCharacter* J = GetJira())
	{
		const FVector2D E = At ? *At : Room->DoorCenterPx(Entry);
		J->ResetForRoom(Room->RoomPxToWorld(E, J->GetSimpleCollisionHalfHeight() + 2.f));
		if (APlayerController* PC = Cast<APlayerController>(J->GetController())) { if (PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); } }
	}
	RoomReadyAt = FPlatformTime::Seconds();
	OnRoomLoaded.Broadcast(this);
}

void AEONFloorManager::EnterTraining(bool bCine)
{
	// buildTraining : 36 × 20, 7 cuves (PODS), fossé à dasher, petit trou, porte nord.
	FEONTileGrid G = FEONTileGrid::BuildTraining();
	const FIntPoint Pod(17, 15);   // la cuve de Jira (PODS, st 'jira')
	if (bCine) { for (int32 Y = Pod.Y; Y < Pod.Y + 3; ++Y) { for (int32 X = Pod.X; X < Pod.X + 2; ++X) { G.Set(X, Y, EEONTile::Floor); } } }
	const FVector2D PodPx((Pod.X + 1) * 16.f, (Pod.Y + 1.5f) * 16.f);
	LoadKiboGrid(G, EONDoor::N, bCine ? &PodPx : nullptr);
	Kibo = 1;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Les 4 cibles d'entraînement (dummy, increvables) : trois près des cuves, une au-delà du fossé.
	for (const FVector2D& Px : { FVector2D(96.f, 128.f), FVector2D(160.f, 112.f), FVector2D(224.f, 136.f), FVector2D(504.f, 160.f) })
	{
		if (AActor* A = GetWorld()->SpawnActor<AEONTrainingDummy>(AEONTrainingDummy::StaticClass(), Room->RoomPxToWorld(Px, 0.f), FRotator::ZeroRotator, P)) { KiboActors.Add(A); }
	}
	// Les 7 cuves (PODS du proto) : scellées, brisées, et celle de Jira (pleine pendant la cinématique, ouverte ensuite).
	static const EEONPodState States[] = { EEONPodState::Sealed, EEONPodState::Broken, EEONPodState::Sealed, EEONPodState::Sealed, EEONPodState::Sealed, EEONPodState::Broken, EEONPodState::Jira };
	const TArray<FIntPoint>& Pods = FEONTileGrid::TrainingPods();
	for (int32 I = 0; I < Pods.Num(); ++I)
	{
		const FIntPoint Q = Pods[I];
		AEONWakePod* Pd = GetWorld()->SpawnActor<AEONWakePod>(AEONWakePod::StaticClass(), Room->RoomPxToWorld(FVector2D((Q.X + 1) * 16.f, (Q.Y + 1.5f) * 16.f), 0.f), FRotator::ZeroRotator, P);   // toutes vitre vers la caméra (sud) : vues de dos, les cuves du mur sud cachaient leur contenu
		if (!Pd) { continue; }
		const bool bJira = States[I] == EEONPodState::Jira;
		Pd->Init(bJira && !bCine ? EEONPodState::Open : States[I]);
		KiboActors.Add(Pd);
		if (bJira) { JiraPod = Pd; }
	}
	if (bCine)
	{
		if (JiraPod) { JiraPod->StartWake(); }
		WakeT = 0.f;
		// Jira flotte dans le tube (fond du verre à 130 cm), figée jusqu'à l'ouverture.
		if (AEONCharacter* J = GetJira())
		{
			WakeGroundZ = J->GetActorLocation().Z;
			WakeLift = 130.f;
			J->AddActorWorldOffset(FVector(0.f, 0.f, WakeLift));
			SetJiraFrozen(true);
		}
	}
	// Ulysse, une fois sauvé, se réfugie au Kibo (8 × 16, 16 × 16 + 8) : on lui rend les insignes.
	bUlysseKiboTalked = false;
	if (const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this); MS && MS->M() && MS->M()->bUlysse)
	{
		if (AEONNpc* N = GetWorld()->SpawnActor<AEONNpc>(AEONNpc::StaticClass(), Room->RoomPxToWorld(FVector2D(128.f, 264.f), 0.f), FRotator::ZeroRotator, P))
		{
			N->Setup(22.f, 8.f, FLinearColor(FColor(106, 90, 74)), FLinearColor(FColor(224, 196, 172)));
			KiboActors.Add(N);
			UlysseKibo = N;
		}
	}
	ShowBanner(TEXT("Kibo - Salle de reveil"), FLinearColor(FColor(62, 242, 224)));
	UE_LOG(LogEON, Display, TEXT("EON Kibo : salle de reveil%s"), bCine ? TEXT(" (cinematique)") : TEXT(""));
}

void AEONFloorManager::EnterPasserelle(uint8 Entry)
{
	// buildPasserelle : 12 × 30, un pont de 4 tuiles (x 4–7), du vide de chaque côté, portes nord et sud.
	FEONTileGrid G;
	G.W = 12; G.H = 30; G.Doors = EONDoor::N | EONDoor::S;
	G.T.SetNumZeroed(G.W * G.H);
	for (int32 Y = 0; Y < G.H; ++Y) for (int32 X = 0; X < G.W; ++X)
	{
		const bool bEdge = X == 0 || Y == 0 || X == G.W - 1 || Y == G.H - 1;
		G.T[Y * G.W + X] = (uint8)(bEdge ? EEONTile::Wall : ((X < 4 || X > 7) && Y > 2 && Y < G.H - 3) ? EEONTile::Pit : EEONTile::Floor);
	}
	for (uint8 D : { EONDoor::N, EONDoor::S }) { FIntPoint Dt[2]; FEONTileGrid::DoorTiles(D, G.W, G.H, Dt); for (const FIntPoint& Q : Dt) { G.Set(Q.X, Q.Y, EEONTile::Door); } }
	G.Feat = TEXT("passerelle");
	LoadKiboGrid(G, Entry);
	Kibo = 2;
	ShowBanner(TEXT("Passerelle"), FLinearColor(FColor(62, 242, 224)));
}

void AEONFloorManager::GoKibo(uint8 Which)
{
	if (!bHub) { return; }
	if (!bDocked) { BeginKibo(); }
	if (Which == 2) { EnterPasserelle(EONDoor::S); } else if (Which == 3) { LoadDock(); } else { EnterTraining(false); }
}

void AEONFloorManager::TickKibo(float DeltaSeconds, AEONCharacter* J)
{
	// Ulysse au Kibo : à 26 px, il prend les insignes en poche (une fois par passage).
	if (AEONNpc* U = UlysseKibo.Get(); U && J && Room && WakeT < 0.f && !bUlysseKiboTalked)
	{
		const UEONUISubsystem* UI = UEONUISubsystem::Get(this);
		const FVector2D Jp = Room->WorldToRoomPx(J->GetActorLocation());
		if (UI && !UI->IsBlocking() && !UI->HasPendingDialog() && FVector2D::Distance(Jp, FVector2D(128.f, 264.f)) < 26.f) { bUlysseKiboTalked = true; SayUlysseHub(); }
	}
	// Cinématique de réveil (2,6 s) : la cuve se vidange (0,6 s), s'ouvre (1,7 s), Jira sort, puis le dialogue wake.
	// La cuve (AEONWakePod) joue elle-même la vidange, l'ouverture, la vapeur et les sons.
	if (WakeT >= 0.f)
	{
		const float T0 = WakeT;
		WakeT += DeltaSeconds;
		AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld());
		if (T0 < 1.7f && WakeT >= 1.7f && D) { D->Shake(0.3f); }
		// Le tube est descendu (2,15 s) : Jira est relâchée et saute hors de la cuve, vers le nord (-Y, ~4 m, au-delà de
		// l'ombre du tube vu par la caméra) :
		// vers la caméra (+Y), elle atterrissait derrière le muret sud, à 2,5 m de la cuve, hors de vue (16/11).
		if (T0 < 2.2f && WakeT >= 2.2f)
		{
			SetJiraFrozen(false);
			if (J)
			{
				// La contrainte de plan (sol plat des salles) annule la vitesse verticale : Jira restait à 130 cm, « flottante ».
				// Elle n'était active qu'une partie sur deux, selon l'ordre de BeginPlay (Vincent 16/11).
				bWakePlaneWas = J->GetCharacterMovement()->bConstrainToPlane;
				bWakePlaneOff = true;
				J->GetCharacterMovement()->SetPlaneConstraintEnabled(false);
				J->LaunchCharacter(FVector(0.f, -520.f, 220.f), true, true);
			}
		}
		if (WakeLift > 0.f && WakeT >= 2.2f) { if (J) { WakeLift = FMath::Max(0.f, J->GetActorLocation().Z - WakeGroundZ); } }
		// Le dialogue wake met le jeu en pause : il attend que Jira ait atterri, sinon elle restait figée en l'air
		// (« Jira flotte au démarrage », Vincent 16/11). Au plus 1,5 s d'attente.
		const bool bInAir = J && J->GetCharacterMovement()->IsFalling();
		if (WakeT >= 2.6f && (!bInAir || WakeT >= 4.1f))
		{
			if (bWakePlaneOff && J) { J->GetCharacterMovement()->SetPlaneConstraintEnabled(bWakePlaneWas); }
			bWakePlaneOff = false;
			UE_LOG(LogEON, Display, TEXT("EON_WAKE dialogue : Jira à %.0f cm du sol, en l'air=%d, Jira %s, cuve %s"), J ? J->GetActorLocation().Z - WakeGroundZ : -1.f, bInAir ? 1 : 0, J ? *J->GetActorLocation().ToCompactString() : TEXT("-"), JiraPod ? *JiraPod->GetActorLocation().ToCompactString() : TEXT("-"));
			WakeT = -1.f;
			WakeLift = 0.f;
			if (D) { D->Shake(0.25f); }
			if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("wake")); }
		}
		return;
	}
	if (Kibo == 3)
	{
		// C.O.M à quai : le sas sud mène à la passerelle.
		if (FVector2D::Distance(FVector2D(J->GetActorLocation()), FVector2D(HubDoor[DoorIdx(EONDoor::S)])) < 130.f) { StartTrans([this]() { EnterPasserelle(EONDoor::N); }); }
		return;
	}
	if (!Room || J->IsFallingIntoVoid()) { return; }
	const FVector2D P = Room->WorldToRoomPx(J->GetActorLocation());
	const float RH = KiboGrid.H * 16.f;
	uint8 Dir = 0;
	if (P.Y < 10.f && (KiboGrid.Doors & EONDoor::N)) { Dir = EONDoor::N; }
	else if (P.Y > RH - 10.f && (KiboGrid.Doors & EONDoor::S)) { Dir = EONDoor::S; }
	if (!Dir) { return; }
	// Salle de réveil -> passerelle (par le sud) ; passerelle : sud -> réveil, nord -> C.O.M à quai.
	if (Kibo == 1) { StartTrans([this]() { EnterPasserelle(EONDoor::S); }); }
	else if (Dir == EONDoor::S) { StartTrans([this]() { EnterTraining(false); }); }
	else { StartTrans([this]() { LoadDock(); }); }
}

void AEONFloorManager::SetDockLayout(bool bOn)
{
	// Retour à l'étage : tout ce qui a été retiré revient, les murs ajoutés disparaissent.
	for (AActor* A : DockRemoved) { if (A) { A->SetActorHiddenInGame(false); A->SetActorEnableCollision(true); } }
	for (AActor* A : DockAdded) { if (A) { A->Destroy(); } }
	DockRemoved.Reset();
	DockAdded.Reset();
	if (!bOn || !bHub) { return; }
	// Côté d'un acteur vu du C.O.M (nord = -Y), distance le long de ce côté et écart latéral.
	auto Side = [this](const FVector& P, float& Ax, float& Lat) -> uint8
	{
		const FVector D = P - HubCentre;
		if (FMath::Abs(D.X) > FMath::Abs(D.Y)) { Ax = FMath::Abs(D.X); Lat = FMath::Abs(D.Y); return D.X > 0.f ? EONDoor::E : EONDoor::W; }
		Ax = FMath::Abs(D.Y); Lat = FMath::Abs(D.X); return D.Y > 0.f ? EONDoor::S : EONDoor::N;
	};
	TArray<AStaticMeshActor*> LowWalls, TopWalls;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (A == HubCube || HubFlesh.Contains(A)) { continue; }
		FString Mesh;
		AStaticMeshActor* SA = Cast<AStaticMeshActor>(A);
		if (SA) { if (const UStaticMesh* M = SA->GetStaticMeshComponent()->GetStaticMesh()) { Mesh = M->GetName(); } }
		const bool bSas = A->IsA<AEONSlidingDoor>();
		if (Mesh == TEXT("WallA320"))
		{
			const float Z = A->GetActorLocation().Z;
			if (FMath::Abs(Z - HubCentre.Z) < 20.f) { TopWalls.Add(SA); } else if (FMath::Abs(Z - (HubCentre.Z - 610.f)) < 20.f) { LowWalls.Add(SA); }
			continue;
		}
		// Passerelle : grilles de sol, rambardes, mur percé, sas et son bouchon (cube « Porte_fermee »).
		if (!bSas && Mesh != TEXT("Grille01") && Mesh != TEXT("Barrier_V3_1") && Mesh != TEXT("walldooreno") && Mesh != TEXT("Cube")) { continue; }
		float Ax = 0.f, Lat = 0.f;
		const uint8 D = Side(A->GetActorLocation(), Ax, Lat);
		if (D == EONDoor::S || Ax < 800.f || Lat > 300.f) { continue; }
		A->SetActorHiddenInGame(true);
		A->SetActorEnableCollision(false);
		DockRemoved.Add(A);
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Trous des portes : là où le mur du bas n'a pas de mur au-dessus (2 modules par porte), on pose le même mur.
	for (AStaticMeshActor* Lo : LowWalls)
	{
		const FVector L = Lo->GetActorLocation();
		float Ax = 0.f, Lat = 0.f;
		if (Side(L, Ax, Lat) == EONDoor::S || Lat > 500.f) { continue; }
		bool bTop = false;
		for (const AStaticMeshActor* T : TopWalls) { if (FVector2D::Distance(FVector2D(T->GetActorLocation()), FVector2D(L)) < 10.f) { bTop = true; break; } }
		for (const AActor* T : DockAdded) { if (FVector2D::Distance(FVector2D(T->GetActorLocation()), FVector2D(L)) < 10.f) { bTop = true; break; } }
		if (bTop) { continue; }
		AStaticMeshActor* W = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(L.X, L.Y, HubCentre.Z), Lo->GetActorRotation(), P);
		if (!W) { continue; }
		W->SetMobility(EComponentMobility::Movable);
		UStaticMeshComponent* Src = Lo->GetStaticMeshComponent();
		UStaticMeshComponent* SM = W->GetStaticMeshComponent();
		SM->SetStaticMesh(Src->GetStaticMesh());
		SM->SetWorldScale3D(FVector(Lo->GetActorScale3D().X, Lo->GetActorScale3D().Y, 1.f));
		for (int32 I = 0; I < Src->GetNumMaterials(); ++I) { SM->SetMaterial(I, Src->GetMaterial(I)); }
		SM->SetCollisionProfileName(TEXT("BlockAll"));
		DockAdded.Add(W);
	}
	// Bout des passerelles retirées : une barrière invisible au bord de la plateforme du C.O.M (sinon chute dans le vide).
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (uint8 D : { EONDoor::N, EONDoor::E, EONDoor::W })
	{
		const FIntPoint Dv = EONDoor::Dir(D);
		const float Edge = FVector2D::Distance(FVector2D(HubDoor[DoorIdx(D)]), FVector2D(HubCentre)) - 440.f;   // début des grilles
		const FVector At = HubCentre + FVector(Dv.X, Dv.Y, 0.f) * Edge + FVector(0.f, 0.f, 150.f);
		AStaticMeshActor* B = GetWorld()->SpawnActor<AStaticMeshActor>(At, FRotator::ZeroRotator, P);
		if (!B || !Cube) { continue; }
		B->SetMobility(EComponentMobility::Movable);
		UStaticMeshComponent* SM = B->GetStaticMeshComponent();
		SM->SetStaticMesh(Cube);
		SM->SetWorldScale3D(Dv.X ? FVector(0.2f, 4.f, 3.f) : FVector(4.f, 0.2f, 3.f));
		SM->SetCollisionProfileName(TEXT("BlockAll"));
		B->SetActorHiddenInGame(true);
		DockAdded.Add(B);
	}
	UE_LOG(LogEON, Display, TEXT("EON Kibo : %d pieces de passerelle retirees, %d murs et barrieres poses"), DockRemoved.Num(), DockAdded.Num());
}

void AEONFloorManager::SetComDz(float Dz)
{
	const FVector Dv(0.f, 0.f, Dz - ComDz);
	ComDz = Dz;
	for (int32 I = 0; I < ComMovers.Num(); ++I) { if (ComMovers[I]) { ComMovers[I]->SetActorLocation(ComMoverBase[I] + FVector(0.f, 0.f, Dz)); } }
	AEONElectricArc::ComShift = FVector(0.f, 0.f, Dz);
	if (HubCube) { HubCube->AddActorWorldOffset(Dv); }
	if (AEONCharacter* J = GetJira()) { J->AddActorWorldOffset(Dv); }
}

void AEONFloorManager::SetJiraFrozen(bool bFrozen)
{
	AEONCharacter* J = GetJira();
	if (!J) { return; }
	J->GetCharacterMovement()->StopMovementImmediately();
	J->GetCharacterMovement()->SetMovementMode(bFrozen ? MOVE_None : MOVE_Walking);
}

void AEONFloorManager::SetHubDoorsForced(int32 State)
{
	for (AEONSlidingDoor* D : HubDoorActors) { if (D) { D->ForceState = State; } }
}

void AEONFloorManager::StartFloor()
{
	if (bDocked) { bDocked = false; Kibo = 0; ClearKiboActors(); SetHubDoorsForced(-1); SetDockLayout(false); }
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
	if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->OnFloor(FloorN); }
	// Dialogue d'arrivée : après la descente du C.O.M s'il y en a une.
	if (!bComArrive) { SayFloorIntro(); }
}

void AEONFloorManager::SayFloorIntro()
{
	// iaIntro au premier étage de la run, puis cuves, labos, serres, ou la phrase générique.
	if (UEONUISubsystem* UI = UEONUISubsystem::Get(this))
	{
		if (!bIntroDone) { bIntroDone = true; UI->Say(TEXT("iaIntro")); }
		else if (FloorN == 2) { UI->Say(TEXT("cuves")); }
		else if (FloorN == 3) { UI->Say(TEXT("labos")); }
		else if (FloorN == 4) { UI->Say(TEXT("serres")); }
		else { UI->SayLines({ { TEXT("ia"), TEXT("neutre"), FString::Printf(TEXT("Le C.O.M a atteint l'etage %d. Protocole identique : la carte rouge, le gardien, puis retour au C.O.M."), FloorN) } }); }
	}
}

// ---------------------------------------------------------------- genFloor

void AEONFloorManager::GenFloor()
{
	bMapped = false;
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
		// Chemin vers l'autre entrée du gardien : au moins 3 salles, sans passer par lui (tous les étages : infestation systématique).
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
	// Infestation systématique : la passerelle C.O.M -> gardien est toujours murée. Garde-fou : sans seconde entrée, pas de chair (gardien inaccessible sinon).
	bInfest = FMath::CountBits(Cells[Boss].Doors) >= 2;
	if (!bInfest) { UE_LOG(LogEON, Warning, TEXT("EON étage %d : pas de seconde entrée vers le gardien après 400 essais, infestation retirée"), FloorN); }

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
	MarkFloorPickups();
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
			C->RoomClass = PickTemplate(R, C->Shape, C->Doors);
			if (!C->RoomClass) { C->RoomClass = PickTemplate(G, C->Shape, C->Doors); }
			FString Err;
			TArray<FEONDoorSpec> Specs;
			if (C->RoomClass) { AEONTileRoom::GetClassDoorSpecs(C->RoomClass, Specs); }
			if (!C->RoomClass || !AEONTileRoom::LayoutToGrid(C->RoomClass->GetDefaultObject<AEONTileRoom>()->Layout, C->Doors, C->Grid, &Err, &Specs))
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
				FIntPoint DT[2]; G.GetDoorTiles(D, DT);
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
			else if (R == TEXT("TRESOR") && (CVarEONTresor.GetValueOnGameThread() == 1 || (CVarEONTresor.GetValueOnGameThread() == 0 && Rng.GetFraction() < 0.5f)))
			{
				FName Pick = NAME_None;
				for (const FName& Id : EONItems::Roll(4, J, false)) { if (EONItems::Find(Id)->Cat == TEXT("DISQUETTE")) { Pick = Id; break; } }
				if (Pick.IsNone()) { const TArray<FName> One = EONItems::Roll(1, J, false); if (One.Num()) { Pick = One[0]; } }
				if (!Pick.IsNone()) { C->PedItems.Add(Pick); C->PedPrice.Add(0); C->PedPx.Add(FVector2D(Cx, Cy - 18.f)); }
				C->bStock = true;   // butin « stock » (10 scrap, 2 Données, disquette) ; sinon « carnage » (ADN et énergie)
				C->bTrap = CVarEONTresor.GetValueOnGameThread() == 1 || Rng.GetFraction() < 0.4f;
			}
		}
		if (R == TEXT("FAILLE")) { BuildRift(*C); }
		// Irruption : une salle de cuves (placeVats en Calme), la première brisée et vide, les autres scellées.
		if (R == TEXT("IRRUPTION"))
		{
			PlaceVats(*C);
			for (int32 I = 0; I < C->PodBroken.Num(); ++I) { C->PodBroken[I] = I == 0; }
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
	if (bHub && C.Role == TEXT("DEPART"))
	{
		AEONPedestal::ClearAll(GetWorld());
		if (CardMesh) { CardMesh->SetVisibility(false); }
	if (ConfMesh) { ConfMesh->SetVisibility(false); }
		RoomWaves = 0; RoomStartT = 0.f; AmbushT = -1.f;
		Room->SetLocked(false);
		LoadHub(Entry, bFromCom);
		RoomReadyAt = FPlatformTime::Seconds();
		OnRoomLoaded.Broadcast(this);
		return;
	}
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
	RiftT = -1.f; RiftStage = 0; RiftO2 = 1.f; bConfOn = false;
	ClearRoomActors();
	if (C.Role == TEXT("CARTO") && C.KidSt < 5)
	{
		FActorSpawnParameters Sp;
		Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AEONNpc* N = GetWorld()->SpawnActor<AEONNpc>(AEONNpc::StaticClass(), Room->RoomPxToWorld(KidPxOf(C), 0.f), FRotator::ZeroRotator, Sp))
		{
			// cartographe.png : 16 × 22, veste verte (#5a7a4a), visage (#e0c4ac) sous une casquette ambre.
			N->Setup(20.f, 7.f, FLinearColor(FColor(90, 122, 74)), FLinearColor(FColor(224, 196, 172)));
			N->SetDown(C.KidSt == 3);
			RoomActors.Add(N);
			KidNpc = N;
		}
		KidHurtT = 0.f;
	}
	// Salle Événement : la gamine (étage 2, tant qu'elle n'a pas été rencontrée), sinon Ulysse, sinon un membre d'équipage muté.
	if (C.Role == TEXT("EVENEMENT") && C.EvtKind == 0)
	{
		const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
		const UEONSaveGame* M = MS ? MS->M() : nullptr;
		if (FloorN == 2 && !(M && M->bNewtDone)) { C.EvtKind = 1; }
		else if (!(M && M->bUlysse)) { C.EvtKind = 2; }
		else
		{
			TArray<FString> Miss = CrewMissing();
			Miss.Remove(TEXT("Ex-quartier-maitre"));
			if (Miss.Num()) { C.EvtKind = 3; C.CrewName = Miss[FMath::RandHelper(Miss.Num())]; } else { C.EvtKind = 4; }
		}
	}
	EvtNpc = nullptr;
	if ((C.EvtKind == 1 && C.EvtSt < 3) || C.EvtKind == 2)
	{
		FActorSpawnParameters Sp;
		Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AEONNpc* N = GetWorld()->SpawnActor<AEONNpc>(AEONNpc::StaticClass(), Room->RoomPxToWorld(EvtNpcPx(C), 0.f), FRotator::ZeroRotator, Sp))
		{
			// La gamine (drawNewt) : recroquevillée, cheveux châtains, pull bleu-gris, 10 × 14 px. Ulysse : sprite 12 × 22.
			if (C.EvtKind == 1) { N->Setup(13.f, 8.f, FLinearColor(FColor(90, 106, 138)), FLinearColor(FColor(240, 212, 196))); N->ShakePx = C.EvtSt == 2 ? 0.f : 0.6f; }
			else { N->Setup(22.f, 8.f, FLinearColor(FColor(106, 90, 74)), FLinearColor(FColor(224, 196, 172))); N->ShakePx = C.EvtSt < 2 ? 0.5f : 0.f; }
			RoomActors.Add(N);
			EvtNpc = N;
		}
	}
	// Ramassables de quête : le log de l'étage (à gauche du centre), la peluche (à droite), tant qu'ils ne sont pas pris.
	if (C.bLog && !C.bLogTaken) { AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Log, FreeSpot(C.Grid, -3), FVector2D::ZeroVector, 1.f, 0.f); }
	if (C.bDoudou && !C.bDoudouTaken)
	{
		const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
		if (!(MS && MS->M() && MS->M()->bNewtDone)) { AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Doudou, FreeSpot(C.Grid, 3), FVector2D::ZeroVector, 1.f, 0.f); }
	}
	for (int32 I = 0; I < C.Pods.Num(); ++I)
	{
		FActorSpawnParameters Sp;
		Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FIntPoint Q = C.Pods[I];
		if (AEONWakePod* Pd = GetWorld()->SpawnActor<AEONWakePod>(AEONWakePod::StaticClass(), Room->RoomPxToWorld(FVector2D((Q.X + 1) * 16.f, (Q.Y + 1.5f) * 16.f), 0.f), FRotator::ZeroRotator, Sp))
		{
			Pd->Init(C.PodBroken.IsValidIndex(I) && C.PodBroken[I] ? EEONPodState::Broken : EEONPodState::Sealed);
			RoomActors.Add(Pd);
		}
	}
	if (!C.bCleared)
	{
		if (R == TEXT("COMBAT")) { bLocked = true; RoomWaves = (CombatN < 2 || FloorN == 4) ? 1 : 2; RoomStartT = 0.6f; }
		else if (R == TEXT("TENSION")) { RoomWaves = 1; RoomStartT = 0.4f; }
		else if (R == TEXT("CORRIDOR")) { bLocked = true; RoomWaves = 3; RoomStartT = 0.9f; }
		else if (R == TEXT("FAILLE")) { bLocked = true; RoomWaves = 1; RoomStartT = 0.7f; RiftT = C.RiftTiles.Num() ? 0.f : -1.f; RiftAlarmT = 0.f; RiftHurtT = 0.f; }
		else if (R == TEXT("ASCENSEUR")) { bLocked = true; }    // gardien : la salle se verrouille, il sort de sa cuve (AEONBoss)
		else if (R == TEXT("IRRUPTION")) { bLocked = true; C.IrrSt = 0; IrrT = 0.f; FlickT = 0.f; }
		else if (R == TEXT("CARTO")) { if (C.KidSt == 1) { C.KidSt = 0; } bLocked = C.KidSt == 2; }   // ouvert tant que le gamin n'a pas parlé
		else if (R == TEXT("EVENEMENT"))
		{
			// La gamine : salle libre tant qu'elle n'a pas muté. Ulysse et l'équipage : verrouillée, un mutant élite (posé au premier Tick).
			if (C.EvtKind == 1) { if (C.EvtSt != 3) { C.bCleared = true; } else { bLocked = true; } }
			else { bLocked = true; C.EvtSt = 0; }
		}
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
			AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Donnee, FVector2D(Cx + 30.f, Cy + 20.f), FVector2D::ZeroVector, 2.f, 1.f);
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
	// Console de confinement : visible dès l'entrée (éteinte pendant le combat), sans quoi on ne sait pas où aller.
	if (ConfMesh)
	{
		const bool bConf = R == TEXT("FAILLE") && !C.bRiftDone && C.RiftTiles.Num() > 0;
		ConfMesh->SetVisibility(bConf);
		if (bConf) { ConfMesh->SetWorldLocation(Room->RoomPxToWorld(C.ConfPx, 50.f)); }
		if (ConfMID) { ConfMID->SetVectorParameterValue(TEXT("BC1"), FLinearColor(0.16f, 0.15f, 0.31f)); ConfMID->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.f, 0.f, 0.f)); }
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
	if (R == TEXT("ASCENSEUR")) { if (C.bCleared) { ShowBanner(TEXT("Retourne au C.O.M"), FLinearColor(FColor(62, 242, 224))); } }
	else { ShowBanner(RoleName(R) + (bCor ? TEXT(" // Couloir") : TEXT("")), FLinearColor(bLocked ? FColor(255, 61, 85) : FColor(216, 212, 240))); }
	RoomReadyAt = FPlatformTime::Seconds();
	OnRoomLoaded.Broadcast(this);
	// Le gardien (après le broadcast : le directeur vient de vider la salle).
	if (R == TEXT("ASCENSEUR") && !C.bCleared && FloorN != 4)
	{
		if (!AEONBoss::SpawnInRoom(AEONCombatDirector::Get(GetWorld()), Room, FloorN)) { C.bCleared = true; bFloorDone = true; Room->SetLocked(false); }
	}
	else if (R == TEXT("ASCENSEUR") && !C.bCleared) { C.bCleared = true; bFloorDone = true; Room->SetLocked(false); }   // étage 4 : pas de gardien
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
	UE_LOG(LogEON, Display, TEXT("EON banniere : %s"), *Text);
}

void AEONFloorManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BannerT = FMath::Max(0.f, BannerT - DeltaSeconds);
	// Cube du terminal du hub : rotation lente, s'emballe quand le C.O.M descend.
	if (HubCube) { HubCube->AddActorWorldRotation(FRotator(0.f, (ComTravelT >= 0.f ? 540.f : 45.f) * DeltaSeconds, 0.f)); }
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
		RoomReadyAt = FPlatformTime::Seconds();
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
	// Le C.O.M descend (comTravel), sans mouvement linéaire :
	//  départ  : mise en marche 0,9 s (vibrations qui montent), descente de 32 m dans la colonne en accélérant (ease-in cubique),
	//            fondu au noir (smoothstep) de 2,7 à 3,7 s ; l'étage suivant se charge dans le noir ;
	//  arrivée : 0,25 s de noir, fondu d'ouverture 0,9 s, le C.O.M arrive de 22 m plus haut et freine (ease-out quintique) jusqu'à
	//            la hauteur des passerelles en 3,3 s, puis un petit choc à l'arrêt.
	static constexpr float ComDropOut = 3200.f;   // le C.O.M s'enfonce de 32 m dans la colonne
	static constexpr float ComDropIn = 2200.f;    // et arrive de 22 m plus haut
	auto Fade = [this](float A)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PC->PlayerCameraManager)
			{
				if (A > 0.001f) { PC->PlayerCameraManager->SetManualCameraFade(A, FLinearColor::Black, false); }
				else { PC->PlayerCameraManager->StopCameraFade(); }
			}
		}
	};
	auto Smooth = [](float X) { X = FMath::Clamp(X, 0.f, 1.f); return X * X * (3.f - 2.f * X); };
	if (ComTravelT >= 0.f)
	{
		const float T0 = ComTravelT;
		ComTravelT += DeltaSeconds;
		const float T = ComTravelT;
		AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld());
		// Mise en marche : 3 à-coups de plus en plus forts, puis une vibration douce pendant la descente.
		for (const float Kick : { 0.15f, 0.45f, 0.75f }) { if (D && T0 < Kick && T >= Kick) { D->Shake(0.1f + Kick * 0.25f); } }
		if (T > 0.9f)
		{
			const float K = FMath::Min(1.f, (T - 0.9f) / 2.8f);
			SetComDz(-ComDropOut * K * K * K);
			if (D && FMath::Fmod(T, 0.35f) < DeltaSeconds) { D->Shake(0.06f); }
		}
		Fade(Smooth((T - 2.7f) / 1.f));
		if (T >= 3.7f && !bComGo)
		{
			bComGo = true;
			ComTravelT = -1.f;
			bComArrive = true;
			StartFloor();
			ComDz = 0.f;          // Jira et le terminal viennent d'être reposés à hauteur des passerelles
			SetComDz(ComDropIn);  // le C.O.M (et Jira) repartent d'en haut
			SetJiraFrozen(true);
			ComArrT = 0.f;
			Fade(1.f);
		}
		return;
	}
	if (ComArrT >= 0.f)
	{
		ComArrT += DeltaSeconds;
		Fade(1.f - Smooth((ComArrT - 0.25f) / 0.9f));
		const float K = FMath::Clamp((ComArrT - 0.1f) / 3.2f, 0.f, 1.f);
		SetComDz(ComDropIn * FMath::Pow(1.f - K, 5.f));
		if (K >= 1.f)
		{
			ComArrT = -1.f;
			bComArrive = false;
			SetComDz(0.f);
			Fade(0.f);
			SetJiraFrozen(false);
			if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->Shake(0.3f); }
			SayFloorIntro();
		}
		return;
	}
	AEONCharacter* J = GetJira();
	if (bDocked) { if (J && !J->IsDead()) { TickKibo(DeltaSeconds, J); } return; }
	FEONFloorCell* CM = CellAt(Cur);
	if (!J || !CM || !Room || J->IsDead()) { return; }
	if (bHub && Cur == Start) { TickHub(DeltaSeconds, J); return; }
	const FVector2D P = Room->WorldToRoomPx(J->GetActorLocation());
	if (CM->Role == TEXT("FAILLE") && RiftT >= 0.f && !CM->bRiftDone) { TickRift(DeltaSeconds, J, P, *CM); }
	if (CM->Role == TEXT("IRRUPTION") && !CM->bCleared) { TickIrruption(DeltaSeconds, J, P, *CM); }
	if (CM->Role == TEXT("CARTO")) { TickCarto(DeltaSeconds, J, P, *CM); }
	if (CM->Role == TEXT("EVENEMENT")) { TickEvent(DeltaSeconds, J, P, *CM); }
	// Trésor piégé : la disquette prise, le Dormeur des pièces (Brainless élite) se réveille et la salle se verrouille.
	if (CM->Role == TEXT("TRESOR") && CM->bTrap)
	{
		AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld());
		if (!CM->bSprung && CM->bLootDone && CM->PedItems.Num() == 0 && Dir)
		{
			CM->bSprung = true;
			Room->SetLocked(true);
			EONSfx::Play(this, TEXT("SFX_Proto_roar"), 0.9f);
			Dir->Shake(0.6f);
			if (AEONEnemy* M = Dir->SpawnEnemyAt(TEXT("brainless"), FVector2D((CM->Grid.W >> 1) * 16.f, (CM->Grid.H >> 1) * 16.f + 10.f), true))
			{
				M->DisplayName = TEXT("Dormeur des pieces");
				M->Hp = M->MaxHp = FMath::RoundToFloat(18.f * (1.f + 0.3f * (FloorN - 1)));
				M->R = 7.f;
				M->MassM = 2.f;
			}
			ShowBanner(TEXT("Quelque chose dormait sous les pieces"), FLinearColor(FColor(255, 61, 85)));
		}
		else if (CM->bSprung && Room->IsLocked() && Dir)
		{
			bool bAlive = false;
			for (const AEONEnemy* E : Dir->GetEnemies()) { if (E && !E->bDead) { bAlive = true; } }
			if (!bAlive) { Room->SetLocked(false); EONSfx::Play(this, TEXT("SFX_Proto_door"), 0.8f); }
		}
	}
	else if (ExpBias != 0.f || LampLight->IsVisible()) { ExpBias = 0.f; LampLight->SetVisibility(false); EyeLight->SetVisibility(false); }
	// Carte rouge : ramassage à moins de 10 px, puis embuscade (la salle se verrouille, 2 vagues).
	if (CM->Role == TEXT("SECURITE") && !CM->bCardTaken && !J->IsFallingIntoVoid() && FVector2D::Distance(P, GetCardPx()) < 10.f)
	{
		CM->bCardTaken = true;
		bRedCard = true;
		Room->SetRedDoor(0);
		if (CardMesh) { CardMesh->SetVisibility(false); }
	if (ConfMesh) { ConfMesh->SetVisibility(false); }
		ShowBanner(TEXT("Carte rouge"), FLinearColor(FColor(255, 61, 85)));
		// Dialogue « card », puis l'embuscade.
		if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("card"), [this]() { AmbushT = 0.05f; }); }
		else { AmbushT = 1.4f; }
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
						if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(CM->Role == TEXT("DEPART") ? TEXT("infestCom") : TEXT("infestBoss")); }
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
		if (!bRedHint) { bRedHint = true; if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("redDoor")); } }
		else { ShowBanner(TEXT("Carte rouge requise"), FLinearColor(FColor(255, 61, 85))); }
	}
	// Marchand : il salue à la première approche (moins de 80 px de son comptoir).
	if (CM->Role == TEXT("MARCHAND") && !Greeted.Contains(Cur) && FVector2D::Distance(P, FVector2D(CM->Grid.W * 8.f, 4 * 16.f + 10.f)) < 80.f)
	{
		Greeted.Add(Cur);
		if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("marchand")); }
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
	// Kibo (eon.go N|S) : Jira est posée sur le seuil de la porte, la sortie se fait comme en marchant.
	if (bDocked)
	{
		AEONCharacter* J = GetJira();
		if (!J || !Room || TransT > 0.f || WakeT >= 0.f) { return false; }
		if (Kibo == 3) { J->ResetForRoom(HubDoor[DoorIdx(EONDoor::S)] + FVector(0.f, -60.f, J->GetSimpleCollisionHalfHeight() + 2.f)); return true; }
		const uint8 D = Dir ? Dir : EONDoor::N;
		if (!(KiboGrid.Doors & D)) { return false; }
		FVector2D Px = Room->DoorCenterPx(D);
		Px.Y = D == EONDoor::N ? 6.f : KiboGrid.H * 16.f - 6.f;
		J->ResetForRoom(Room->RoomPxToWorld(Px, J->GetSimpleCollisionHalfHeight() + 2.f));
		return true;
	}
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

TSubclassOf<AEONTileRoom> AEONFloorManager::PickTemplate(FName InRole, FName InShape, uint8 Doors)
{
	float Tot = 0.f;
	for (const TSubclassOf<AEONTileRoom>& Cl : Templates) { const AEONTileRoom* D = Cl->GetDefaultObject<AEONTileRoom>(); if (D->CanServe(InRole, InShape, Doors)) { Tot += D->Weight; } }
	if (Tot <= 0.f) { return nullptr; }
	float R = Rng.GetFraction() * Tot;
	for (const TSubclassOf<AEONTileRoom>& Cl : Templates)
	{
		const AEONTileRoom* D = Cl->GetDefaultObject<AEONTileRoom>();
		if (!D->CanServe(InRole, InShape, Doors)) { continue; }
		R -= D->Weight;
		if (R <= 0.f) { return Cl; }
	}
	return nullptr;
}

void AEONFloorManager::OnBossDefeated(const FVector2D& BossPx)
{
	// Fin du gardien : la salle se libère, la chair se rétracte (Tick), 2 objets au choix sur des socles (on en prend un).
	FEONFloorCell* C = CellAt(Cur);
	if (!C) { return; }
	C->bCleared = true;
	bFloorDone = true;
	if (Room) { Room->SetLocked(false); }
	const AEONCharacter* J = GetJira();
	const TArray<FName> It = EONItems::Roll(2, J, false);
	C->PedItems.Reset(); C->PedPrice.Reset(); C->PedPx.Reset();
	for (int32 I = 0; I < It.Num(); ++I)
	{
		const FVector2D Px(BossPx.X + (I ? 28.f : -28.f), BossPx.Y);
		C->PedItems.Add(It[I]); C->PedPrice.Add(0); C->PedPx.Add(Px);
		AEONPedestal::Spawn(GetWorld(), Room, It[I], Px, 0, I);
	}
	ShowBanner(TEXT("La chair se retracte, retourne au C.O.M"), FLinearColor(FColor(255, 194, 61)));
	// META.st.boss[étage], bossCob ; le premier gardien vaincu ouvre la cuve du Cobaye (bannière 2,2 s plus tard).
	if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this); MS && MS->OnBossDefeated(FloorN))
	{
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]() { ShowBanner(TEXT("Une cuve s'est ouverte au Kibo"), FLinearColor(FColor(255, 61, 85))); }), 2.2f, false);
	}
	UE_LOG(LogEON, Display, TEXT("EON gardien vaincu : étage %d terminé, %d objets au choix"), FloorN, It.Num());
}

bool AEONFloorManager::GoToShape(FName InShape)
{
	if (bDocked) { StartFloor(); }
	for (const TPair<FIntPoint, FEONFloorCell>& KV : Cells)
	{
		if (KV.Value.Shape == InShape && KV.Key != Start) { EnterCell(KV.Key, 0, false); return true; }
	}
	// Aucune : on prend une salle Combat ou Tension pas encore construite et on lui donne cette forme (test).
	for (TPair<FIntPoint, FEONFloorCell>& KV : Cells)
	{
		FEONFloorCell& C = KV.Value;
		if (!C.bBuilt && (C.Role == TEXT("COMBAT") || C.Role == TEXT("TENSION")))
		{
			UE_LOG(LogEON, Display, TEXT("eon.forme : salle %s (%d,%d) passée de %s à %s"), *C.Role.ToString(), KV.Key.X, KV.Key.Y, *C.Shape.ToString(), *InShape.ToString());
			C.Shape = InShape;
			EnterCell(KV.Key, 0, false);
			return true;
		}
	}
	return false;
}

bool AEONFloorManager::GoToRole(FName InRole)
{
	if (bDocked) { StartFloor(); }
	for (const TPair<FIntPoint, FEONFloorCell>& KV : Cells)
	{
		if (KV.Value.Role == InRole) { EnterCell(KV.Key, 0, false); return true; }
	}
	// Events tirés au hasard (Irruption 50 %) : s'il n'y en a pas, une salle Combat ou Tension pas encore construite le devient (test).
	if (InRole == TEXT("IRRUPTION") || InRole == TEXT("CARTO"))
	{
		for (TPair<FIntPoint, FEONFloorCell>& KV : Cells)
		{
			FEONFloorCell& C = KV.Value;
			if (!C.bBuilt && (C.Role == TEXT("COMBAT") || C.Role == TEXT("TENSION")))
			{
				C.Role = InRole;
				C.Shape = TEXT("std");
				C.RoomClass = nullptr;
				EnterCell(KV.Key, 0, false);
				return true;
			}
		}
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
	if (bDocked) { return Kibo == 3 && bHub && J && HubCube && FVector2D::Distance(FVector2D(J->GetActorLocation()), FVector2D(HubCube->GetActorLocation())) < 500.f; }
	if (!C || !J || !Room || C->Role != TEXT("DEPART")) { return false; }
	if (bHub) { return HubCube && FVector2D::Distance(FVector2D(J->GetActorLocation()), FVector2D(HubCube->GetActorLocation())) < 500.f; }   // zone du terminal : 5 m de rayon
	return FVector2D::Distance(Room->WorldToRoomPx(J->GetActorLocation()), FVector2D(C->Grid.W * 8.f, C->Grid.H * 8.f)) < 56.f;
}

bool AEONFloorManager::TryInteract()
{
	if (Room && !bDocked && !(bHub && Cur == Start))
	{
		if (const AEONCharacter* J = GetJira(); J && Room->TryOpenInnerDoor(Room->WorldToRoomPx(J->GetActorLocation()), 40.f)) { return true; }
	}
	if (!IsNearComTerminal() || ComTravelT >= 0.f || TransT > 0.f) { return false; }
	// openCOM : le terminal (onglet Descente si le levier est prêt).
	if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->OpenCom(); return true; }
	return PullLever();
}

bool AEONFloorManager::PullLever()
{
	if (!bFloorDone || ComTravelT >= 0.f) { return false; }
	ComTravelT = 0.f;
	bComGo = false;
	ComDz = 0.f;
	SetJiraFrozen(true);
	if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->Shake(0.5f); }
	ShowBanner(TEXT("Le C.O.M descend"), FLinearColor(FColor(62, 242, 224)));
	return true;
}

bool AEONFloorManager::IsDocked() const
{
	return bDocked && Kibo == 3;
}

// ---------------------------------------------------------------- hub 3D (L_HUB) comme salle du C.O.M

void AEONFloorManager::SetupHub()
{
	const AActor* Com = nullptr;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const UStaticMesh* M = It->GetStaticMeshComponent() ? It->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
		if (M && M->GetName().StartsWith(TEXT("COMv1"))) { Com = *It; break; }
	}
	if (!Com) { return; }
	HubCentre = Com->GetActorLocation();
	// Ce qui descend avec le C.O.M : son maillage (dôme compris) et le champ de force.
	ComMovers.Reset(); ComMoverBase.Reset();
	if (AStaticMeshActor* CA = const_cast<AStaticMeshActor*>(Cast<AStaticMeshActor>(Com))) { CA->SetMobility(EComponentMobility::Movable); ComMovers.Add(CA); ComMoverBase.Add(CA->GetActorLocation()); }
	for (TActorIterator<AEONChampForce> It(GetWorld()); It; ++It) { ComMovers.Add(*It); ComMoverBase.Add(It->GetActorLocation()); }
	AEONElectricArc::ComCentre = HubCentre;
	AEONElectricArc::ComShift = FVector::ZeroVector;
	// Les 4 sas : chacun rangé selon sa direction depuis le C.O.M (nord = -Y, comme l'écran).
	int32 Found = 0;
	for (TActorIterator<AEONSlidingDoor> It(GetWorld()); It; ++It)
	{
		const FVector D = It->GetActorLocation() - HubCentre;
		const uint8 Dir = FMath::Abs(D.X) > FMath::Abs(D.Y) ? (D.X > 0.f ? EONDoor::E : EONDoor::W) : (D.Y > 0.f ? EONDoor::S : EONDoor::N);
		HubDoor[DoorIdx(Dir)] = It->GetActorLocation();
		HubDoorActors.Add(*It);
		++Found;
	}
	if (Found < 4) { UE_LOG(LogEON, Warning, TEXT("EON hub : %d sas trouvés sur 4, le hub n'est pas branché sur l'étage"), Found); return; }
	bHub = true;
	// Terminal du C.O.M : un cube lumineux sur la passerelle est, juste devant le dôme (visible de la caméra).
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector East = HubDoor[DoorIdx(EONDoor::E)];
	HubCube = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(HubCentre.X + (East.X - HubCentre.X) * 0.66f, HubCentre.Y + 150.f, HubCentre.Z + 50.f), FRotator::ZeroRotator, P);
	if (HubCube && Cube)
	{
		HubCube->SetMobility(EComponentMobility::Movable);
		UStaticMeshComponent* SM = HubCube->GetStaticMeshComponent();
		SM->SetStaticMesh(Cube);
		SM->SetWorldScale3D(FVector(0.6f));
		SM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Mat)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Mat, HubCube);
			M->SetVectorParameterValue(TEXT("BC1"), FLinearColor(0.24f, 0.95f, 0.88f));
			M->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.4f, 3.f, 2.7f));
			SM->SetMaterial(0, M);
		}
	}
	UE_LOG(LogEON, Display, TEXT("EON hub : salle du C.O.M = hub 3D, terminal en %s, salles générées en %s"),
		HubCube ? *HubCube->GetActorLocation().ToCompactString() : TEXT("?"), *GetActorLocation().ToCompactString());
}

void AEONFloorManager::LoadHub(uint8 Entry, bool bFromCom)
{
	AEONCharacter* J = GetJira();
	if (!J) { return; }
	// Terminal du C.O.M : sur une passerelle libre (jamais celle du gardien, murée par la chair), juste devant le dôme.
	if (HubCube)
	{
		const FVector Sd = HubSafeDir();
		const FVector Door = HubDoor[DoorIdx(HubSafeDoor())];
		const float R = FVector2D::Distance(FVector2D(Door), FVector2D(HubCentre)) * 0.66f;
		HubCube->SetActorLocation(HubCentre + Sd * R + FVector(-Sd.Y, Sd.X, 0.f) * 150.f + FVector(0.f, 0.f, 50.f));
	}
	// Par le C.O.M : 3 m du centre ; sinon 3,5 m en dedans du sas d'arrivée.
	FVector Pos;
	if (bFromCom || !Entry || !HubCube)
	{
		Pos = HubCentre + HubSafeDir() * 300.f;   // à 3 m du centre du C.O.M, côté passerelle libre
	}
	else
	{
		const FIntPoint Dv = EONDoor::Dir(Entry);
		Pos = HubDoor[DoorIdx(Entry)] - FVector(Dv.X, Dv.Y, 0.f) * 350.f;
	}
	Pos.Z = HubCentre.Z + J->GetSimpleCollisionHalfHeight() + 2.f;
	J->ResetForRoom(Pos);
	if (APlayerController* PC = Cast<APlayerController>(J->GetController())) { if (PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); } }
	BuildHubFlesh();
	ShowBanner(bFromCom ? FString::Printf(TEXT("C.O.M - gardien : %s"), *GuardDir()) : RoleName(TEXT("DEPART")), FLinearColor(FColor(62, 242, 224)));
	UE_LOG(LogEON, Display, TEXT("EON hub : Jira en %s (entrée %s)"), *J->GetActorLocation().ToCompactString(), Entry ? EONDoor::Name(Entry) : TEXT("C.O.M"));
}

void AEONFloorManager::BuildHubFlesh()
{
	for (AStaticMeshActor* A : HubFlesh) { if (A) { A->Destroy(); } }
	HubFlesh.Reset();
	if (bDocked || !IsInfestEdge(Start, Boss)) { return; }
	uint8 D = 0;
	for (uint8 Dd : DoorOrder) { if (Start + EONDoor::Dir(Dd) == Boss) { D = Dd; } }
	if (!D) { return; }
	// setInfest du hub : la passerelle vers le gardien est murée de chair, du sas jusqu'au bord du C.O.M.
	UStaticMesh* Sph = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"));
	if (!Sph) { return; }
	UMaterialInstanceDynamic* M = Mat ? UMaterialInstanceDynamic::Create(Mat, this) : nullptr;
	if (M)
	{
		M->SetVectorParameterValue(TEXT("BC1"), FLinearColor(FColor(138, 42, 74)));
		M->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.25f, 0.02f, 0.08f));
	}
	const FVector Door = HubDoor[DoorIdx(D)];
	const FIntPoint Dv = EONDoor::Dir(D);
	const FVector Out(Dv.X, Dv.Y, 0.f), Side(-Dv.Y, Dv.X, 0.f);
	FRandomStream R(FloorN * 131 + 7);
	// La chair reste sur la passerelle (grilles, du sas jusqu'à 4,1 m en dedans) : elle ne déborde pas sur le C.O.M.
	for (int32 I = 0; I < 5; ++I)
	{
		for (int32 K = -1; K <= 1; ++K)
		{
			const FVector At = Door - Out * (70.f + I * 85.f) + Side * (K * 70.f + R.FRandRange(-10.f, 10.f));
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(At.X, At.Y, HubCentre.Z + 45.f), FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), P);
			if (!A) { continue; }
			A->SetMobility(EComponentMobility::Movable);
			UStaticMeshComponent* SM = A->GetStaticMeshComponent();
			SM->SetStaticMesh(Sph);
			const float S = R.FRandRange(0.95f, 1.2f);
			SM->SetWorldScale3D(FVector(S, S, S * 0.75f));
			SM->SetCollisionProfileName(TEXT("BlockAll"));
			if (M) { SM->SetMaterial(0, M); }
			HubFlesh.Add(A);
		}
	}
	HubFleshK = 1.f;
}

void AEONFloorManager::TickHub(float DeltaSeconds, AEONCharacter* J)
{
	const FEONFloorCell* C = CellAt(Cur);
	if (!C) { return; }
	const FVector2D P(J->GetActorLocation());
	// Chair : se rétracte en 1,6 s quand le gardien est neutralisé ; sinon, s'en approcher = refus.
	if (HubFlesh.Num())
	{
		if (bFloorDone)
		{
			HubFleshK = FMath::Max(0.f, HubFleshK - DeltaSeconds / 1.6f);
			for (AStaticMeshActor* A : HubFlesh) { if (A) { A->SetActorScale3D(A->GetActorScale3D() * FMath::Max(0.05f, HubFleshK) / FMath::Max(0.05f, HubFleshK + DeltaSeconds / 1.6f)); } }
			if (HubFleshK <= 0.f) { for (AStaticMeshActor* A : HubFlesh) { if (A) { A->Destroy(); } } HubFlesh.Reset(); }
		}
		else if ((InfBumpT -= DeltaSeconds) <= 0.f)
		{
			for (const AStaticMeshActor* A : HubFlesh)
			{
				if (A && FVector2D::Distance(P, FVector2D(A->GetActorLocation())) < 160.f)
				{
					InfBumpT = 3.f;
					if (!bInfHint) { bInfHint = true; if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("infestCom")); } }
					else { ShowBanner(TEXT("La mutation bloque le passage"), FLinearColor(FColor(255, 61, 85))); }
					break;
				}
			}
		}
	}
	// Sortie par un sas : Jira à moins de 1,3 m de la porte.
	RedBumpT -= DeltaSeconds;
	for (uint8 D : DoorOrder)
	{
		if (!(C->Doors & D) || FVector2D::Distance(P, FVector2D(HubDoor[DoorIdx(D)])) > 130.f) { continue; }
		const FIntPoint N = Cur + EONDoor::Dir(D);
		const FEONFloorCell* NC = CellAt(N);
		if (!NC || IsInfestEdge(Cur, N)) { continue; }
		if (NC->Role == TEXT("ASCENSEUR") && !bRedCard && FloorN != 4)
		{
			if (RedBumpT <= 0.f)
			{
				RedBumpT = 2.5f;
				if (!bRedHint) { bRedHint = true; if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("redDoor")); } }
				else { ShowBanner(TEXT("Carte rouge requise"), FLinearColor(FColor(255, 61, 85))); }
			}
			continue;
		}
		StartTrans([this, N, D]() { EnterCell(N, D, false); });
		return;
	}
}

uint8 AEONFloorManager::HubSafeDoor() const
{
	// Est d'abord (côté caméra le plus lisible), puis sud, ouest, nord : la première passerelle qui ne mène pas au gardien.
	if (bDocked) { return EONDoor::S; }
	for (uint8 D : { EONDoor::E, EONDoor::S, EONDoor::W, EONDoor::N }) { if (Start + EONDoor::Dir(D) != Boss) { return D; } }
	return EONDoor::E;
}

FVector AEONFloorManager::HubSafeDir() const
{
	const FIntPoint Dv = EONDoor::Dir(HubSafeDoor());
	return FVector(Dv.X, Dv.Y, 0.f);
}

// ---------------------------------------------------------------- la Faille (buildRoom, riftOpen, updateRift)

void AEONFloorManager::BuildRift(FEONFloorCell& C)
{
	// Une paroi extérieure va céder : un pan de mur sans porte (sinon on décale la brèche pour ne jamais crever une porte).
	const FEONTileGrid& G = C.Grid;
	const int32 W = G.W, H = G.H;
	TArray<uint8> Free;
	for (uint8 D : DoorOrder) { if (!(C.Doors & D)) { Free.Add(D); } }
	const uint8 Side = Free.Num() ? Free[Rng.RandRange(0, Free.Num() - 1)] : DoorOrder[Rng.RandRange(0, 3)];
	const bool bNS = Side == EONDoor::N || Side == EONDoor::S;
	const int32 Cxr = W >> 1, Cyr = H >> 1, Len = bNS ? 6 : 4;
	const int32 Off = (C.Doors & Side) ? (Rng.GetFraction() < 0.5f ? -1 : 1) * ((bNS ? Cxr : Cyr) - Len) : 0;
	const int32 C0 = (bNS ? Cxr : Cyr) + Off;
	C.RiftTiles.Reset();
	auto Add = [&](int32 X, int32 Y) { C.RiftTiles.Add(Y * W + X); };
	for (int32 K = -(Len >> 1); K < Len - (Len >> 1); ++K)
	{
		const int32 Cc = FMath::Clamp(C0 + K, 1, (bNS ? W : H) - 2);
		if (Side == EONDoor::N) { Add(Cc, 0); Add(Cc, 1); }
		else if (Side == EONDoor::S) { Add(Cc, H - 1); Add(Cc, H - 2); }
		else if (Side == EONDoor::W) { Add(0, Cc); Add(1, Cc); }
		else { Add(W - 1, Cc); Add(W - 2, Cc); }
	}
	C.RiftPx = FVector2D(Side == EONDoor::W ? 0.f : Side == EONDoor::E ? W * 16.f : C0 * 16.f,
		Side == EONDoor::N ? 0.f : Side == EONDoor::S ? H * 16.f : C0 * 16.f);
	// Console de confinement : la tuile de sol (entourée de sol) la plus éloignée de la brèche, pour forcer la traversée.
	FIntPoint Best(Cxr, Cyr);
	float Bd = -1.f;
	const uint8 Fl = (uint8)EEONTile::Floor;
	for (int32 I = 0; I < W * H; ++I)
	{
		if (G.T[I] != Fl) { continue; }
		const int32 X = I % W, Y = I / W;
		if (X < 2 || Y < 2 || X > W - 3 || Y > H - 3) { continue; }
		if (G.T[I + 1] != Fl || G.T[I - 1] != Fl || G.T[I + W] != Fl || G.T[I - W] != Fl) { continue; }
		const float D = FVector2D::Distance(FVector2D(X * 16.f + 8.f, Y * 16.f + 8.f), C.RiftPx);
		if (D > Bd) { Bd = D; Best = FIntPoint(X, Y); }
	}
	C.ConfPx = FVector2D(Best.X * 16.f + 8.f, Best.Y * 16.f + 8.f);
	C.RiftSide = Side;
	C.bRiftDone = false;
	UE_LOG(LogEON, Display, TEXT("EON Faille : brèche côté %d (%d tuiles), console en (%d,%d)"), Side, C.RiftTiles.Num(), Best.X, Best.Y);
}

void AEONFloorManager::RiftOpen(FEONFloorCell& C, int32 N)
{
	// Stade 1 : le bord extérieur de la paroi devient du vide ; stade 2 : le rang intérieur aussi.
	if (!Room) { return; }
	FEONTileGrid G = Room->GetGrid();
	bool bChanged = false;
	for (int32 I = 0; I < C.RiftTiles.Num(); ++I)
	{
		if (I % 2 != (N >= 2 ? 1 : 0)) { continue; }
		const int32 Ti = C.RiftTiles[I];
		if (G.T.IsValidIndex(Ti) && G.T[Ti] != (uint8)EEONTile::Pit) { G.T[Ti] = (uint8)EEONTile::Pit; bChanged = true; }
	}
	if (!bChanged) { return; }
	const uint8 Red = Room->GetRedDoor();
	Room->SetGrid(G);
	Room->SetRedDoor(Red);
	Room->SetInfest(C.Infest, C.InfestK);
	if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->Shake(0.6f); }
	EONSfx::Play(this, TEXT("SFX_Proto_explode"), 0.9f);
}

void AEONFloorManager::TickRift(float Dt, AEONCharacter* J, const FVector2D& P, FEONFloorCell& C)
{
	RiftT += Dt;
	if (RiftStage == 0 && RiftT > 1.2f)
	{
		RiftStage = 1;
		RiftOpen(C, 1);
		ShowBanner(TEXT("Rupture de paroi"), FLinearColor(FColor(62, 242, 224)));
		if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("rift")); }
	}
	else if (RiftStage == 1 && RiftT > 6.f)
	{
		RiftStage = 2;
		RiftOpen(C, 2);
		ShowBanner(TEXT("La breche s'elargit"), FLinearColor(FColor(255, 61, 85)));
	}
	if (RiftStage == 0) { return; }
	// Alarme de la station : elle se resserre quand la brèche s'élargit, puis quand l'oxygène tombe.
	if ((RiftAlarmT -= Dt) <= 0.f)
	{
		RiftAlarmT = RiftO2 <= 0.f ? 1.2f : RiftStage > 1 ? 2.1f : 3.f;
		EONSfx::Play(this, TEXT("SFX_Proto_alarm"), 0.8f);
	}
	// Aspiration vers la brèche (70 px/s par stade) : Jira dérive vraiment (x 0,45), les ennemis (x 0,6 x 0,5), les ramassables (x 0,5 x 0,5).
	const float Fz = 70.f * RiftStage;
	auto Pull = [&](const FVector2D& O) { const FVector2D D = C.RiftPx - O; const float L = FMath::Max(1.f, D.Size()); return D / L * Fz; };
	if (!J->IsFallingIntoVoid())
	{
		const FVector2D Step = Pull(P) * Dt * 0.45f;
		const FVector W0 = Room->RoomPxToWorld(P), W1 = Room->RoomPxToWorld(P + Step);
		J->AddActorWorldOffset(FVector(W1.X - W0.X, W1.Y - W0.Y, 0.f), true);
	}
	AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld());
	if (Dir)
	{
		for (AEONEnemy* E : Dir->GetEnemies())
		{
			if (!E || E->bDead || E->Falling > 0.f || E->IsBoss()) { continue; }
			E->Pos += Pull(E->Pos) * 0.6f * Dt * 0.5f;
		}
	}
	AEONPickup::PullAll(GetWorld(), C.RiftPx, Fz * 0.5f * 0.5f, Dt);
	// Oxygène : 32 s ; à zéro, 1 PV toutes les 2 s.
	RiftO2 = FMath::Max(0.f, RiftO2 - Dt / 32.f);
	if (RiftO2 <= 0.f && (RiftHurtT -= Dt) <= 0.f)
	{
		RiftHurtT = 2.f;
		if (Dir) { Dir->HurtJira(1, P + FVector2D(0.f, 40.f)); }
	}
	// Console : disponible une fois la salle vide (toutes les vagues parties, plus d'ennemi).
	if (!bConfOn && Dir && Dir->IsFightOver())
	{
		bConfOn = true;
		ShowBanner(TEXT("Confinement disponible"), FLinearColor(FColor(62, 242, 224)));
		EONSfx::Play(this, TEXT("SFX_Proto_card"), 0.8f);
		if (ConfMID) { ConfMID->SetVectorParameterValue(TEXT("BC1"), FLinearColor(0.24f, 0.95f, 0.88f)); ConfMID->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.4f, 3.f, 2.8f)); }
	}
	if (bConfOn && !J->IsFallingIntoVoid() && FVector2D::Distance(P, C.ConfPx) < 12.f)
	{
		// Confinement : la paroi se referme (les deux rangs deviennent du mur), la salle est libérée, l'oxygène revient.
		C.bRiftDone = true;
		for (const int32 I : C.RiftTiles) { if (C.Grid.T.IsValidIndex(I)) { C.Grid.T[I] = (uint8)EEONTile::Wall; } }
		const uint8 Red = Room->GetRedDoor();
		Room->SetGrid(C.Grid);
		Room->SetRedDoor(Red);
		Room->SetInfest(C.Infest, C.InfestK);
		MarkCurrentRoomCleared();
		RiftO2 = 1.f;
		RiftT = -1.f;
		RiftStage = 0;
		bConfOn = false;
		if (ConfMesh) { ConfMesh->SetVisibility(false); }
		EONSfx::Play(this, TEXT("SFX_Proto_door"), 0.8f);
		if (Dir) { Dir->Shake(0.3f); }
		ShowBanner(TEXT("Paroi confinee"), FLinearColor(FColor(138, 240, 122)));
	}
}

bool AEONFloorManager::IsRiftActive() const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	return C && C->Role == TEXT("FAILLE") && !C->bRiftDone && RiftT >= 0.f && RiftStage > 0;
}

bool AEONFloorManager::HoldsRoomClear() const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	return C && C->Role == TEXT("FAILLE") && !C->bRiftDone && C->RiftTiles.Num() > 0 && RiftT >= 0.f;
}

FVector AEONFloorManager::GetConfWorld() const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	return C && Room ? Room->RoomPxToWorld(C->ConfPx, 50.f) : FVector::ZeroVector;
}

// ---------------------------------------------------------------- cuves (placeVats) et Irruption (eventsUpdate)

void AEONFloorManager::ClearRoomActors()
{
	for (AActor* A : RoomActors) { if (A) { A->Destroy(); } }
	RoomActors.Reset();
	ExpBias = 0.f;
	if (LampLight) { LampLight->SetVisibility(false); }
	if (EyeLight) { EyeLight->SetVisibility(false); }
}

void AEONFloorManager::PlaceVats(FEONFloorCell& C)
{
	// 2 à 4 cuves de 2 × 3 tuiles le long des murs nord et sud ; une cuve n'est posée que si elle laisse un tour de sol
	// devant elle, ne coupe aucune porte et ne retire pas plus de 6 tuiles accessibles par cuve.
	if (C.Shape == TEXT("corH") || C.Shape == TEXT("corV")) { return; }
	FEONTileGrid& G = C.Grid;
	const int32 W = G.W, H = G.H, Cx = W >> 1;
	const uint8 Fl = (uint8)EEONTile::Floor;
	auto IsF = [&](int32 I) { return G.T[I] == Fl; };
	TArray<uint8> DoorList;
	for (uint8 D : DoorOrder) { if (G.Doors & D) { DoorList.Add(D); } }
	if (!DoorList.Num()) { return; }
	auto Reach = [&](bool& bOk) -> int32
	{
		auto Walk = [&](int32 I) { const uint8 V = G.T[I]; return V == Fl || V == (uint8)EEONTile::Door || (V == (uint8)EEONTile::Pit && G.Gap.Contains(I)); };
		FIntPoint Dt[2];
		G.GetDoorTiles(DoorList[0], Dt);
		TArray<uint8> Seen; Seen.SetNumZeroed(W * H);
		TArray<int32> Q = { Dt[0].Y * W + Dt[0].X };
		Seen[Q[0]] = 1;
		int32 N = 0;
		while (Q.Num())
		{
			const int32 I = Q.Pop(EAllowShrinking::No);
			if (G.T[I] == Fl) { ++N; }
			const int32 X = I % W, Y = I / W;
			for (const FIntPoint Dd : { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) })
			{
				const int32 Nx = X + Dd.X, Ny = Y + Dd.Y;
				if (Nx < 0 || Ny < 0 || Nx >= W || Ny >= H) { continue; }
				const int32 J = Ny * W + Nx;
				if (!Seen[J] && Walk(J)) { Seen[J] = 1; Q.Add(J); }
			}
		}
		bOk = true;
		for (uint8 D : DoorList) { FIntPoint T2[2]; G.GetDoorTiles(D, T2); for (const FIntPoint& Tq : T2) { if (!Seen[Tq.Y * W + Tq.X]) { bOk = false; } } }
		return N;
	};
	bool bOk0 = true;
	const int32 Base = Reach(bOk0);
	const int32 Want = 2 + Rng.RandRange(0, 2);
	struct FSlot { int32 X, Y; uint8 Side; };
	TArray<FSlot> Slots;
	for (int32 X = 2; X < W - 3; ++X) { Slots.Add({ X, 1, EONDoor::N }); Slots.Add({ X, H - 4, EONDoor::S }); }
	for (int32 I = Slots.Num() - 1; I > 0; --I) { Slots.Swap(I, Rng.RandRange(0, I)); }
	C.Pods.Reset();
	C.PodBroken.Reset();
	for (const FSlot& S : Slots)
	{
		if (C.Pods.Num() >= Want) { break; }
		if ((G.Doors & S.Side) && FMath::Abs(S.X + 1 - Cx) < 5) { continue; }
		bool bNear = false;
		for (const FIntPoint& Pq : C.Pods) { if (FMath::Abs(Pq.X - S.X) < 4 && Pq.Y == S.Y) { bNear = true; } }
		if (bNear) { continue; }
		bool bOk = true;
		for (int32 Y = S.Y; Y < S.Y + 3 && bOk; ++Y) { for (int32 X = S.X; X < S.X + 2; ++X) { if (!IsF(Y * W + X)) { bOk = false; } } }
		const int32 Fy = S.Side == EONDoor::N ? S.Y + 3 : S.Y - 1;
		for (int32 X = S.X; X < S.X + 2 && bOk; ++X) { if (!IsF(Fy * W + X)) { bOk = false; } }
		if (!bOk) { continue; }
		for (int32 Y = S.Y; Y < S.Y + 3; ++Y) { for (int32 X = S.X; X < S.X + 2; ++X) { G.T[Y * W + X] = (uint8)EEONTile::Tank; } }
		bool bReachOk = true;
		const int32 N = Reach(bReachOk);
		if (!bReachOk || N < Base - 6 * (C.Pods.Num() + 1))
		{
			for (int32 Y = S.Y; Y < S.Y + 3; ++Y) { for (int32 X = S.X; X < S.X + 2; ++X) { G.T[Y * W + X] = Fl; } }
			continue;
		}
		C.Pods.Add(FIntPoint(S.X, S.Y));
		C.PodBroken.Add(Rng.GetFraction() < 0.28f);
	}
	// Les cuves sont des acteurs (AEONWakePod) : pas de cylindre provisoire pour ces tuiles K.
	if (C.Pods.Num()) { G.Feat += TEXT("+pods"); }
	UE_LOG(LogEON, Display, TEXT("EON cuves : %d posées (%s)"), C.Pods.Num(), *C.Role.ToString());
}

void AEONFloorManager::TickIrruption(float Dt, AEONCharacter* J, const FVector2D& P, FEONFloorCell& C)
{
	IrrT += Dt;
	AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld());
	if (C.IrrSt == 0)
	{
		// Les lampes de la salle vacillent (35 % d'extinction à chaque image dans le proto) avant de lâcher.
		if ((FlickT -= Dt) <= 0.f) { FlickT = 0.05f; ExpBias = FMath::FRand() < 0.35f ? -1.5f : 0.f; }
		if (IrrT > 1.8f && Dir)
		{
			C.IrrSt = 1;
			IrrT = 0.f;
			EONSfx::Play(this, TEXT("SFX_Proto_hiss"), 0.8f);
			Dir->Shake(0.3f);
			ShowBanner(TEXT("Les lumieres lachent"), FLinearColor(FColor(90, 86, 120)));
			// La chose apparaît sur la tuile de sol la plus éloignée de Jira, à moins de 260 px.
			FVector2D Best(C.Grid.W * 8.f, C.Grid.H * 8.f);
			float Bd = -1.f;
			for (int32 I = 0; I < C.Grid.W * C.Grid.H; ++I)
			{
				if (C.Grid.T[I] != (uint8)EEONTile::Floor) { continue; }
				const FVector2D Q((I % C.Grid.W + 0.5f) * 16.f, (I / C.Grid.W + 0.5f) * 16.f);
				const float D = FVector2D::Distance(Q, P);
				if (D > Bd && D < 260.f) { Bd = D; Best = Q; }
			}
			AEONEnemy* M = Dir->SpawnEnemyAt(TEXT("slasher"), Best, true);
			if (M) { M->MakeStalker(); }
			Stalker = M;
			UE_LOG(LogEON, Display, TEXT("EON Irruption : les lumières lâchent, la chose en (%.0f, %.0f)"), Best.X, Best.Y);
		}
	}
	else if (C.IrrSt == 1)
	{
		// Le noir : seule la lampe de Jira (58 px) et les yeux de la chose éclairent.
		ExpBias = -5.f;
		LampLight->SetVisibility(true);
		LampLight->SetWorldLocation(J->GetActorLocation() + FVector(0.f, 0.f, 80.f));
		AEONEnemy* M = Stalker.Get();
		if (M && !M->bDead)
		{
			const bool bLurk = M->StalkSt == 0;
			EyeLight->SetVisibility(true);
			EyeLight->SetAttenuationRadius(EON::PxToCm(bLurk ? 10.f : 22.f));
			EyeLight->SetIntensity(bLurk ? 15000.f : 60000.f);
			EyeLight->SetWorldLocation(Room->RoomPxToWorld(M->Pos, 70.f));
		}
		else
		{
			// Elle est morte : la lumière revient, un objet rare ou épique sur un socle, 3 Données.
			C.IrrSt = 2;
			ExpBias = 0.f;
			LampLight->SetVisibility(false);
			EyeLight->SetVisibility(false);
			MarkCurrentRoomCleared();
			EONSfx::Play(this, TEXT("SFX_Proto_door"), 0.8f);
			ShowBanner(TEXT("La lumiere revient"), FLinearColor(FColor(62, 242, 224)));
			const FVector2D Ctr((C.Grid.W >> 1) * 16.f, (C.Grid.H >> 1) * 16.f);
			FName It = NAME_None;
			for (const FName& Id : EONItems::Roll(3, J, false)) { if (EONItems::Find(Id)->Rar != TEXT('C')) { It = Id; break; } }
			if (It.IsNone()) { const TArray<FName> One = EONItems::Roll(1, J, false); if (One.Num()) { It = One[0]; } }
			if (!It.IsNone())
			{
				C.PedItems.Add(It); C.PedPrice.Add(0); C.PedPx.Add(Ctr);
				AEONPedestal::Spawn(GetWorld(), Room, It, Ctr, 0, C.PedItems.Num() - 1);
			}
			AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Donnee, Ctr + FVector2D(24.f, 10.f), FVector2D::ZeroVector, 3.f, 1.f);
		}
	}
}

// ---------------------------------------------------------------- le Cartographe (eventsUpdate)

void AEONFloorManager::TickCarto(float Dt, AEONCharacter* J, const FVector2D& P, FEONFloorCell& C)
{
	if (C.KidSt >= 5)
	{
		// Reparti dans les conduits : il s'efface en 1,2 s.
		if (AEONNpc* N = KidNpc.Get()) { KidT += Dt; if (KidT > 1.2f) { N->Destroy(); KidNpc = nullptr; } }
		return;
	}
	KidHurtT -= Dt;
	const FVector2D K = KidPxOf(C);
	// Il est solide : Jira ne passe pas à moins de 8 px.
	const float D = FVector2D::Distance(P, K);
	if (D < 8.f && !J->IsFallingIntoVoid())
	{
		const FVector2D To = K + (P - K) / FMath::Max(D, 0.01f) * 8.f;
		const FVector W0 = Room->RoomPxToWorld(P), W1 = Room->RoomPxToWorld(To);
		J->AddActorWorldOffset(FVector(W1.X - W0.X, W1.Y - W0.Y, 0.f), true);
	}
	UEONUISubsystem* UI = UEONUISubsystem::Get(this);
	AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld());
	if (C.KidSt == 0 && D < 70.f && UI && !UI->IsBlocking() && !UI->HasPendingDialog() && !J->IsFallingIntoVoid())
	{
		// « Ils me suivent depuis les conduits ! » : à la fin du dialogue, la salle se verrouille, 2 vagues.
		C.KidSt = 1;
		const FIntPoint Cell = Cur;
		UI->Say(TEXT("cartoStart"), [this, Cell]()
		{
			FEONFloorCell* Cc = CellAt(Cell);
			if (!Cc || Cur != Cell) { return; }
			Cc->KidSt = 2;
			Cc->bCleared = false;
			Room->SetLocked(true);
			RoomWaves = 2;
			if (AEONCombatDirector* Dd = AEONCombatDirector::Get(GetWorld())) { Dd->StartRoomWaves(2, 0.8f); }
			EONSfx::Play(this, TEXT("SFX_Proto_door"), 0.8f);
		});
	}
	if (C.KidSt == 2 && Dir)
	{
		// Chaque contact d'ennemi lui retire 1 PV, au plus un toutes les 0,8 s.
		if (KidHurtT <= 0.f)
		{
			for (const AEONEnemy* E : Dir->GetEnemies())
			{
				if (!E || E->bDead || E->Falling > 0.f) { continue; }
				if (FVector2D::Distance(E->Pos, K) < E->R + 7.f)
				{
					--C.KidHp;
					KidHurtT = 0.8f;
					if (AEONNpc* N = KidNpc.Get()) { N->Flash(0.15f); }
					EONSfx::Play(this, TEXT("SFX_Proto_hurt"), 0.7f);
					if (C.KidHp <= 0)
					{
						C.KidSt = 3;
						if (AEONNpc* N = KidNpc.Get()) { N->SetDown(true); }
						if (UI) { UI->Say(TEXT("cartoLost")); }
					}
					break;
				}
			}
		}
		// La salle nettoyée et lui vivant : il donne sa carte (toutes les salles de l'étage et leur rôle).
		if (C.KidSt == 2 && C.bCleared)
		{
			C.KidSt = 4;
			const FIntPoint Cell = Cur;
			if (UI)
			{
				UI->Say(TEXT("cartoWin"), [this, Cell]()
				{
					bMapped = true;
					for (TPair<FIntPoint, FEONFloorCell>& KV : Cells) { KV.Value.bSeen = true; }
					if (FEONFloorCell* Cc = CellAt(Cell)) { Cc->KidSt = 5; }
					KidT = 0.f;
					EONSfx::Play(this, TEXT("SFX_Proto_item"), 0.8f);
					ShowBanner(TEXT("Carte de l'etage completee"), FLinearColor(FColor(62, 242, 224)));
				});
			}
		}
	}
}

bool AEONFloorManager::IsKidDefending() const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	return C && C->Role == TEXT("CARTO") && C->KidSt == 2;
}

bool AEONFloorManager::GetKidTarget(FVector2D& OutPx) const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	if (!C || C->Role != TEXT("CARTO") || C->KidSt != 2) { return false; }
	OutPx = KidPxOf(*C);
	return true;
}

bool AEONFloorManager::GetKidHud(FVector& OutWorld, uint8& OutSt, int32& OutHp) const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	if (!C || C->Role != TEXT("CARTO") || C->KidSt >= 5 || !Room || !KidNpc.IsValid()) { return false; }
	OutWorld = Room->RoomPxToWorld(KidPxOf(*C), EON::PxToCm(26.f));
	OutSt = C->KidSt;
	OutHp = C->KidHp;
	return true;
}

bool AEONFloorManager::GetAutoNpcPx(FVector2D& OutPx) const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	if (!C) { return false; }
	if (C->Role == TEXT("CARTO") && C->KidSt == 0) { OutPx = KidPxOf(*C) + FVector2D(0.f, 20.f); return true; }
	// La gamine qui attend, Ulysse sauf qui n'a pas encore parlé.
	if (C->Role == TEXT("EVENEMENT") && ((C->EvtKind == 1 && C->EvtSt == 0) || (C->EvtKind == 2 && C->EvtSt == 2))) { OutPx = EvtNpcPx(*C) + FVector2D(0.f, 18.f); return true; }
	return false;
}

// ---------------------------------------------------------------- la gamine, Ulysse, l'équipage, les logs (eventsUpdate, markFloorPickups, readLog)

namespace
{
	const TCHAR* GEvtCrew[] = { TEXT("Ex-quartier-maitre"), TEXT("Ex-mecanicienne"), TEXT("Ex-pilote"), TEXT("Ex-cuisinier"), TEXT("Ex-medecin"), TEXT("Ex-operateur radio") };
	const FLinearColor EvtAm(FColor(255, 194, 61));
	bool AnyAlive(const AEONCombatDirector* Dir)
	{
		if (!Dir) { return false; }
		for (const AEONEnemy* E : Dir->GetEnemies()) { if (E && !E->bDead) { return true; } }
		return false;
	}
}

TArray<FString> AEONFloorManager::CrewMissing() const
{
	TArray<FString> R;
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	for (const TCHAR* N : GEvtCrew) { if (!(MS && MS->M() && MS->M()->Crew.Contains(N))) { R.Add(N); } }
	return R;
}

void AEONFloorManager::MarkFloorPickups()
{
	// Un log par étage ; à l'étage 1, tant que la gamine n'a pas été rencontrée, sa peluche est cachée quelque part.
	TArray<FIntPoint> Ce;
	for (auto& KV : Cells)
	{
		const FName R = KV.Value.Role;
		if (R == TEXT("CALME") || R == TEXT("OPPORTUNITE") || R == TEXT("COMBAT") || R == TEXT("TENSION")) { Ce.Add(KV.Key); }
	}
	if (!Ce.Num()) { return; }
	Cells[Pick(Rng, Ce)].bLog = true;
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	if (FloorN == 1 && !(MS && MS->M() && MS->M()->bNewtDone) && !bHasDoudou)
	{
		TArray<FIntPoint> C2, C3;
		for (const FIntPoint& K : Ce) { if (!Cells[K].bLog) { C3.Add(K); if (Cells[K].Role != TEXT("COMBAT")) { C2.Add(K); } } }
		const TArray<FIntPoint>& Pool = C2.Num() ? C2 : C3;
		if (Pool.Num()) { Cells[Pick(Rng, Pool)].bDoudou = true; }
	}
}

FVector2D AEONFloorManager::FreeSpot(const FEONTileGrid& G, int32 Dx)
{
	// freeSpot : la tuile de sol la plus proche du centre (décalé de Dx), entourée de sol.
	const int32 Cx = (G.W >> 1) + Dx, Cy = G.H >> 1;
	auto Fl = [&G](int32 X, int32 Y) { return G.Get(X, Y) == EEONTile::Floor; };
	for (int32 Rad = 0; Rad < 12; ++Rad)
	{
		for (int32 Y = Cy - Rad; Y <= Cy + Rad; ++Y)
		{
			for (int32 X = Cx - Rad; X <= Cx + Rad; ++X)
			{
				if (X < 2 || Y < 2 || X > G.W - 3 || Y > G.H - 3) { continue; }
				if (!Fl(X, Y) || !Fl(X + 1, Y) || !Fl(X - 1, Y) || !Fl(X, Y + 1) || !Fl(X, Y - 1)) { continue; }
				return FVector2D((X + 0.5f) * 16.f, (Y + 0.5f) * 16.f);
			}
		}
	}
	return FVector2D((G.W >> 1) * 16.f, (G.H >> 1) * 16.f);
}

void AEONFloorManager::PlaceEvtPed(FEONFloorCell& C, const FVector2D& Px, bool bEpic)
{
	AEONCharacter* J = GetJira();
	const TArray<FName> R3 = EONItems::Roll(3, J, false);
	FName It = NAME_None;
	if (bEpic) { for (const FName& Id : R3) { if (EONItems::Find(Id)->Rar == TEXT('E')) { It = Id; break; } } }
	if (It.IsNone()) { for (const FName& Id : R3) { if (EONItems::Find(Id)->Rar != TEXT('C')) { It = Id; break; } } }
	if (It.IsNone()) { const TArray<FName> One = EONItems::Roll(1, J, false); if (One.Num()) { It = One[0]; } }
	if (It.IsNone()) { return; }
	C.PedItems.Add(It); C.PedPrice.Add(0); C.PedPx.Add(Px);
	AEONPedestal::Spawn(GetWorld(), Room, It, Px, 0, C.PedItems.Num() - 1);
}

void AEONFloorManager::TickEvent(float Dt, AEONCharacter* J, const FVector2D& P, FEONFloorCell& C)
{
	UEONUISubsystem* UI = UEONUISubsystem::Get(this);
	AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld());
	UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	UEONSaveGame* M = MS ? MS->M() : nullptr;
	const FVector2D N = EvtNpcPx(C);
	const float D = FVector2D::Distance(P, N);
	const bool bFree = UI && !UI->IsBlocking() && !UI->HasPendingDialog() && !J->IsFallingIntoVoid();
	const FIntPoint Cell = Cur;
	// Le PNJ est solide (8 px).
	if (EvtNpc.IsValid() && D < 8.f && !J->IsFallingIntoVoid())
	{
		const FVector2D To = N + (P - N) / FMath::Max(D, 0.01f) * 8.f;
		const FVector W0 = Room->RoomPxToWorld(P), W1 = Room->RoomPxToWorld(To);
		J->AddActorWorldOffset(FVector(W1.X - W0.X, W1.Y - W0.Y, 0.f), true);
	}
	if (C.EvtKind == 1)
	{
		// La gamine (« Newt ») : avec sa peluche elle se calme et donne un objet ; sans elle, elle mute et attaque.
		if (C.EvtSt == 0 && D < 34.f && bFree)
		{
			C.EvtSt = 1;
			if (bHasDoudou)
			{
				UI->Say(TEXT("newtGood"), [this, Cell]()
				{
					FEONFloorCell* Cc = CellAt(Cell);
					if (!Cc || Cur != Cell) { return; }
					Cc->EvtSt = 2;
					bHasDoudou = false;
					if (UEONMetaSubsystem* Ms = UEONMetaSubsystem::Get(this); Ms && Ms->M()) { Ms->M()->bNewtDone = true; Ms->M()->NewtEnd = TEXT("calme"); Ms->Save(); }
					if (AEONNpc* Np = EvtNpc.Get()) { Np->ShakePx = 0.f; }
					EONSfx::Play(this, TEXT("SFX_Proto_item"), 0.8f);
					PlaceEvtPed(*Cc, EvtNpcPx(*Cc) + FVector2D(30.f, 26.f), false);
				});
			}
			else
			{
				UI->Say(TEXT("newtBad"), [this, Cell]()
				{
					FEONFloorCell* Cc = CellAt(Cell);
					AEONCombatDirector* Dd = AEONCombatDirector::Get(GetWorld());
					if (!Cc || Cur != Cell || !Dd) { return; }
					Cc->EvtSt = 3;
					Cc->bCleared = false;
					Room->SetLocked(true);
					EONSfx::Play(this, TEXT("SFX_Proto_roar"), 0.9f);
					Dd->Shake(0.6f);
					if (AEONNpc* Np = EvtNpc.Get()) { Np->Destroy(); EvtNpc = nullptr; }
					if (AEONEnemy* E = Dd->SpawnEnemyAt(TEXT("brainless"), EvtNpcPx(*Cc) + FVector2D(0.f, 10.f), true))
					{
						E->DisplayName = TEXT("La gamine");
						E->Hp = E->MaxHp = FMath::RoundToFloat(22.f * (1.f + 0.3f * (FloorN - 1)));
						E->R = 7.f;
						E->MassM = 2.f;
					}
					ShowBanner(TEXT("La gamine mute"), FLinearColor(FColor(255, 61, 85)));
				});
			}
		}
		if (C.EvtSt == 3 && Room->IsLocked() && !AnyAlive(Dir))
		{
			C.EvtSt = 4;
			MarkCurrentRoomCleared();
			if (M) { M->bNewtDone = true; M->NewtEnd = TEXT("perdue"); MS->Save(); }
			EONSfx::Play(this, TEXT("SFX_Proto_door"), 0.8f);
			ShowBanner(TEXT("Elle ne souffre plus"), FLinearColor(FColor(138, 132, 184)));
			PlaceEvtPed(C, N + FVector2D(P.X < N.X ? 34.f : -34.f, 30.f), true);
		}
		return;
	}
	// Ulysse, l'équipage, le survivant : un mutant élite garde la salle.
	if (C.EvtSt == 0 && Dir)
	{
		C.EvtSt = 1;
		const FName Ty = C.EvtKind == 3 ? (FMath::RandBool() ? FName(TEXT("slasher")) : FName(TEXT("brainless"))) : FName(TEXT("slasher"));
		const FVector2D Sp = C.EvtKind == 2 ? N + FVector2D(20.f, 34.f) : FVector2D((C.Grid.W >> 1) * 16.f, (C.Grid.H >> 1) * 16.f - 10.f);
		if (AEONEnemy* E = Dir->SpawnEnemyAt(Ty, Sp, true))
		{
			E->DisplayName = C.EvtKind == 3 ? C.CrewName : C.EvtKind == 2 ? FString(TEXT("Ex-quartier-maitre")) : FString(TEXT("Survivant mute"));
			E->Hp = E->MaxHp = FMath::RoundToFloat(20.f * (1.f + 0.3f * (FloorN - 1)));
			E->R = 6.f;
			E->MassM = 2.2f;
		}
	}
	else if (C.EvtSt == 1 && !AnyAlive(Dir))
	{
		C.EvtSt = 2;
		MarkCurrentRoomCleared();
		EONSfx::Play(this, TEXT("SFX_Proto_door"), 0.8f);
		if (AEONNpc* Np = EvtNpc.Get()) { Np->ShakePx = 0.f; }
		if (C.EvtKind == 2) { ShowBanner(TEXT("Ulysse est sauf"), EvtAm); }
		else if (C.EvtKind == 3 && !(M && M->Crew.Contains(C.CrewName)))
		{
			const FVector2D Ctr((C.Grid.W >> 1) * 16.f, (C.Grid.H >> 1) * 16.f);
			if (AEONPickup* Pk = AEONPickup::Spawn(GetWorld(), Room, EEONPickup::Insigne, Ctr, FVector2D(0.f, -30.f), 1.f, 0.f)) { Pk->Name = C.CrewName; }
			ShowBanner(C.CrewName + TEXT(" : son insigne"), EvtAm);
		}
		else { ShowBanner(TEXT("Salle nettoyee"), FLinearColor(FColor(62, 242, 224))); }
	}
	// Ulysse sauf : à 28 px, il parle (ulysse1, ou ulysse2 s'il a déjà été croisé dans la run) et donne l'insigne du quartier-maître.
	if (C.EvtKind == 2 && C.EvtSt == 2 && D < 28.f && bFree)
	{
		C.EvtSt = 3;
		UI->Say(MetUlysse ? TEXT("ulysse2") : TEXT("ulysse1"), [this, Cell]()
		{
			FEONFloorCell* Cc = CellAt(Cell);
			UEONMetaSubsystem* Ms = UEONMetaSubsystem::Get(this);
			if (Ms && Ms->M())
			{
				Ms->M()->bUlysse = true;
				if (!Ms->M()->Crew.Contains(GEvtCrew[0])) { Ms->M()->Crew.Add(GEvtCrew[0], TEXT("poche")); }
				Ms->Save();
			}
			++MetUlysse;
			EONSfx::Play(this, TEXT("SFX_Proto_item"), 0.8f);
			ShowBanner(TEXT("Insigne du quartier-maitre - Ulysse t'attend au Kibo"), EvtAm);
			if (Cc && Cur == Cell) { PlaceEvtPed(*Cc, EvtNpcPx(*Cc) + FVector2D(26.f, 22.f), false); }
		});
	}
}

void AEONFloorManager::SayUlysseHub()
{
	// ulysseHubLines : les insignes en poche sont rendus. +10 scrap chacun ; tous les 3 rendus, +1 PV max permanent (2 au plus).
	UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	UEONUISubsystem* UI = UEONUISubsystem::Get(this);
	if (!MS || !MS->M() || !UI) { return; }
	UEONSaveGame* M = MS->M();
	const int32 Tot = UE_ARRAY_COUNT(GEvtCrew);
	int32 Pend = 0, Rendus = 0;
	for (const auto& KV : M->Crew) { if (KV.Value == TEXT("poche")) { ++Pend; } else if (KV.Value == TEXT("rendu")) { ++Rendus; } }
	auto L = [](const TCHAR* Ex, const FString& T) { FEONDlgLine D; D.Who = TEXT("ulysse"); D.Ex = Ex; D.T = T; return D; };
	TArray<FEONDlgLine> Lines;
	if (!Pend)
	{
		const int32 Left = Tot - Rendus;
		Lines.Add(L(Left ? TEXT("triste") : TEXT("sourire"), Left ? FString::Printf(TEXT("Il en manque encore %d. Ils sont quelque part dans les étages."), Left) : FString(TEXT("L'équipage repose. Grâce à toi."))));
		UI->SayLines(Lines);
		return;
	}
	const int32 After = FMath::Min(Tot, Rendus + Pend);
	const int32 Gain = FMath::Min(2, After / 3) - FMath::Min(2, Rendus / 3);
	Lines.Add(L(TEXT("triste"), Pend > 1 ? FString::Printf(TEXT("%d insignes... Ils sont revenus à la maison."), Pend) : FString(TEXT("Un insigne... Il est revenu à la maison."))));
	Lines.Add(L(TEXT("sourire"), TEXT("Prends ça. Ce n'est pas grand-chose, mais c'est tout ce que j'ai.")));
	if (Gain > 0) { Lines.Add(L(TEXT("neutre"), TEXT("Et retiens ceci : ta cuve te reconstruira un peu plus solide, désormais."))); }
	if (After >= Tot) { Lines.Add(L(TEXT("triste"), TEXT("Tout l'équipage est là. Je peux enfin leur dire adieu. Merci, soldat."))); }
	else { Lines.Add(L(TEXT("neutre"), FString::Printf(TEXT("Il en reste %d. Continue de descendre."), Tot - After))); }
	UI->SayLines(Lines, [this, Pend]()
	{
		UEONMetaSubsystem* Ms = UEONMetaSubsystem::Get(this);
		if (!Ms || !Ms->M()) { return; }
		const int32 Hb0 = Ms->M()->HpBonus;
		for (auto& KV : Ms->M()->Crew) { if (KV.Value == TEXT("poche")) { KV.Value = TEXT("rendu"); } }
		Ms->Save();   // recalcule les PV bonus (crewSync)
		const int32 Gn = Ms->M()->HpBonus - Hb0;
		AEONCharacter* Jr = GetJira();
		if (Jr && Gn > 0) { Jr->AddMaxHP(Gn); ShowBanner(FString::Printf(TEXT("+%d PV max (permanent)"), Gn), EvtAm); }
		if (Jr)
		{
			Jr->AddScrap(10 * Pend);
			if (AEONCombatDirector* Dd = AEONCombatDirector::Get(GetWorld())) { Dd->AddNum(Jr->GetActorLocation() + FVector(0.f, 0.f, 160.f), FString::Printf(TEXT("+%d scrap"), 10 * Pend), FColor(255, 194, 61), 1.f, -30.f); }
		}
		EONSfx::Play(this, TEXT("SFX_Proto_item"), 0.8f);
	});
}

void AEONFloorManager::OnQuestPickup(EEONPickup Type, const FString& Name)
{
	UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	FEONFloorCell* C = CellAt(Cur);
	if (Type == EEONPickup::Log)
	{
		// readLog : lu sans pause, en surimpression ; on reprend là où on s'était arrêté.
		const int32 Nl = FMath::Max(1, UEONUISubsystem::NumLogs());
		const int32 I = MS && MS->M() ? MS->M()->LogN % Nl : 0;
		LogTxt = UEONUISubsystem::Log(I);
		LogShownN = I + 1;
		LogT0 = FPlatformTime::Seconds();
		if (MS && MS->M()) { ++MS->M()->LogN; MS->Save(); }
		if (C) { C->bLogTaken = true; }
		EONSfx::Play(this, TEXT("SFX_Proto_card"), 0.8f);
	}
	else if (Type == EEONPickup::Doudou)
	{
		bHasDoudou = true;
		if (C) { C->bDoudouTaken = true; }
		EONSfx::Play(this, TEXT("SFX_Proto_item"), 0.8f);
		ShowBanner(TEXT("Une peluche usee... un lapin"), EvtAm);
	}
	else if (Type == EEONPickup::Insigne)
	{
		if (MS && MS->M()) { MS->M()->Crew.Add(Name, TEXT("poche")); MS->Save(); }
		EONSfx::Play(this, TEXT("SFX_Proto_item"), 0.8f);
		ShowBanner(FString::Printf(TEXT("Insigne : %s - a rendre a Ulysse"), *Name), EvtAm);
	}
}

bool AEONFloorManager::GetLogMsg(FString& OutTxt, int32& OutN, float& OutEl) const
{
	if (LogT0 < 0.0) { return false; }
	OutEl = float(FPlatformTime::Seconds() - LogT0);
	if (OutEl > LogTxt.Len() / 40.f + 5.f) { return false; }
	OutTxt = LogTxt;
	OutN = LogShownN;
	return true;
}

bool AEONFloorManager::GetEvtHud(FVector& OutWorld, FString& OutTxt, FLinearColor& OutCol) const
{
	const FEONFloorCell* C = Cells.Find(Cur);
	if (!C || C->Role != TEXT("EVENEMENT") || !Room || !EvtNpc.IsValid()) { return false; }
	if (C->EvtKind == 1 && C->EvtSt == 0) { OutTxt = TEXT("?"); OutCol = FLinearColor(FColor(255, 176, 192)); }
	else if (C->EvtKind == 2 && C->EvtSt < 2) { OutTxt = TEXT("?!"); OutCol = FLinearColor(FColor(255, 61, 85)); }
	else if (C->EvtKind == 2 && C->EvtSt == 2) { OutTxt = TEXT("!"); OutCol = EvtAm; }
	else { return false; }
	OutWorld = Room->RoomPxToWorld(EvtNpcPx(*C), EON::PxToCm(C->EvtKind == 1 ? 18.f : 28.f));
	return true;
}

void AEONFloorManager::TestQuete(const TArray<FString>& Args)
{
	UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	UEONSaveGame* M = MS ? MS->M() : nullptr;
	if (!M || !Args.Num()) { return; }
	const FString A = Args[0].ToLower();
	const int32 V = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
	if (A == TEXT("ulysse")) { M->bUlysse = V != 0; if (!V) { M->Crew.Reset(); } }
	else if (A == TEXT("gamine")) { M->bNewtDone = V != 0; M->NewtEnd.Reset(); }
	else if (A == TEXT("peluche")) { bHasDoudou = true; }
	else if (A == TEXT("poche")) { for (int32 I = 0; I < FMath::Min(V, (int32)UE_ARRAY_COUNT(GEvtCrew)); ++I) { M->Crew.Add(GEvtCrew[I], TEXT("poche")); } }
	else if (A == TEXT("log")) { OnQuestPickup(EEONPickup::Log, FString()); }
	MS->Save();
	UE_LOG(LogEON, Display, TEXT("eon.quete %s : Ulysse %d, gamine %d, peluche %d, equipage %d"), *A, M->bUlysse ? 1 : 0, M->bNewtDone ? 1 : 0, bHasDoudou ? 1 : 0, M->Crew.Num());
}
