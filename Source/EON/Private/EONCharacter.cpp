#include "EONCharacter.h"
#include "EONSfx.h"
#include "GameFramework/PlayerStart.h"
#include "EONPedestal.h"
#include "EON.h"
#include "EONPlayerCameraManager.h"
#include "EONProjectile.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONPickup.h"
#include "Kismet/GameplayStatics.h"
#include "EONTileRoom.h"
#include "EONFloorManager.h"
#include "EONCombatDirector.h"
#include "EONStatics.h"
#include "DrawDebugHelpers.h"
#include "Misc/App.h"
#include "EONEnemy.h"
#include "EONTrainingDummy.h"
#include "EONTuningSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// Démo de locomotion pour les captures vidéo : Jira posée au centre de la COM, parcours automatique
// (idle, cercle en marchant, cercle en courant, allers-retours) en boucle de 16 s. 0 = jeu normal.
static TAutoConsoleVariable<int32> CVarEONAuto(TEXT("eon.auto"), 0,
	TEXT("1 = pilote automatique de test : Jira vise et tire sur l'ennemi le plus proche, tourne autour, puis passe une porte."));
static TAutoConsoleVariable<int32> CVarEONDemo(TEXT("eon.demo"), 0,
	TEXT("1 = démo de locomotion : Jira au centre de la COM, parcours marche / course automatique."));

AEONCharacter::AEONCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	const float Radius = EON::PxToCm(CollisionRadiusPx);
	const float HalfHeight = EON::PxToCm(HeightPx) * 0.5f;
	GetCapsuleComponent()->InitCapsuleSize(Radius, HalfHeight);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->MaxWalkSpeed = EON::PxToCm(WalkSpeedPx);
	// Le lissage de vitesse est fait à la main (k = min(1, dt×16), comme le proto) :
	// la CharacterMovement suit l'entrée lissée sans inertie propre.
	Move->MaxAcceleration = 100000.f;
	Move->BrakingDecelerationWalking = 100000.f;
	Move->GroundFriction = 8.f;
	Move->bConstrainToPlane = true;
	Move->SetPlaneConstraintNormal(FVector::UpVector);

	// Jira en 3D riggée sur le squelette du mannequin UEFN (Saved/Scripts/blender_jira_uefn.py) : locomotion =
	// Animation Blueprint du Game Animation Sample 5.8 (SandboxCharacter_CMC_ABP), alimenté par BP_Jira
	// (interface BPI_SandboxCharacter_Pawn -> UEONStatics::FillAnimProps). Pendant le tir : couche haut du corps
	// (A_Jira_shoot_front depuis spine_01, variable EONTirAlpha). Jira 29 px = 181 cm : échelle ajustée sur HeightPx.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> JiraMesh(TEXT("/Game/EON/Characters/Jira/SK_Jira_UEFN.SK_Jira_UEFN"));
	// L'ABP n'est chargé qu'au BeginPlay : chargé ici (CDO, au démarrage du module), il se compilait avant le plugin
	// DrawDebugLibrary et perdait ses fonctions (15/11).
	USkeletalMeshComponent* Body = GetMesh();
	if (JiraMesh.Succeeded()) { Body->SetSkeletalMeshAsset(JiraMesh.Object); }
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	// Le mannequin regarde vers +Y : -90° pour qu'il regarde vers +X (sens de la visée).
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -HalfHeight), FRotator(0.f, -90.f, 0.f));
	Body->SetRelativeScale3D(FVector(EON::PxToCm(HeightPx) / 198.f));   // SK_Jira mesure 198 cm (casque compris)
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Flash de bouche : L(tip, 66 px, couleur de la greffe, 0,95) pendant 0,06 s.
	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(RootComponent);
	MuzzleLight->SetAttenuationRadius(EON::PxToCm(66.f));
	MuzzleLight->SetIntensity(6000.f);
	MuzzleLight->SetCastShadows(false);
	MuzzleLight->SetVisibility(false);
}

void AEONCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Pose au sol : la contrainte de plan interdit toute vitesse verticale, un départ en l'air restait en chute
	// sans jamais atterrir (Jira bloquée dans le HUB, 15/11). Hors étage en tuiles (HUB : sol à marches),
	// pas de contrainte : Jira suit le sol.
	{
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(EONSpawnGround), true, this);
		const FVector P = GetActorLocation();
		if (GetWorld()->LineTraceSingleByChannel(Hit, P + FVector(0.f, 0.f, 50.f), P - FVector(0.f, 0.f, 5000.f), ECC_WorldStatic, Q))
		{
			SetActorLocation(Hit.ImpactPoint + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
			UE_LOG(LogEON, Display, TEXT("EON_SPAWN sol sur %s à Z=%.0f (départ %s)"), *GetNameSafe(Hit.GetActor()), Hit.ImpactPoint.Z, *P.ToCompactString());
		}
		if (!AEONFloorManager::Get(this)) { GetCharacterMovement()->SetPlaneConstraintEnabled(false); }
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	// BP_Jira a pu garder l'ancien maillage / AnimBP dans ses propriétés sauvegardées.
	if (USkeletalMesh* Uefn = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/EON/Characters/Jira/SK_Jira_UEFN.SK_Jira_UEFN")))
	{
		if (GetMesh()->GetSkeletalMeshAsset() != Uefn) { GetMesh()->SetSkeletalMeshAsset(Uefn); }
	}
	LocoAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Blueprints/SandboxCharacter_CMC_ABP.SandboxCharacter_CMC_ABP_C"));
	if (LocoAnimClass && GetMesh()->GetAnimClass() != LocoAnimClass)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(LocoAnimClass);
	}
	GetCharacterMovement()->MaxWalkSpeed = EON::PxToCm(WalkSpeedPx);
	BaseEnergyMax = EnergyMax;
	BaseDashMax = DashMax;
	Energy = EnergyMax;
	DashCharges = DashMax;
	MeshBaseLocation = GetMesh()->GetRelativeLocation();
	MeshBaseScale = GetMesh()->GetRelativeScale3D();
	ResetRun();
}

AEONPedestal* AEONCharacter::AutoFindPedestal(AEONTileRoom* Rm) const
{
	if (!Rm) { return nullptr; }
	const FVector2D Me = Rm->WorldToRoomPx(GetActorLocation());
	AEONPedestal* Best = nullptr;
	float BestD = TNumericLimits<float>::Max();
	for (TActorIterator<AEONPedestal> It(GetWorld()); It; ++It)
	{
		if (It->IsActorBeingDestroyed() || (It->Price > 0 && Scrap < It->Price)) { continue; }
		const float D = FVector2D::Distance(Me, It->Px);
		if (D < BestD) { BestD = D; Best = *It; }
	}
	return Best;
}

void AEONCharacter::ApplyCassettes()
{
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	if (MS && MS->CasLvl(TEXT("shield")) >= 1) { ShieldMax = 1; Shield = 1; }
}

void AEONCharacter::ResetRun()
{
	// newPlayer : PV max + bonus des insignes (META.hpBonus).
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	MaxHP = StartHP + (MS && MS->M() ? MS->M()->HpBonus : 0);
	HP = MaxHP;
	// Jalon 6 : l'ADN, le scrap et les objets sont perdus à la mort.
	EnergyMax = BaseEnergyMax; DashMax = BaseDashMax; DashCharges = DashMax;
	Adn = 0.f; Lvl = 1; LvPend = 0; LvT = 0.f; LvOpts.Reset(); Scrap = 0; Items.Reset();
	MagR = 40.f; AdnM = ScrapM = DonM = 1.f; RateM = RangeM = ChargeM = DashDistM = CdM = SpdItemM = 1.f;
	Size = Revive = Shield = ShieldMax = Pierce = Bounce = Multi = 0;
	ShieldT = 0.f;
	ApplyCassettes();
	DmgM = 1.f; bBerserk = bStab = bReg = false;
	FallCount = 0;
	// Jalon 4 : état sain, sans armure, énergie pleine, jauge d'ultime vide.
	Mut = 0; MutT = 0.f; RecT = 0.f; PeakMut = 0; ArmorPerm = 0; ShownArmor = 0; ArmorMsgShown.Reset();
	Ult = 0.f; Charge = 0.f; bCharged = false; BurstQ = 0; SpellCd = 0.f; HoloCd = 0.f; DecoyT = 0.f;
	bSpHold = false; bSpMaint = false; bQTE = false; CutinT = 0.f; UltStrikes.Reset();
	Energy = EnergyMax;
	ResetForRoom(GetActorLocation());
}

void AEONCharacter::ResetForRoom(FVector Location)
{
	// loadRoom : P.sx/sy = entrée, vitesse nulle, dash et chute annulés.
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	// Plan contraint à l'horizontale : sans ce passage en marche, un pion posé en l'air resterait « en chute » (ABP en chute).
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SafeLocation = Location;
	bHasSafeLocation = true;
	FallTimer = 0.f;
	bFallingIntoVoid = false;
	DashRemaining = 0.f;
	DecoyT = 0.f;
	BurstQ = 0;
	UltStrikes.Reset();
	RecoilVel = FVector::ZeroVector;
	SmoothMove = FVector2D::ZeroVector;
	GetMesh()->SetRelativeLocation(MeshBaseLocation);
	GetMesh()->SetRelativeScale3D(MeshBaseScale);
}

// ---------------------------------------------------------------- PV, coups, chute

float AEONCharacter::DmgMult() const
{
	// Triche « tue en un coup » : dégâts x1000.
	return DmgM * (bBerserk ? 1.f + 0.35f * Mut : 1.f) * (UEONStatics::GetGameFeel(this).OneShot ? 1000.f : 1.f);
}

void AEONCharacter::HurtPlayer(int32 Amount, FVector SourceWorld)
{
	const UEONTuningSubsystem* Tuning = UEONTuningSubsystem::Get(this);
	const FEONGameFeelRow Feel = Tuning ? Tuning->GetGameFeel() : FEONGameFeelRow();
	if (InvTimer > 0.f || DashRemaining > 0.f || FallTimer > 0.f || Feel.God || IsDead()) { return; }
	// Bouclier (cassette Shield) : absorbe un coup, rechargé à chaque étage.
	if (Shield > 0)
	{
		--Shield;
		ShieldT = 0.f;
		InvTimer = 0.6f;
		// Shield NV3 : nettoyant (A) ou miroir (B).
		if (const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this); MS && MS->CasLvl(TEXT("shield")) >= 3)
		{
			if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->ShieldBurst(MS->CasBr(TEXT("shield")) == TEXT("B")); }
		}
		if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.25f); }
		if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->AddNum(GetActorLocation() + FVector(0.f, 0.f, 120.f), TEXT("Bouclier"), FColor(62, 242, 224), 0.8f, -30.f); }
		return;
	}
	// Armure de mutation : armBlock (20 %) × niveau de chances de bloquer.
	if (ArmorPerm > 0 && FMath::FRand() < Feel.ArmBlock * ArmorPerm)
	{
		InvTimer = 0.5f;
		if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.15f); }
		if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->AddNum(GetActorLocation() + FVector(0.f, 0.f, 120.f), TEXT("Bloque"), FColor(224, 90, 138), 0.8f, -30.f); }
		return;
	}
	// Mutation (stade 1 ou plus) : les coups reçus font double dégât.
	HP -= Amount * (Mut >= 1 ? 2 : 1);
	ShieldT = 0.f;
	InvTimer = 1.f;
	HurtFlash = 0.25f;
	if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.55f); }
	if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(Feel.Hitstop * 1.2f + 30.f); }
	// Recul : 120 px/s à l'opposé de la source.
	FVector D = GetActorLocation() - SourceWorld; D.Z = 0.f;
	RecoilVel += D.GetSafeNormal() * EON::PxToCm(120.f);
	UE_LOG(LogEON, Display, TEXT("EON Jira touchée : %d PV"), HP);
	CheckDeath();
}

void AEONCharacter::CheckDeath()
{
	// Renaissance (objet) : jalon 6. À 0 PV : état « mort » (écran Signal perdu, géré par le directeur de combat).
	if (HP <= 0 && Revive > 0)
	{
		--Revive;
		HP = FMath::CeilToInt(MaxHP / 2.f);
		InvTimer = 2.f;
		Banner(TEXT("Renaissance"), FColor(255, 194, 61));
		return;
	}
	if (HP <= 0)
	{
		HP = 0;
		bFireHeld = false;
		// META.st.runs++, morts++ (checkDeath du proto).
		if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->OnDeath(); }
		GetCharacterMovement()->StopMovementImmediately();
		UE_LOG(LogEON, Display, TEXT("EON Jira : signal perdu"));
	}
}

void AEONCharacter::CheckPlayerFall()
{
	// checkPlayerFall() : on tombe si le centre ET au moins 3 des 4 points autour des pieds sont au-dessus du vide ;
	// sinon, si les 9 points SAFEPTS sont sur du sol, c'est le nouveau point sûr.
	if (DashRemaining > 0.f || FallTimer > 0.f) { return; }
	const AEONTileRoom* Room = AEONTileRoom::FindTileRoomAt(this, GetActorLocation());
	if (!Room) { return; }
	const FVector2D P = Room->WorldToRoomPx(GetActorLocation());
	static const FVector2D FallPts[4] = { {2.5f, -1.5f}, {-2.5f, -1.5f}, {2.5f, 2.f}, {-2.5f, 2.f} };
	static const FVector2D SafePts[9] = { {6, 0}, {-6, 0}, {0, 6}, {0, -6}, {4, 4}, {-4, 4}, {4, -4}, {-4, -4}, {0, 0} };
	if (Room->PitAtPx(P))
	{
		int32 N = 0;
		for (const FVector2D& O : FallPts) { if (Room->PitAtPx(P + O)) { ++N; } }
		if (N >= 3)
		{
			FallTimer = 0.5f;
			FallStart = GetActorLocation();
			bFallingIntoVoid = true;
			RecoilVel = FVector::ZeroVector;
			SmoothMove = FVector2D::ZeroVector;
			GetCharacterMovement()->StopMovementImmediately();
			++FallCount;
			return;
		}
	}
	bool bSafe = true;
	for (const FVector2D& O : SafePts) { if (Room->PitAtPx(P + O)) { bSafe = false; break; } }
	if (bSafe) { SafeLocation = GetActorLocation(); bHasSafeLocation = true; }
}

AEONPlayerCameraManager* AEONCharacter::GetEONCamera() const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	return PC ? Cast<AEONPlayerCameraManager>(PC->PlayerCameraManager) : nullptr;
}

const FEONGreffeRow* AEONCharacter::FindGreffe() const
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	UDataTable* Table = T ? T->GetGreffes() : nullptr;
	return Table ? Table->FindRow<FEONGreffeRow>(GreffeId, TEXT("EON"), false) : nullptr;
}

void AEONCharacter::SetGreffe(FName NewGreffeId)
{
	GreffeId = NewGreffeId;
	CooldownTimer = 0.f; // changer de greffe annule la charge et la salve en cours
	Charge = 0.f;
	bCharged = false;
	BurstQ = 0;
	bWarnedGreffe = false;
}

// ---------------------------------------------------------------- Tick

void AEONCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UEONTuningSubsystem* Tuning = UEONTuningSubsystem::Get(this);
	const FEONGameFeelRow Feel = Tuning ? Tuning->GetGameFeel() : FEONGameFeelRow();
	const float Dt = DeltaSeconds;

	// --- Méta : stade de mutation le plus haut (META.st.maxMut) ; Shield NV2 : recharge après 10 s sans dégâts.
	if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this))
	{
		if (Mut > 0) { MS->OnMut(Mut); }
		if (ShieldMax > 0 && Shield < ShieldMax && MS->CasLvl(TEXT("shield")) >= 2 && !IsDead())
		{
			ShieldT += Dt;
			if (ShieldT > 10.f) { Shield = ShieldMax; ShieldT = 0.f; }
		}
	}

	// --- Démo de locomotion (eon.demo 1) : remplace l'entrée du joueur.
	if (CVarEONDemo.GetValueOnGameThread() > 0)
	{
		if (DemoTime < 0.f && CVarEONDemo.GetValueOnGameThread() == 4) { DemoTime = 0.f; }   // 4 = cercle, sans téléport ni retrait de contrainte (chemin du jeu normal)
		if (DemoTime < 0.f)
		{
			DemoTime = 0.f;
			// Jira posée au point de départ de la carte (PlayerStart : passerelle du HUB ; le centre du C.O.M, ce sont
			// des ordinateurs, on n'y circule pas — Vincent 15/11). Sans PlayerStart : là où elle est.
			FHitResult Hit;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(EONDemo), true, this);
			FVector C = GetActorLocation();
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) { C = It->GetActorLocation(); break; }
			C.Z = 3000.f;
			float Z = 600.f;
			if (GetWorld()->LineTraceSingleByChannel(Hit, C, C - FVector(0.f, 0.f, 6000.f), ECC_WorldStatic, Q))
			{
				Z = Hit.ImpactPoint.Z;
				UE_LOG(LogEON, Display, TEXT("EON_DEMO sol trouvé sur %s à Z=%.0f"), *GetNameSafe(Hit.GetActor()), Z);
			}
			SetActorLocation(FVector(C.X, C.Y, Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
			GetCharacterMovement()->SetPlaneConstraintEnabled(false);   // démo : Jira suit les marches de la COM
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
		DemoTime += Dt;
		// Boucle de 16 s : idle 2 s, 2 tours en marchant (3 s/tour, r ≈ 2,4 m), idle, 1 tour en courant (r ≈ 4 m), idle, allers-retours.
		const float T = FMath::Fmod(DemoTime, 16.f);
		FVector2D M = FVector2D::ZeroVector;
		bool bRun = false;
		auto Circle = [](float t) { const float a = t * UE_TWO_PI / 3.f; return FVector2D(FMath::Cos(a), FMath::Sin(a)); };
		if (T >= 2.f && T < 8.f)         { M = Circle(T - 2.f); }
		else if (T >= 9.f && T < 12.f)   { M = Circle(T - 9.f); bRun = true; }
		else if (T >= 13.f)              { M = FVector2D((T < 13.5f || (T >= 14.5f && T < 15.5f)) ? 1.f : -1.f, 0.f); }
		if (CVarEONDemo.GetValueOnGameThread() == 2) { M = FVector2D::ZeroVector; bRun = false; }   // 2 = immobile au centre
		// 3 = visée fixe vers le haut de l'écran, marche avant / gauche / arrière / droite puis course avant / arrière
		// (2 s chacune, 1 s d'arrêt entre) : teste les pas de côté et le recul des jambes.
		const bool bFixedAim = CVarEONDemo.GetValueOnGameThread() == 3;
		if (bFixedAim)
		{
			static const FVector2D Dirs[6] = { {0.f, 1.f}, {-1.f, 0.f}, {0.f, -1.f}, {1.f, 0.f}, {0.f, 1.f}, {0.f, -1.f} };
			const float T3 = FMath::Fmod(DemoTime, 19.f);
			const int32 I = FMath::FloorToInt((T3 - 1.f) / 3.f);
			const bool bGo = T3 >= 1.f && I < 6 && FMath::Fmod(T3 - 1.f, 3.f) < 2.f;
			M = bGo ? Dirs[I] : FVector2D::ZeroVector;
			bRun = bGo && I >= 4;
		}
		RawMove = M;
		bDashHeld = bRun;
		if (bFixedAim) { RawAimAngle = UE_HALF_PI * -1.f; }   // haut de l'écran = -Y monde
		else if (M.Size() > 0.1f) { RawAimAngle = FMath::Atan2(-M.Y, M.X); }
		if (FMath::Fmod(DemoTime, 0.5f) < Dt)
		{
			UE_LOG(LogEON, Display, TEXT("EON_DEMO t=%.1f pos=%s mode=%d vit=%.0f course=%d"), DemoTime, *GetActorLocation().ToCompactString(),
				(int32)GetCharacterMovement()->MovementMode.GetValue(), GetVelocity().Size2D(), bRunning ? 1 : 0);
		}
	}
	else
	{
		DemoTime = -1.f;
	}
	if (CVarEONAuto.GetValueOnGameThread() > 0 && !IsDead()) { AutoPilot(Dt); }

	// --- QTE de l'ultime : le jeu est figé, le chrono tourne en temps réel.
	const float RealDt = FMath::Min((float)FApp::GetDeltaTime(), 0.1f);
	CutinT = FMath::Max(0.f, CutinT - RealDt);
	if (bQTE) { UpdateQTE(RealDt); return; }

	WorldTime += Dt;
	InvTimer = FMath::Max(0.f, InvTimer - Dt);
	HurtFlash = FMath::Max(0.f, HurtFlash - Dt);

	// --- Mort : plus d'entrée (écran Signal perdu).
	if (IsDead())
	{
		bFireHeld = false;
		RawMove = FVector2D::ZeroVector;
		return;
	}

	// --- Chute dans le vide (0,5 s) : Jira s'enfonce et rétrécit, puis revient au dernier point sûr, −FallDmg PV, 1,2 s d'invulnérabilité.
	if (FallTimer > 0.f)
	{
		FallTimer -= Dt;
		const float K = FMath::Clamp(1.f - FallTimer / 0.5f, 0.f, 1.f);
		GetMesh()->SetRelativeLocation(MeshBaseLocation - FVector(0.f, 0.f, K * K * 420.f));
		GetMesh()->SetRelativeScale3D(MeshBaseScale * (1.f - 0.55f * K));
		GetCharacterMovement()->StopMovementImmediately();
		if (FallTimer <= 0.f)
		{
			FallTimer = 0.f;
			bFallingIntoVoid = false;
			GetMesh()->SetRelativeLocation(MeshBaseLocation);
			GetMesh()->SetRelativeScale3D(MeshBaseScale);
			SetActorLocation(bHasSafeLocation ? SafeLocation : FallStart, false, nullptr, ETeleportType::TeleportPhysics);
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			InvTimer = 1.2f;
			if (!Feel.God && Feel.FallDmg > 0.f)
			{
				HP -= FMath::RoundToInt(Feel.FallDmg);
				HurtFlash = 0.25f;
				if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.4f); }
				CheckDeath();
			}
		}
		return;
	}
	CooldownTimer -= Dt;

	// --- Course (façon Hades) : maintenir le bouton du dash plus de 0,2 s en se déplaçant.
	HoldTimer = (bDashHeld && DashRemaining <= 0.f && !bFallingIntoVoid) ? HoldTimer + Dt : 0.f;
	bRunning = HoldTimer > RunHoldTime && DashRemaining <= 0.f && RawMove.Size() > 0.2f;

	// --- Visée : en course, Jira regarde où elle court ; sinon aide à la visée.
	float A = RawAimAngle;
	if (bRunning)
	{
		A = FMath::Atan2(-RawMove.Y, RawMove.X);
	}
	else
	{
		// Armure de mutation : ×0,4 / 0,6 / 0,8 / 1 selon le niveau.
		static const float ArmAssist[4] = { 0.4f, 0.6f, 0.8f, 1.f };
		A = ApplyAimAssist(RawAimAngle, Feel.AimAssist * ArmAssist[FMath::Clamp(GetArmorLevel(), 0, 3)]);
	}
	// Stade 3 : la visée tremble (±0,175 rad).
	if (Mut >= 3) { A += (FMath::FRand() - 0.5f) * 0.35f; }
	AimAngle = A;

	// --- Dash : recharge une charge à la fois.
	if (DashCharges < DashMax)
	{
		DashRecharge += Dt;
		if (DashRecharge >= DashCooldown * CdM) { ++DashCharges; DashRecharge = 0.f; }
	}

	// --- Mouvement
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (DashRemaining > 0.f)
	{
		DashRemaining -= Dt;
		Move->MaxWalkSpeed = EON::PxToCm(DashSpeedPx) * DashDistM * Feel.DashMul;   // pas de SpeedMul3D : la portée du dash est une règle de terrain (fossés de 2 tuiles)
		AddMovementInput(DashDir, 1.f);
	}
	else
	{
		const float K = FMath::Min(1.f, Dt * 16.f);
		SmoothMove += (RawMove - SmoothMove) * K;
		const float RunMul = bRunning ? Feel.RunMul : 1.f;
		// spdM : ×0,8 au stade 2 de mutation, ×0,5 pendant l'onde maintenue.
		const float SpdM = (Mut >= 2 ? 0.8f : 1.f) * (bSpMaint ? 0.5f : 1.f);
		Move->MaxWalkSpeed = EON::PxToCm(WalkSpeedPx) * Feel.SpeedMul3D * RunMul * SpdM * SpdItemM;
		// Écran : haut = -Y monde. Le recul du tir s'ajoute à l'entrée et s'amortit comme la vitesse.
		FVector Input(SmoothMove.X, -SmoothMove.Y, 0.f);
		Input += RecoilVel / FMath::Max(1.f, Move->MaxWalkSpeed);
		// Stade 3 : Jira dérape (bruit de ±20 px/s sur la vitesse).
		if (Mut >= 3) { Input += FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f) * EON::PxToCm(20.f) / FMath::Max(1.f, Move->MaxWalkSpeed); }
		AddMovementInput(Input, 1.f);
		RecoilVel *= (1.f - K);
	}
	if (DashRemaining > 0.f) { RecoilVel = FVector::ZeroVector; }

	// --- Énergie et mutation (02, sections 6 et 7)
	UpdateMutation(Dt, Feel);

	// --- Arme
	// En course : pas de tir.
	const bool bCanFire = bFireHeld && !bRunning && !bFallingIntoVoid;
	if (const FEONGreffeRow* G = FindGreffe())
	{
		UpdateWeapon(Dt, *G, Feel);
	}
	else if (!bWarnedGreffe)
	{
		bWarnedGreffe = true;
		UE_LOG(LogEON, Warning, TEXT("Greffe '%s' absente de DT_Greffes."), *GreffeId.ToString());
	}
	(void)bCanFire;
	UpdateSpells(Dt, Feel);

	// --- Flash de bouche
	if (MuzzleTimer > 0.f)
	{
		MuzzleTimer -= Dt;
		if (MuzzleTimer <= 0.f) { MuzzleLight->SetVisibility(false); }
	}

	// --- Orientation : Jira regarde où elle vise.
	SetActorRotation(FRotator(0.f, FMath::RadiansToDegrees(AimAngle), 0.f));
	// Couche « tir » du haut du corps dans l'ABP d'Epic (variable EONTirAlpha ajoutée par UEONStatics::AddUpperBodyLayer).
	TirAlpha = FMath::FInterpTo(TirAlpha, bFireHeld ? 1.f : 0.f, DeltaSeconds, 14.f);
	if (UAnimInstance* AI = GetMesh()->GetAnimInstance())
	{
		if (FProperty* P = AI->GetClass()->FindPropertyByName(TEXT("EONTirAlpha")))
		{
			void* TirAddr = P->ContainerPtrToValuePtr<void>(AI);
			if (FDoubleProperty* D = CastField<FDoubleProperty>(P)) { D->SetPropertyValue(TirAddr, TirAlpha); }
			else if (FFloatProperty* F = CastField<FFloatProperty>(P)) { F->SetPropertyValue(TirAddr, TirAlpha); }
		}
	}

	// --- Vide (jalon 2)
	CheckPlayerFall();
}

// ---------------------------------------------------------------- Dash

void AEONCharacter::PressDash()
{
	if (DashCharges <= 0 || DashRemaining > 0.f || bFallingIntoVoid) { return; }

	// Direction = le déplacement en cours, ou la visée si Jira est immobile.
	if (RawMove.Size() >= 0.2f)
	{
		DashDir = FVector(RawMove.X, -RawMove.Y, 0.f).GetSafeNormal();
	}
	else
	{
		DashDir = FVector(FMath::Cos(AimAngle), FMath::Sin(AimAngle), 0.f);
	}
	DashRemaining = DashDuration;
	--DashCharges;
	if (UEONStatics::GetGameFeel(this).InfDash) { DashCharges = DashMax; }
	InvTimer = FMath::Max(InvTimer, DashInvulnerability);
	HoldTimer = 0.f;

	if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.06f); }
	// Traînée de fantômes et poussière : pas encore (MANQUANTS.md §7).
}

// ---------------------------------------------------------------- Visée

float AEONCharacter::ApplyAimAssist(float Raw, float Strength) const
{
	if (Strength <= 0.f) { return Raw; }

	const float Cone = 0.12f + Strength * 0.4f;
	const FVector Arm = GetArmLocation();
	const AEONTrainingDummy* Best = nullptr;
	float BestScore = 1e9f;
	float BestAngle = 0.f;

	for (TActorIterator<AEONTrainingDummy> It(GetWorld()); It; ++It)
	{
		const FVector To = It->GetAimPoint() - Arm;
		const float DistPx = EON::CmToPx(To.Size2D());
		if (DistPx > 220.f) { continue; }
		const float Ang = FMath::Atan2(To.Y, To.X);
		const float Ad = FMath::Abs(FMath::FindDeltaAngleRadians(Raw, Ang));
		if (Ad < Cone && Ad * 60.f + DistPx < BestScore)
		{
			BestScore = Ad * 60.f + DistPx;
			Best = *It;
			BestAngle = Ang;
		}
	}
	// Ennemis (jalon 3) : même règle (moins de 220 px, dans le cône).
	const AEONEnemy* BestEnemy = nullptr;
	for (TActorIterator<AEONEnemy> It(GetWorld()); It; ++It)
	{
		if (!It->IsTargetable()) { continue; }
		const FVector To = It->GetAimPoint() - Arm;
		const float DistPx = EON::CmToPx(To.Size2D());
		if (DistPx > 220.f) { continue; }
		const float Ang = FMath::Atan2(To.Y, To.X);
		const float Ad = FMath::Abs(FMath::FindDeltaAngleRadians(Raw, Ang));
		if (Ad < Cone && Ad * 60.f + DistPx < BestScore)
		{
			BestScore = Ad * 60.f + DistPx;
			BestEnemy = *It;
			BestAngle = Ang;
		}
	}
	if (!Best && !BestEnemy) { return Raw; }
	return Raw + FMath::FindDeltaAngleRadians(Raw, BestAngle) * Strength * 0.8f;
}

FVector AEONCharacter::GetArmLocation() const
{
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	return Feet + FVector(0.f, 0.f, EON::PxToCm(EON::ArmHeightPx));
}

FVector AEONCharacter::GetMuzzleLocation(float Angle) const
{
	// tip(a) : sx = x + face*3 ; bout = (sx,sy) + (cos a, sin a)*12
	const float Face = FMath::Cos(Angle) >= 0.f ? 1.f : -1.f;
	const FVector Shoulder = GetArmLocation() + FVector(EON::PxToCm(Face * 3.f), 0.f, 0.f);
	return Shoulder + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * EON::PxToCm(12.f);
}

// ---------------------------------------------------------------- Tir

void AEONCharacter::Spend(float Cost)
{
	// spend(c) : toute action compte comme dépense (nourrit la mutation) ; l'énergie ne baisse qu'à l'état sain.
	LastSpendTime = WorldTime;
	RecT = 0.f;
	if (Cost <= 0.f || Mut > 0 || UEONStatics::GetGameFeel(this).InfEnergy) { return; }
	Energy -= Cost;
	if (Energy <= 0.f)
	{
		Energy = 0.f;
		Mut = 1;
		MutT = 0.f;
		PeakMut = FMath::Max(PeakMut, 1);
		if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.35f); }
		Banner(TEXT("Mutation - stade 1"), FColor(255, 61, 85));
	}
}

void AEONCharacter::FireAuto(const FEONGreffeRow& G, const FEONGameFeelRow& Feel)
{
	// Cadence : rate × rateMul × 0,8 en mutation.
	const float Rate = G.Rate * RateM * Feel.RateMul * (Mut >= 1 ? 0.8f : 1.f);
	if (Rate <= 0.f) { return; }
	SpawnBullet(G, Feel, 0.f);
	OnShot(G, Feel);
	Spend(G.Cost * Feel.EnergyCost);
	CooldownTimer = 1.f / Rate;
}

void AEONCharacter::SpawnBullet(const FEONGreffeRow& G, const FEONGameFeelRow& Feel, float SpeedOverridePx)
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// fireBullet() : dispersion uniforme dans le cône, vitesse tirée entre 94 % et 106 %.
	const float SpreadRad = FMath::DegreesToRadians(G.Spread * Feel.Spread);
	const float Aa = AimAngle + (FMath::FRand() - 0.5f) * SpreadRad;
	const float SpeedPx = (SpeedOverridePx > 0.f ? SpeedOverridePx : G.Spd) * Feel.BulletSpeed * (0.94f + FMath::FRand() * 0.12f);
	const FVector Dir(FMath::Cos(Aa), FMath::Sin(Aa), 0.f);

	// La balle part du bout du bras dans la direction de l'angle de visée, avant dispersion.
	// Double canon : 2 puis 3 projectiles côte à côte (décalage −3/+3 puis −5/0/+5 px).
	static const float Offs1[] = { 0.f }, Offs2[] = { -3.f, 3.f }, Offs3[] = { -5.f, 0.f, 5.f };
	const float* Offs = Multi >= 2 ? Offs3 : Multi == 1 ? Offs2 : Offs1;
	const int32 NO = Multi >= 2 ? 3 : Multi == 1 ? 2 : 1;
	const FVector Nrm(-FMath::Sin(Aa), FMath::Cos(Aa), 0.f);
	for (int32 I = 0; I < NO; ++I)
	{
		const FTransform Xf(Dir.Rotation(), GetMuzzleLocation(AimAngle) + Nrm * EON::PxToCm(Offs[I]));
		AEONProjectile* B = World->SpawnActorDeferred<AEONProjectile>(
			AEONProjectile::StaticClass(), Xf, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!B) { continue; }
		// Écart 3D éventuel : RangeMul3D allonge la durée de vie sans toucher à la vitesse du proto.
		B->Setup(Dir * EON::PxToCm(SpeedPx), G.Life * RangeM * Feel.RangeMul3D, G.Dmg * DmgMult(), 1.f + Size, G.Col);
		B->SetPierceBounce(Pierce, Bounce);
		B->FinishSpawning(Xf);
	}	EONSfx::Play(this, TEXT("SFX_Jira_Tir"), 0.45f, 3, 0.07f, 0.03f);
}

void AEONCharacter::OnShot(const FEONGreffeRow& G, const FEONGameFeelRow& Feel, float KickOverride)
{
	const FVector Dir(FMath::Cos(AimAngle), FMath::Sin(AimAngle), 0.f);
	const float Kick = KickOverride >= 0.f ? KickOverride : G.Kick;

	// onShot(a, k) : flash 0,06 s, kick caméra k×1,6 px, recul de Jira k×14 px/s, secousse 0,04×k.
	if (AEONPlayerCameraManager* Cam = GetEONCamera())
	{
		Cam->AddKick(-Dir, Kick * 1.6f);
		Cam->AddTrauma(0.04f * Kick);
	}
	RecoilVel -= Dir * EON::PxToCm(Kick * 14.f * Feel.Recoil);

	if (Feel.Flash)
	{
		MuzzleLight->SetWorldLocation(GetMuzzleLocation(AimAngle) + FVector(0.f, 0.f, 20.f));
		MuzzleLight->SetLightColor(G.Col);
		MuzzleLight->SetVisibility(true);
		MuzzleTimer = 0.06f;
	}
	// Douille éjectée, réticule qui s'écarte, son : pas encore (MANQUANTS.md §7 et §8).
}

// ---------------------------------------------------------------- Pilote automatique (tests et vidéos)

void AEONCharacter::AutoPilot(float Dt)
{
	// QTE : une direction, puis neutre, puis la suivante.
	if (bQTE)
	{
		AutoQTEFlip = !AutoQTEFlip;
		const uint8 Want = QTESeq.IsValidIndex(QTEIdx) ? QTESeq[QTEIdx] : 0;
		RawMove = !AutoQTEFlip ? FVector2D::ZeroVector : (Want == 1 ? FVector2D(0, 1) : Want == 2 ? FVector2D(0, -1) : Want == 3 ? FVector2D(-1, 0) : FVector2D(1, 0));
		return;
	}
	// Jalon 4 (eon.auto 2) : change de greffe toutes les 7 s, onde quand un ennemi est près, hologramme, ultime dès qu'il est prêt.
	const bool bJ4 = CVarEONAuto.GetValueOnGameThread() >= 2;
	static float StatT = 0.f;
	if ((StatT += Dt) > 10.f)
	{
		StatT = 0.f;
		UE_LOG(LogEON, Display, TEXT("EON tir : %d balles, %d au but (%.0f %%), %d dans le decor, mutation %d"), AEONProjectile::StatShots, AEONProjectile::StatHits,
			AEONProjectile::StatShots ? 100.f * AEONProjectile::StatHits / AEONProjectile::StatShots : 0.f, AEONProjectile::StatWalls, Mut);
	}
	if (bJ4)
	{
		AutoGreffeT += Dt;
		if (AutoGreffeT > 7.f) { AutoGreffeT = 0.f; CycleGreffe(1); }
		// eon.auto 3 : jauge d'ultime remplie toutes les 20 s, pour tester le QTE et les frappes.
		static float AutoUltT = 0.f;
		if (CVarEONAuto.GetValueOnGameThread() >= 3) { AutoUltT += Dt; if (AutoUltT > 20.f) { AutoUltT = 0.f; Ult = 100.f; } }
		if (Ult >= 100.f) { PressUlt(); }
	}
	const FVector L = GetActorLocation();
	AEONTileRoom* Room = AEONTileRoom::FindTileRoomAt(this, L);
	AEONEnemy* Best = nullptr;
	float BestD = TNumericLimits<float>::Max();
	bool bBestLos = false;
	for (TActorIterator<AEONEnemy> It(GetWorld()); It; ++It)
	{
		if (!It->IsTargetable()) { continue; }
		// Ligne de tir : pas de mur ni de caisse entre Jira et l'ennemi. Une cible visible passe avant une plus proche cachée.
		bool bLos = true;
		if (Room)
		{
			const FVector2D A0 = Room->WorldToRoomPx(L), A1 = Room->WorldToRoomPx(It->GetActorLocation());
			for (float K = 0.05f; K < 0.95f && bLos; K += 0.05f) { if (Room->SolidAtPx(FMath::Lerp(A0, A1, K))) { bLos = false; } }
		}
		const float D = FVector::Dist2D(It->GetActorLocation(), L);
		if ((bLos && !bBestLos) || (bLos == bBestLos && D < BestD)) { BestD = D; Best = *It; bBestLos = bLos; }
	}
	FVector2D W = FVector2D::ZeroVector;
	bool bFire = false;
	bool bAutoSeek = false;
	FVector2D ToTarget = FVector2D::ZeroVector;
	if (Best)
	{
		AutoIdle = 0.f;
		FVector2D D(Best->GetAimPoint() - L);
		const float Dist = D.Size();
		D /= FMath::Max(Dist, 1.f);
		// Visée depuis le bras (comme la souris), avec un peu d'avance sur la cible qui bouge.
		const FVector Arm = GetArmLocation();
		FVector Tp = Best->GetAimPoint();
		const FVector* Prev = AutoLastPos.Find(TWeakObjectPtr<AActor>(Best));
		if (Prev && Dt > 0.f)
		{
			const FVector V = (Best->GetActorLocation() - *Prev) / Dt;
			const float BulletCm = EON::PxToCm(260.f);
			Tp += FVector(V.X, V.Y, 0.f) * FMath::Min(0.6f, FVector::Dist2D(Tp, Arm) / BulletCm);
		}
		AutoLastPos.Reset();
		AutoLastPos.Add(TWeakObjectPtr<AActor>(Best), Best->GetActorLocation());
		RawAimAngle = FMath::Atan2(Tp.Y - Arm.Y, Tp.X - Arm.X);
		// Discipline d'énergie : on arrête de tirer en Mutation II et on attend l'état sain (comme un joueur).
		if (Mut >= 2) { bAutoCool = true; }
		else if (Mut == 0 && Energy > 35.f) { bAutoCool = false; }
		bFire = bBestLos && !bAutoCool;
		const FVector2D Tg(-D.Y * AutoSide, D.X * AutoSide);
		const float Radial = (Dist < 420.f || bAutoCool) ? -1.f : (Dist > 900.f ? 1.f : 0.f);
		W = (Tg + D * Radial).GetSafeNormal();
		ToTarget = D;
		if (!bBestLos) { W = D; bAutoSeek = true; }   // cible cachée : on va vers elle en contournant
		// Cible de l'autre côté d'un fossé : on y va tout droit (et on dashe au bord).
		if (Room)
		{
			const FVector2D A0 = Room->WorldToRoomPx(L), A1 = Room->WorldToRoomPx(Best->GetActorLocation());
			for (float K = 0.1f; K < 1.f; K += 0.05f) { if (Room->PitAtPx(FMath::Lerp(A0, A1, K))) { W = D; break; } }
		}
		if (bJ4 && Dist < 380.f && SpellCd <= 0.f && !bSpHold) { PressSpell(); bSpellHeld = false; }
		if (bJ4 && HoloCd <= 0.f && Dist < 600.f) { PressHolo(); }
		AutoSwap += Dt;
		if (AutoSwap > 2.6f) { AutoSwap = 0.f; AutoSide = -AutoSide; }
	}
	else if (AEONPedestal* Ped = AutoFindPedestal(Room))
	{
		// Un socle gratuit, ou payant si le scrap suffit : on y va (comme un joueur qui ramasse).
		const FVector2D Me = Room->WorldToRoomPx(L);
		W = (Ped->Px + FVector2D(0.f, 4.f) - Me).GetSafeNormal();
		ToTarget = W;
		bAutoSeek = true;
	}
	else if (AEONFloorManager* FM = AEONFloorManager::Get(this); FM && Room && (FM->HasCardInRoom() || (FM->IsFloorDone() && FM->GetCurrentRole() == TEXT("DEPART") && !FM->IsNearComTerminal())))
	{
		// La carte rouge (ou la sphère du C.O.M quand le levier est actif) : droit dessus, dash au-dessus du fossé.
		const FVector2D Me = Room->WorldToRoomPx(L);
		W = (FM->GetCardPx() - Me).GetSafeNormal();
		ToTarget = W;
		bAutoSeek = true;
	}
	else if (Room && !Room->IsLocked())
	{
		// Salle calme : on laisse 2 s aux vagues pour arriver, puis on passe une porte.
		AutoIdle += Dt;
		if (AutoIdle > 2.f)
		{
			AutoIdle = 0.f;
			if (AEONFloorManager* F = AEONFloorManager::Get(this)) { F->AutoExit(); }
		}
	}
	// Ni vide ni mur devant : sinon on change de sens.
	if (Room && !W.IsZero())
	{
		const FVector2D Ahead = Room->WorldToRoomPx(L + FVector(W.X, W.Y, 0.f) * 110.f);
		// Fossé étroit (sol ferme 3,3 m plus loin) : on le franchit au dash, comme le joueur.
		const FVector2D Dd = ToTarget.IsZero() ? W : ToTarget;
		const FVector2D Land = Room->WorldToRoomPx(L + FVector(Dd.X, Dd.Y, 0.f) * 330.f);
		if (Room->PitAtPx(Ahead) && !Room->SolidAtPx(Ahead) && !Room->PitAtPx(Land) && !Room->SolidAtPx(Land)) { W = Dd; PressDash(); }
		else if (bAutoSeek && Room->SolidAtPx(Ahead))
		{
			// Un obstacle sur le chemin de la carte : on le contourne (premier angle libre).
			for (const float Ang : { 60.f, -60.f, 90.f, -90.f, 135.f, -135.f })
			{
				const FVector2D R = W.GetRotated(Ang);
				const FVector2D A2 = Room->WorldToRoomPx(L + FVector(R.X, R.Y, 0.f) * 110.f);
				if (!Room->SolidAtPx(A2) && !Room->PitAtPx(A2)) { W = R; break; }
			}
		}
		else if (Room->PitAtPx(Ahead) || Room->SolidAtPx(Ahead))
		{
			AutoSide = -AutoSide;
			AutoSwap = 0.f;
			W = -W;
		}
	}
	RawMove = FVector2D(W.X, -W.Y);
	// Rafale / Laser ne partent qu'au relâchement : on lâche la gâchette dès que la charge est pleine.
	if (bFire && bCharged) { bFire = false; }
	bFireHeld = bFire;
}

// ---------------------------------------------------------------- Jalon 4 : greffes, mutation, sorts, ultime

namespace
{
	const FColor ColRed(255, 61, 85), ColCy(62, 242, 224), ColAm(255, 194, 61), ColMg(255, 61, 139), ColDim(143, 138, 189), ColArm(224, 90, 138);
	float AngDiff(float A, float B) { return FMath::FindDeltaAngleRadians(B, A); }
	/** Copie de la liste des ennemis visables (un coup peut en tuer pendant le parcours). */
	TArray<AEONEnemy*> LiveEnemies(UWorld* World)
	{
		TArray<AEONEnemy*> Out;
		if (AEONCombatDirector* D = AEONCombatDirector::Get(World))
		{
			for (AEONEnemy* E : D->GetEnemies()) { if (E && E->IsTargetable()) { Out.Add(E); } }
		}
		return Out;
	}
}

void AEONCharacter::Banner(const FString& Text, const FColor& Col) const
{
	if (AEONFloorManager* F = AEONFloorManager::Get(this)) { F->ShowBanner(Text, FLinearColor(Col)); }
}

TArray<FName> AEONCharacter::GetGreffeSet() const
{
	TArray<TPair<int32, FName>> L;
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	// LOCK_G : une greffe verrouillée n'est nulle part (Jira démarre avec Standard et Rafale).
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	if (UDataTable* Table = T ? T->GetGreffes() : nullptr)
	{
		Table->ForeachRow<FEONGreffeRow>(TEXT("EON"), [&L, MS](const FName& Key, const FEONGreffeRow& R) { if (R.JiraOrder > 0 && (!MS || MS->GreffeOk(Key))) { L.Add({ R.JiraOrder, Key }); } });
	}
	L.Sort([](const TPair<int32, FName>& A, const TPair<int32, FName>& B) { return A.Key < B.Key; });
	TArray<FName> Out;
	for (const auto& P : L) { Out.Add(P.Value); }
	if (!Out.Num()) { Out.Add(TEXT("standard")); }
	return Out;
}

void AEONCharacter::SelectGreffeSlot(int32 Slot)
{
	const TArray<FName> S = GetGreffeSet();
	if (S.IsValidIndex(Slot) && S[Slot] != GreffeId)
	{
		SetGreffe(S[Slot]);
		const FEONGreffeRow* G = FindGreffe();
		Banner(G ? G->DisplayName : S[Slot].ToString(), ColCy);
	}
}

void AEONCharacter::CycleGreffe(int32 Step)
{
	const TArray<FName> S = GetGreffeSet();
	const int32 I = FMath::Max(0, S.IndexOfByKey(GreffeId));
	SelectGreffeSlot((I + Step + S.Num()) % S.Num());
}

float AEONCharacter::GetEnergyCap() const
{
	const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
	return EnergyMax * (1.f - F.ArmCost * ArmorPerm);
}

float AEONCharacter::GetCharge01() const
{
	const FEONGreffeRow* G = FindGreffe();
	const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
	const float Full = (G && G->Charge > 0.f) ? G->Charge * ChargeM / FMath::Max(0.01f, F.RateMul) : 1.f;
	return FMath::Clamp(Charge / Full, 0.f, 1.f);
}

AEONTileRoom* AEONCharacter::GetRoomPx(FVector2D& OutJira, FVector2D& OutTip, float A) const
{
	AEONTileRoom* Room = AEONTileRoom::FindTileRoomAt(this, GetActorLocation());
	if (!Room) { return nullptr; }
	OutJira = Room->WorldToRoomPx(GetActorLocation());
	OutTip = OutJira + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 12.f;   // tip(a) : 12 px devant Jira
	return Room;
}

void AEONCharacter::UpdateMutation(float Dt, const FEONGameFeelRow& Feel)
{
	const float Cap = GetEnergyCap();
	const int32 MaxStage = bReg ? 1 : 3;
	const float Dur = Feel.MutDur * (bStab ? 2.f : 1.f) * FMath::Max(0.2f, 1.f - Feel.ArmMut * ArmorPerm);
	if (Mut > 0)
	{
		if (WorldTime - LastSpendTime < 2.5f)
		{
			MutT += Dt;
			if (MutT > Dur && Mut < MaxStage)
			{
				++Mut;
				MutT = 0.f;
				PeakMut = FMath::Max(PeakMut, Mut);
				if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.35f); }
				Banner(FString::Printf(TEXT("Mutation - stade %d"), Mut), ColRed);
			}
		}
		else
		{
			RecT += Dt;
			if (RecT > 1.2f)
			{
				--Mut;
				RecT = 0.f;
				MutT = 0.f;
				if (Mut == 0)
				{
					// État sain : 30 % d'énergie ; l'armure garde la trace du stade le plus haut (+1 niveau au plus).
					Energy = Cap * 0.3f;
					const int32 Np = FMath::Max(ArmorPerm, FMath::Min(ArmorPerm + 1, PeakMut));
					PeakMut = 0;
					if (Np > ArmorPerm) { ArmorPerm = Np; Banner(TEXT("Etat sain - l'armure reste"), ColAm); }
					else { Banner(TEXT("Etat sain"), ColCy); }
				}
			}
		}
	}
	else if (WorldTime - LastSpendTime > EnergyRegenDelay)
	{
		Energy = FMath::Min(Cap, Energy + EnergyRegen * Dt);
	}
	Energy = FMath::Min(Energy, GetEnergyCap());

	// Le corps change avec le niveau d'armure (mesh : MANQUANTS) ; bannière à la première montée de chaque niveau.
	const int32 Lv = GetArmorLevel();
	if (Lv != ShownArmor)
	{
		const bool bUp = Lv > ShownArmor;
		ShownArmor = Lv;
		if (bUp)
		{
			if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.3f); }
			HurtFlash = FMath::Max(HurtFlash, 0.08f);
			if (!ArmorMsgShown.Contains(Lv))
			{
				ArmorMsgShown.Add(Lv);
				static const TCHAR* Msg[4] = { TEXT(""), TEXT("Le bras se transforme"), TEXT("L'armure pousse"), TEXT("Casque : ciblage assiste en ligne") };
				Banner(Msg[FMath::Clamp(Lv, 0, 3)], ColArm);
			}
		}
	}
}

void AEONCharacter::UpdateWeapon(float Dt, const FEONGreffeRow& G, const FEONGameFeelRow& Feel)
{
	const bool bFire = bFireHeld && !bRunning && !bFallingIntoVoid;
	const float Rate = FMath::Max(0.01f, (G.Rate > 0.f ? G.Rate : 1.f) * RateM * Feel.RateMul * (Mut >= 1 ? 0.8f : 1.f));
	const FName Ty = G.Type;
	if (Ty == TEXT("auto"))
	{
		if (bFire && CooldownTimer <= 0.f) { FireAuto(G, Feel); }
	}
	else if (Ty == TEXT("spread"))
	{
		if (bFire && CooldownTimer <= 0.f)
		{
			for (int32 I = 0; I < G.Pellets; ++I) { SpawnBullet(G, Feel, G.Spd * (0.8f + FMath::FRand() * 0.4f)); }
			OnShot(G, Feel);
			Spend(G.Cost * Feel.EnergyCost);
			CooldownTimer = 1.f / Rate;
			if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(Feel.Hitstop * 0.15f); }
		}
	}
	else if (Ty == TEXT("melee"))
	{
		if (bFire && CooldownTimer <= 0.f) { DoSlash(AimAngle, G, Feel); CooldownTimer = 1.f / Rate; }
	}
	else if (Ty == TEXT("lash"))
	{
		if (bFire && CooldownTimer <= 0.f) { DoLash(AimAngle, G, Feel); CooldownTimer = 1.f / Rate; }
	}
	else if (Ty == TEXT("burst") || Ty == TEXT("beam"))
	{
		const float Full = G.Charge * ChargeM / FMath::Max(0.01f, Feel.RateMul);
		if (bFire && BurstQ <= 0)
		{
			Charge = FMath::Min(Full, Charge + Dt);
			if (Charge >= Full && !bCharged) { bCharged = true; }
		}
		else if (Charge > 0.f)
		{
			const float Ratio = Charge / FMath::Max(0.01f, Full);
			if (!bFallingIntoVoid)
			{
				if (Ty == TEXT("burst")) { BurstQ = FMath::RoundToInt(G.MinShots + (G.MaxShots - G.MinShots) * Ratio); BurstT = 0.f; }
				else if (Ratio > 0.2f) { DoBeam(AimAngle, Ratio, G, Feel); }
			}
			Charge = 0.f;
			bCharged = false;
		}
	}
	if (BurstQ > 0)
	{
		BurstT -= Dt;
		if (BurstT <= 0.f)
		{
			SpawnBullet(G, Feel, 0.f);
			OnShot(G, Feel);
			Spend(G.Cost * Feel.EnergyCost);
			--BurstQ;
			BurstT = 0.045f;
		}
	}
}

void AEONCharacter::DoBeam(float A, float Ratio, const FEONGreffeRow& G, const FEONGameFeelRow& Feel)
{
	FVector2D J, T;
	AEONTileRoom* Room = GetRoomPx(J, T, A);
	const FVector2D D(FMath::Cos(A), FMath::Sin(A));
	float Len = 0.f;
	FVector2D P = T;
	const float LMax = G.Len * RangeM;
	if (Room) { while (Len < LMax && !Room->SolidAtPx(P)) { P += D * 2.f; Len += 2.f; } }
	else { Len = LMax; }
	const float W = 2.f + Size * 1.5f + Ratio * 2.f;
	const float Dmg = FMath::Max(1.f, FMath::RoundToFloat(G.Dmg * Ratio * DmgMult() * 10.f) / 10.f);
	if (Room)
	{
		for (AEONEnemy* E : LiveEnemies(GetWorld()))
		{
			const FVector2D Ex = E->Pos - T;
			const float Proj = FVector2D::DotProduct(Ex, D);
			if (Proj < 0.f || Proj > Len) { continue; }
			if (FMath::Abs(Ex.X * D.Y - Ex.Y * D.X) < E->Ehr() + E->Ehh() * 0.35f + W) { E->HitPx(Dmg, D.X, D.Y, 1.6f); }
		}
	}
	// Rayon (placeholder : ligne de débogage, 0,14 s)
	const FVector From = GetMuzzleLocation(A);
	const FVector To = From + FVector(D.X, D.Y, 0.f) * EON::PxToCm(Len);
	DrawDebugLine(GetWorld(), From, To, G.Col.ToFColor(true), false, 0.14f, 0, EON::PxToCm(W));
	EONSfx::Play(this, TEXT("SFX_Jira_Laser"), 0.5f + 0.4f * Ratio, 0, 0.04f);
	OnShot(G, Feel, G.Kick * (0.5f + Ratio * 0.5f));
	if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.15f * Ratio); }
	if (Ratio > 0.95f) { if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(Feel.Hitstop * 0.5f); } }
	Spend(G.Cost * Feel.EnergyCost * (0.4f + 0.6f * Ratio));
}

void AEONCharacter::DoSlash(float A, const FEONGreffeRow& G, const FEONGameFeelRow& Feel)
{
	FVector2D J, T;
	AEONTileRoom* Room = GetRoomPx(J, T, A);
	const float R = G.Range * (1.f + 0.15f * Size) * FMath::Sqrt(RangeM), Arc = G.Arc + Size * 0.15f;
	if (G.Cost > 0.f) { Spend(G.Cost * Feel.EnergyCost); }   // la Lame ne coûte rien et ne nourrit pas la mutation
	if (Room)
	{
		for (AEONEnemy* E : LiveEnemies(GetWorld()))
		{
			const FVector2D V = E->Pos - J;
			if (V.Size() < R + E->Ehr() + E->Ehh() * 0.3f && FMath::Abs(AngDiff(FMath::Atan2(V.Y, V.X), A)) < Arc * 0.5f)
			{
				E->HitPx(G.Dmg * DmgMult(), FMath::Cos(A), FMath::Sin(A), 2.f);
			}
		}
		// Détruit les tirs ennemis dans l'arc (repère écran : bras à 9 px de haut).
		if (AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld()))
		{
			const FVector2D C = J - FVector2D(0.f, 9.f);
			Dir->ClearBullets([C, R, Arc, A](const FVector2D& B)
			{
				const FVector2D V = B - C;
				return V.Size() < R + 3.f && FMath::Abs(AngDiff(FMath::Atan2(V.Y, V.X), A)) < Arc * 0.5f;
			});
		}
	}
	// Petite avancée (40 px/s), kick caméra vers l'avant.
	const FVector Fw(FMath::Cos(A), FMath::Sin(A), 0.f);
	RecoilVel += Fw * EON::PxToCm(40.f);
	if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddKick(Fw, 2.f); }
	// Arc (placeholder : lignes de débogage, 0,12 s)
	const FVector C3 = GetActorLocation() - FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() - 30.f);
	const float Rc = EON::PxToCm(R + 4.f);
	for (int32 I = 0; I < 8; ++I)
	{
		const float A0 = A - Arc * 0.5f + Arc * I / 8.f, A1 = A - Arc * 0.5f + Arc * (I + 1) / 8.f;
		DrawDebugLine(GetWorld(), C3 + FVector(FMath::Cos(A0), FMath::Sin(A0), 0.f) * Rc, C3 + FVector(FMath::Cos(A1), FMath::Sin(A1), 0.f) * Rc, G.Col.ToFColor(true), false, 0.12f, 0, 6.f);
	}
}

void AEONCharacter::DoLash(float A, const FEONGreffeRow& G, const FEONGameFeelRow& Feel)
{
	FVector2D J, T;
	AEONTileRoom* Room = GetRoomPx(J, T, A);
	const FVector2D D(FMath::Cos(A), FMath::Sin(A));
	float Len = 0.f;
	FVector2D P = T;
	const float LashL = G.Range * FMath::Sqrt(RangeM) * (1.f + 0.1f * Size);
	if (Room) { while (Len < LashL && !Room->SolidAtPx(P)) { P += D * 2.f; Len += 2.f; } }
	int32 Hits = 0;
	if (Room)
	{
		for (AEONEnemy* E : LiveEnemies(GetWorld()))
		{
			const FVector2D Ex = E->Pos - T;
			const float Proj = FVector2D::DotProduct(Ex, D);
			if (Proj < 0.f || Proj > Len + E->Ehr()) { continue; }
			if (FMath::Abs(Ex.X * D.Y - Ex.Y * D.X) < E->Ehr() + G.W) { E->HitPx(G.Dmg * DmgMult(), -D.X, -D.Y, G.Pull); ++Hits; }   // ramène vers Jira
		}
	}
	const FVector From = GetMuzzleLocation(A);
	DrawDebugLine(GetWorld(), From, From + FVector(D.X, D.Y, 0.f) * EON::PxToCm(Len), G.Col.ToFColor(true), false, 0.18f, 0, 8.f);
	OnShot(G, Feel);
	Spend(G.Cost * Feel.EnergyCost);
	if (Hits) { if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(Feel.Hitstop * 0.35f); } }
}

void AEONCharacter::Shockwave(float RPx, float Dmg, float Kn, bool bBig)
{
	FVector2D J, T;
	AEONTileRoom* Room = GetRoomPx(J, T, 0.f);
	if (Room)
	{
		const FVector2D C = J - FVector2D(0.f, 8.f);
		for (AEONEnemy* E : LiveEnemies(GetWorld()))
		{
			const FVector2D V(E->Pos.X - C.X, E->Ecy() - C.Y);
			const float D = V.Size();
			if (D < RPx + E->Ehr()) { E->HitPx(Dmg, V.X / FMath::Max(D, 1.f), V.Y / FMath::Max(D, 1.f), Kn); }
		}
		if (AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld()))
		{
			Dir->ClearBullets([C, RPx](const FVector2D& B) { return FVector2D::Distance(B, C) < RPx; });
		}
	}
	// Anneau (placeholder : cercle de débogage)
	const FVector C3 = GetActorLocation() - FVector(0.f, 0.f, GetSimpleCollisionHalfHeight() - 20.f);
	DrawDebugCircle(GetWorld(), C3, EON::PxToCm(RPx), 48, ColMg, false, bBig ? 0.3f : 0.18f, 0, bBig ? 10.f : 5.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	if (bBig)
	{
		const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
		if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.25f); }
		if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(F.Hitstop * 0.5f); }
	}
}

void AEONCharacter::CastHolo()
{
	if (HoloCd > 0.f || bFallingIntoVoid) { return; }
	if (AreSpellsBlocked()) { Banner(TEXT("Spells bloques"), ColRed); return; }
	DecoyLoc = GetActorLocation();
	DecoyT = 5.f;
	DecoyHp = 8.f;
	Spend(28.f);
	HoloCd = 7.f * CdM;
}

void AEONCharacter::UpdateSpells(float Dt, const FEONGameFeelRow& Feel)
{
	SpellCd -= Dt;
	HoloCd -= Dt;
	const bool bBlk = AreSpellsBlocked();
	// Onde : appui court = onde de choc ; maintenu plus de 0,25 s = onde continue.
	if (bSpellPress && !bSpHold && !bFallingIntoVoid)
	{
		if (bBlk) { Banner(TEXT("Spells bloques"), ColRed); }
		else { bSpHold = true; SpT = 0.f; bSpMaint = false; }
	}
	bSpellPress = false;
	if (bSpHold)
	{
		SpT += Dt;
		if (!bSpellHeld || bBlk || bFallingIntoVoid)
		{
			if (!bSpMaint && SpellCd <= 0.f && !bBlk) { Shockwave(56.f, 1.f * DmgMult(), 3.4f, true); Spend(18.f); SpellCd = 0.9f * CdM; }
			bSpHold = false;
			bSpMaint = false;
		}
		else if (SpT > 0.25f)
		{
			if (!bSpMaint) { bSpMaint = true; SpPulse = 0.f; }
			SpPulse -= Dt;
			Spend(30.f * Dt);
			if (SpPulse <= 0.f) { Shockwave(44.f, 1.2f * DmgMult(), 1.7f, false); SpPulse = 0.22f; }
		}
	}
	if (bHoloPress) { CastHolo(); }
	bHoloPress = false;
	if (bUltPress)
	{
		if (Ult >= 100.f && !bFallingIntoVoid) { StartQTE(); }
		else if (Ult < 100.f) { Banner(FString::Printf(TEXT("Ultime : charge %d%%"), FMath::FloorToInt(Ult)), ColDim); }
	}
	bUltPress = false;

	// Hologramme : 5 s ou 8 PV. Placeholder : capsule cyan qui clignote.
	if (DecoyT > 0.f)
	{
		DecoyT -= Dt;
		if (DecoyT <= 0.f || DecoyHp <= 0.f) { DecoyT = 0.f; }
		else if (FMath::Fmod(WorldTime * 20.f, 2.f) < 1.4f)
		{
			DrawDebugCapsule(GetWorld(), DecoyLoc, GetSimpleCollisionHalfHeight(), EON::PxToCm(6.f), FQuat::Identity, ColCy, false, -1.f, 0, 3.f);
		}
	}

	// Frappes de l'ultime : un rayon par ennemi, l'un après l'autre. 14 dégâts (30 sur un boss : jalon 7).
	for (int32 I = UltStrikes.Num() - 1; I >= 0; --I)
	{
		FUltStrike& U = UltStrikes[I];
		U.T -= Dt;
		if (U.T > 0.f) { continue; }
		AEONEnemy* E = U.E.Get();
		UltStrikes.RemoveAt(I);
		if (!E || E->IsDead()) { continue; }
		const FVector At = E->GetActorLocation();
		DrawDebugLine(GetWorld(), At + FVector(0.f, 0.f, 3000.f), At, ColCy, false, 0.25f, 0, 40.f);
		EONSfx::Play(this, TEXT("SFX_Jira_Ultime_Rayon"), 0.7f, 0, 0.08f);
		E->HitPx(E->IsBoss() ? 30.f : 14.f, 0.f, 1.f, 2.5f);
		if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.35f); }
	}
}

void AEONCharacter::StartQTE()
{
	QTESeq.Reset();
	for (int32 I = 0; I < 5; ++I) { QTESeq.Add((uint8)FMath::RandRange(1, 4)); }   // 1 haut, 2 bas, 3 gauche, 4 droite
	QTEIdx = 0;
	QTET = 3.2f;
	bQTE = true;
	bFireHeld = false;
	QTEPrevDir = 255;   // il faut d'abord relâcher la direction
	if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.1f); }
}

void AEONCharacter::UpdateQTE(float RealDt)
{
	// Jeu figé pendant le QTE (state = 'qte' du proto).
	if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(120.f); }
	QTET -= RealDt;
	uint8 Dir = 0;
	if (RawMove.Size() > 0.6f)
	{
		Dir = FMath::Abs(RawMove.X) > FMath::Abs(RawMove.Y) ? (RawMove.X > 0.f ? 4 : 3) : (RawMove.Y > 0.f ? 1 : 2);
	}
	else if (RawMove.Size() > 0.3f && QTEPrevDir != 255) { Dir = QTEPrevDir; }
	if (QTEPrevDir == 255)
	{
		if (Dir == 0) { QTEPrevDir = 0; }
	}
	else if (Dir && Dir != QTEPrevDir)
	{
		if (QTESeq.IsValidIndex(QTEIdx) && Dir == QTESeq[QTEIdx])
		{
			if (++QTEIdx >= QTESeq.Num()) { bQTE = false; UltFire(); return; }
		}
		else
		{
			bQTE = false;
			Ult = 50.f;
			if (AEONPlayerCameraManager* Cam = GetEONCamera()) { Cam->AddTrauma(0.3f); }
			Banner(TEXT("Sequence ratee"), ColRed);
			return;
		}
		QTEPrevDir = Dir;
	}
	else if (!Dir) { QTEPrevDir = 0; }
	if (QTET <= 0.f) { bQTE = false; Ult = 50.f; Banner(TEXT("Trop lent"), ColRed); }
}

void AEONCharacter::UltFire()
{
	Ult = 0.f;
	CutinT = 1.1f;
	if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(1000.f); }
	int32 K = 0;
	for (AEONEnemy* E : LiveEnemies(GetWorld())) { FUltStrike S; S.E = E; S.T = 0.15f + K * 0.09f; UltStrikes.Add(S); ++K; }
	Banner(TEXT("Protocole thoracique"), ColCy);
}

// ---------------------------------------------------------------- Jalon 6 : ADN, niveaux, objets

void AEONCharacter::GainADN(float V)
{
	Adn += V * AdnM;
	while (Adn >= GetADNNeed())
	{
		Adn -= GetADNNeed();
		++Lvl;
		++LvPend;
		Banner(FString::Printf(TEXT("Niveau %d"), Lvl), FColor(138, 240, 122));
	}
}

void AEONCharacter::PickupEnergy(float V)
{
	if (Mut == 0) { Energy = FMath::Min(GetEnergyCap(), Energy + V); }
}

void AEONCharacter::TakeItem(FName Id)
{
	const FEONItemDef* It = EONItems::Find(Id);
	if (!It) { return; }
	const FString S = Id.ToString();
	if (S == TEXT("d_hp")) { ++MaxHP; HP = FMath::Min(MaxHP, HP + 1); }
	else if (S == TEXT("d_spd")) { SpdItemM *= 1.08f; }
	else if (S == TEXT("d_en")) { EnergyMax += 15.f; if (!Mut) { Energy += 15.f; } }
	else if (S == TEXT("d_dmg")) { DmgM *= 1.15f; }
	else if (S == TEXT("d_cd")) { CdM *= 0.88f; }
	else if (S == TEXT("p_rate")) { RateM *= 1.12f; }
	else if (S == TEXT("p_thick")) { ++Size; }
	else if (S == TEXT("p_range")) { RangeM *= 1.2f; }
	else if (S == TEXT("p_charge")) { ChargeM *= 0.8f; }
	else if (S == TEXT("p_dashd")) { DashDistM *= 1.25f; }
	else if (S == TEXT("p_dashc")) { ++DashMax; ++DashCharges; }
	else if (S == TEXT("p_revive")) { ++Revive; }
	else if (S == TEXT("c_shield")) { ShieldMax = 1; Shield = 1; if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->CasUnlock(TEXT("shield")); } }
	else if (S == TEXT("d_don")) { DonM *= 1.3f; }
	else if (S == TEXT("e_berserk")) { bBerserk = true; }
	else if (S == TEXT("e_stab")) { bStab = true; }
	else if (S == TEXT("e_reg")) { bReg = true; }
	else if (S == TEXT("n_kit")) { HP = FMath::Min(MaxHP, HP + 3); }
	else if (S == TEXT("n_aimant")) { MagR += 30.f; }
	else if (S == TEXT("n_plaque")) { MaxHP += 2; HP += 2; SpdItemM *= 0.94f; }
	else if (S == TEXT("n_adn")) { AdnM *= 1.25f; }
	else if (S == TEXT("n_scrap")) { ScrapM *= 1.8f; }
	else if (S == TEXT("j_pierce")) { ++Pierce; }
	else if (S == TEXT("j_bounce")) { ++Bounce; }
	else if (S == TEXT("j_multi")) { Multi = FMath::Min(2, Multi + 1); }
	else if (It->Cat == TEXT("GREFFE")) { SetGreffe(It->Greffe); }
	// k_* (Griffe) : greffe du Cobaye, perso non jouable pour l'instant (jalon 8).
	Items.Add(Id);
	Banner(It->Name, EONItems::Color(It->Rar).ToFColor(true));
	UE_LOG(LogEON, Display, TEXT("EON objet pris : %s (%s)"), *It->Name, *Id.ToString());
}

void AEONCharacter::PickLevel(int32 I)
{
	if (!LvOpts.IsValidIndex(I) || LvOpenT < 0.35f) { return; }
	const FName Id = LvOpts[I];
	LvOpts.Reset();
	--LvPend;
	LvT = 0.5f;
	UGameplayStatics::SetGamePaused(this, false);
	TakeItem(Id);
}

void AEONCharacter::TickLevelUp(float RealDt)
{
	// Le jeu est en pause pendant le choix : le temps du monde est figé, on prend le temps réel.
	RealDt = FMath::Min((float)FApp::GetDeltaTime(), 0.1f);
	if (IsLevelUpOpen())
	{
		LvOpenT += RealDt;
		// Pilote automatique : choisit une carte au hasard.
		if (CVarEONAuto.GetValueOnGameThread() > 0 && LvOpenT > 0.9f) { PickLevel(FMath::RandRange(0, LvOpts.Num() - 1)); }
		return;
	}
	if (LvPend <= 0 || IsDead() || bFallingIntoVoid || bQTE || UGameplayStatics::IsGamePaused(this)) { if (LvT < 0.f) { LvT = 0.f; } return; }
	// Jamais en combat : salle nettoyée, plus d'ennemi, plus d'ADN au sol, puis 0,6 s.
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	const AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld());
	const bool bQuiet = (!F || F->IsCurrentRoomCleared()) && (!D || D->IsQuiet()) && !AEONPickup::AnyAdn(GetWorld()) && (!F || !F->IsComTravelling());
	if (!bQuiet) { if (LvT < 0.f) { LvT = 0.f; } return; }
	LvT -= RealDt;
	if (LvT <= -0.6f)
	{
		LvOpts = EONItems::Roll(3, this, true);
		LvSel = 0;
		LvOpenT = 0.f;
		if (LvOpts.Num()) { UGameplayStatics::SetGamePaused(this, true); }
		else { --LvPend; }
	}
}
