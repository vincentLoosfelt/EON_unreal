#include "EONEnemy.h"
#include "Camera/PlayerCameraManager.h"
#include "EONCharacter.h"
#include "EON.h"
#include "EONCombatDirector.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EONStatics.h"
#include "EONTileRoom.h"
#include "EONTuningSubsystem.h"
#include "EONSfx.h"
#include "EONFx.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr float TS = 16.f;
	constexpr float PlayerR = 4.f;   // P.r du proto
	const FColor ColRed(255, 61, 85), ColAm(255, 194, 61), ColMg(255, 61, 139);

	FORCEINLINE float FRand01() { return FMath::FRand(); }
	FORCEINLINE float RandSign() { return FMath::FRand() < 0.5f ? -1.f : 1.f; }

	/** NOM du proto sans la parenthèse (« Meduse (hors doc) » -> « Meduse »). */
	FString ShortName(const FString& N)
	{
		int32 I;
		return N.FindChar(TEXT('('), I) ? N.Left(I).TrimEnd() : N;
	}

	/** Modèle 3D de chaque mob. Modèles du studio (Drive BRISCARD_PROD/3D, 23/11 : Saved/Scripts/blender_ennemis_export.py
	 *  puis eon_ennemis_import.py) : Nuke = Ghoul, Onibi = Kamikaze, Kama = Slasher, Ubume = Invocateur, Ikiryo = Shooter
	 *  (rig sans anim). Brainless et Méduse restent des meshes Unity approchants (MANQUANTS.md §2). */
	struct FLook
	{
		const TCHAR* Type;
		TArray<const TCHAR*> Meshes;   // le premier donne la taille ; les suivants se posent au même endroit
		const TCHAR* Anim;             // boucle de marche, ou nullptr
		float YawOff;                  // lacet du modèle pour qu'il regarde vers +X
		float HeightK;                 // hauteur du modèle = HH du proto × HeightK
		float HoverPx;                 // flotte au-dessus du sol (Méduse)
		const TCHAR* Idle = nullptr;   // à l'arrêt (sinon la boucle de marche au ralenti)
		const TCHAR* Attack = nullptr; // joué une fois au début du coup (UpdateAnim)
		float AttackRate = 1.f;
		const TCHAR* Explode = nullptr;   // Onibi : s'arme puis explose, l'anim continue sur le cadavre
		TArray<const TCHAR*> Deaths;   // une au hasard sur le cadavre (BeginCorpse)
		float ModelHCm = 0.f;          // > 0 : hauteur du modèle imposée en cm ; hitbox HH/HR mise à la même échelle (Vincent, 23/11 :
		                               // Nuke 40, Onibi 150, Kama 180, Ubume 200, Ikiryo 180 ; Jira 181)
	};
	// Anim du studio : /Game/EON/Ennemis/<Nom>/Anims/A_<Nom>_<Anim>
#define EON_ANIM(N, A) TEXT("/Game/EON/Ennemis/" N "/Anims/A_" N "_" A ".A_" N "_" A)
	const TArray<FLook>& Looks()
	{
		static const TArray<FLook> L = {
			{ TEXT("goule"), { TEXT("/Game/EON/Ennemis/Nuke/SK_Nuke.SK_Nuke") }, EON_ANIM("Nuke", "Walk"), -90.f, 1.f, 0.f,
				nullptr, EON_ANIM("Nuke", "Attack"), 1.5f, nullptr, { EON_ANIM("Nuke", "Death_01"), EON_ANIM("Nuke", "Death_02"), EON_ANIM("Nuke", "Death_03") }, 40.f },
			{ TEXT("gouleP"), { TEXT("/Game/EON/Ennemis/Nuke/SK_Nuke.SK_Nuke") }, EON_ANIM("Nuke", "Walk"), -90.f, 1.f, 0.f,
				nullptr, EON_ANIM("Nuke", "Attack_V0"), 1.5f, nullptr, { EON_ANIM("Nuke", "Death_V0"), EON_ANIM("Nuke", "Death_01") }, 25.f },   // petite goule : 10/16 de la goule (HH du proto)
			{ TEXT("sniper"), { TEXT("/Game/EON/Ennemis/Ikiryo/SK_Ikiryo.SK_Ikiryo") }, nullptr, -90.f, 1.f, 0.f, nullptr, nullptr, 1.f, nullptr, {}, 180.f },
			{ TEXT("brainless"), { TEXT("/Game/EON/Unity/import/fbx/chara/Brain_Crawler.Brain_Crawler") }, TEXT("/Game/EON/Unity/import/fbx/chara/Brain_CrawlerArmature_Armature_Armature_Armature_Unreal_Take_baselayer.Brain_CrawlerArmature_Armature_Armature_Armature_Unreal_Take_baselayer"), -90.f, 1.f, 0.f },
			{ TEXT("slasher"), { TEXT("/Game/EON/Ennemis/Kama/SK_Kama.SK_Kama") }, EON_ANIM("Kama", "Run"), -90.f, 1.f, 0.f,
				nullptr, EON_ANIM("Kama", "Attack"), 1.f, nullptr, { EON_ANIM("Kama", "Death") }, 180.f },
			{ TEXT("kamikaze"), { TEXT("/Game/EON/Ennemis/Onibi/SK_Onibi.SK_Onibi") }, EON_ANIM("Onibi", "Run"), -90.f, 1.f, 0.f,
				EON_ANIM("Onibi", "Walk"), nullptr, 1.f, EON_ANIM("Onibi", "Explosion"), { EON_ANIM("Onibi", "Death_01"), EON_ANIM("Onibi", "Death_02") }, 150.f },
			{ TEXT("invocateur"), { TEXT("/Game/EON/Ennemis/Ubume/SK_Ubume.SK_Ubume") }, EON_ANIM("Ubume", "Walk"), -90.f, 1.f, 0.f,
				EON_ANIM("Ubume", "Idle"), EON_ANIM("Ubume", "Attack"), 2.f, nullptr, {}, 200.f },
			{ TEXT("meduse"), { TEXT("/Game/EON/Unity/import/fbx/xeno/Meshy_AI_Unyielding_Chimera_0111100029_generate.Meshy_AI_Unyielding_Chimera_0111100029_generate") }, nullptr, -90.f, 1.f, 6.f },
			{ TEXT("dummy"), { TEXT("/Engine/BasicShapes/Cylinder.Cylinder") }, nullptr, 0.f, 1.f, 0.f },
			{ TEXT("gen"), { TEXT("/Engine/BasicShapes/Cube.Cube") }, nullptr, 0.f, 1.f, 0.f },
		};
		return L;
	}
#undef EON_ANIM
}

AEONEnemy::AEONEnemy()
{
	// Piloté par AEONCombatDirector ; le tick ne sert qu'au cadavre (BeginCorpse), une fois sorti de la liste.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Hitbox : ne touche que les projectiles de Jira (WorldDynamic). Jira (Pawn) passe à travers :
	// le contact se gère par distance, comme dans le proto.
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(Root);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionObjectType(ECC_Pawn);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCanEverAffectNavigation(false);

	ModelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Modele"));
	ModelRoot->SetupAttachment(Root);
}

bool AEONEnemy::Init(AEONCombatDirector* InDirector, AEONTileRoom* InRoom, FName InType, FVector2D InPosPx, int32 CombatN)
{
	Director = InDirector;
	Room = InRoom;
	Type = InType;
	Pos = InPosPx;

	const UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this);
	const UDataTable* DT = Tun ? Tun->GetEnemies() : nullptr;
	const FEONEnemyRow* Row = DT ? DT->FindRow<FEONEnemyRow>(InType, TEXT("EONEnemy"), false) : nullptr;
	if (!Row)
	{
		UE_LOG(LogEON, Warning, TEXT("EON_ENEMY type absent de DT_Ennemis : %s"), *InType.ToString());
		return false;
	}
	Spd = Row->Spd; Mass = Row->Mass; HH = Row->HH; HR = Row->HR; bContact = Row->Contact; bFly = Row->Fly; Adn = Row->Adn;
	// Hitbox alignée sur la taille imposée du modèle (Vincent, 23/11) : HH = hauteur du modèle en px, HR à la même échelle.
	// R (rayon de déplacement contre murs et ennemis) reste celui du proto.
	if (const FLook* Lk = Looks().FindByPredicate([&InType](const FLook& L) { return InType == FName(L.Type); }); Lk && Lk->ModelHCm > 0.f && HH > 0.f)
	{
		const float K = (Lk->ModelHCm / EON::CmPerPx) / HH;
		HH *= K; HR *= K;
	}
	DisplayName = ShortName(Row->OfficialName.IsEmpty() ? InType.ToString() : Row->OfficialName);

	// mkEnemy : sc = 1 + combatN × 0,07 ; hp = ceil(d.hp × sc)
	const float Sc = 1.f + CombatN * 0.07f;
	Hp = MaxHp = FMath::CeilToFloat(Row->Hp * Sc * UEONStatics::GetGameFeel(this).EnemyHp);
	R = Row->R;
	Flash = 0.f; St = EEONEnemySt::Move; StT = 0.f;
	Ft = 1.f + FRand01() * 1.2f;
	Cd = 0.6f;
	Side = RandSign();
	T = FRand01() * 5.f;
	Falling = 0.f;
	Sd = RandSign();

	BuildModel();
	ApplyCapsuleSize();
	UpdateVisual(0.f, Pos);
	return true;
}

void AEONEnemy::MakeElite()
{
	bElite = true;
	DisplayName += TEXT(" élite");
	Hp = MaxHp = FMath::RoundToFloat(MaxHp * 2.2f);
	R = FMath::RoundToFloat(R * 1.3f);
	MassM = 2.f;
	ModelRoot->SetRelativeScale3D(FVector(1.45f));
	ApplyCapsuleSize();
}

void AEONEnemy::BuildModel()
{
	const FLook* Lk = Looks().FindByPredicate([this](const FLook& L) { return Type == FName(L.Type); });
	if (!Lk) { UE_LOG(LogEON, Warning, TEXT("EON_ENEMY pas de modèle 3D pour %s"), *Type.ToString()); return; }
	YawOff = Lk->YawOff;
	const float TargetH = Lk->ModelHCm > 0.f ? Lk->ModelHCm : EON::PxToCm(HH > 0.f ? HH : 16.f) * Lk->HeightK;
	FBoxSphereBounds B0(ForceInit);
	bool bFirst = true;
	for (const TCHAR* Path : Lk->Meshes)
	{
		UObject* O = LoadObject<UObject>(nullptr, Path);
		UMeshComponent* C = nullptr;
		FBoxSphereBounds Bd(ForceInit);
		if (USkeletalMesh* SK = Cast<USkeletalMesh>(O))
		{
			USkeletalMeshComponent* K = NewObject<USkeletalMeshComponent>(this);
			K->SetSkeletalMesh(SK);
			if (UAnimSequence* A = Lk->Anim ? LoadObject<UAnimSequence>(nullptr, Lk->Anim) : nullptr)
			{
				K->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				K->PlayAnimation(A, true);
				K->SetPosition(FMath::FRand() * A->GetPlayLength());
				AnimMove = CurAnim = A;
			}
			Bd = SK->GetBounds();
			C = K;
		}
		else if (UStaticMesh* SM = Cast<UStaticMesh>(O))
		{
			UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
			M->SetStaticMesh(SM);
			Bd = SM->GetBounds();
			C = M;
		}
		if (!C) { UE_LOG(LogEON, Warning, TEXT("EON_ENEMY modèle introuvable : %s"), Path); continue; }
		if (bFirst) { B0 = Bd; bFirst = false; }
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(ModelRoot);
		C->RegisterComponent();
		Meshes.Add(C);
		for (int32 I = 0; I < C->GetNumMaterials(); ++I)
		{
			// COM-V9 : chaque matériau devient une instance dynamique (trous) ; c'est elle que le flash remet ensuite.
			UMaterialInstanceDynamic* Hm = C->CreateDynamicMaterialInstance(I);
			if (Hm) { HoleMids.Add(Hm); }
			OrigMats.Add(Hm ? Hm : C->GetMaterial(I));
		}
	}
	if (bFirst) { return; }
	for (UMeshComponent* M : Meshes) { UEONStatics::MarkCelComponent(M); }   // celluloïd
	auto LoadAnim = [](const TCHAR* P) -> UAnimSequence*
	{
		UAnimSequence* A = P ? LoadObject<UAnimSequence>(nullptr, P) : nullptr;
		if (P && !A) { UE_LOG(LogEON, Warning, TEXT("EON_ENEMY anim introuvable : %s"), P); }
		return A;
	};
	if (AnimMove)
	{
		AnimIdle = LoadAnim(Lk->Idle);
		AnimAttack = LoadAnim(Lk->Attack);
		AnimExplode = LoadAnim(Lk->Explode);
		AttackRate = Lk->AttackRate;
		for (const TCHAR* P : Lk->Deaths) { if (UAnimSequence* A = LoadAnim(P)) { AnimDeaths.Add(A); } }
	}
	// Taille : la hauteur du modèle vaut la hauteur de hitbox du proto ; pieds au sol, centré sur la position.
	ModelScale = TargetH / FMath::Max(1.f, B0.BoxExtent.Z * 2.f);
	const FVector Off(-B0.Origin.X * ModelScale, -B0.Origin.Y * ModelScale, -(B0.Origin.Z - B0.BoxExtent.Z) * ModelScale + EON::PxToCm(Lk->HoverPx));
	for (UMeshComponent* C : Meshes) { C->SetRelativeScale3D(FVector(ModelScale)); C->SetRelativeLocation(Off); }
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe")))
	{
		FlashMat = UMaterialInstanceDynamic::Create(Base, this);
		FlashMat->SetVectorParameterValue(TEXT("BC1"), FLinearColor::White);
		FlashMat->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(6.f, 6.f, 6.f));
	}
}

void AEONEnemy::SetFlash(bool bOn)
{
	if (bOn == bFlashOn || !FlashMat) { return; }
	bFlashOn = bOn;
	int32 K = 0;
	for (UMeshComponent* C : Meshes)
	{
		for (int32 I = 0; I < C->GetNumMaterials(); ++I, ++K) { C->SetMaterial(I, bOn ? FlashMat.Get() : (OrigMats.IsValidIndex(K) ? OrigMats[K].Get() : nullptr)); }
	}
}

void AEONEnemy::ApplyCapsuleSize()
{
	// Capsule du proto : des pieds (e.y) à la tête (e.y − ehh), rayon ehr. Elle monte au moins au-dessus
	// du bras de Jira (17 px) : en 3D les tirs volent à cette hauteur, alors que le proto les teste en projection.
	// hitTest(e, x, y, rad) du proto : la balle touche si elle passe à moins de ehr d'un segment vertical d'écran de hauteur ehh.
	// En vue de dessus, la hauteur d'écran est l'axe nord-sud (Y monde) : capsule couchée le long de Y, à hauteur du bras,
	// longue de ehh + 2·ehr. (Une capsule debout ne laissait qu'un disque de rayon ehr : 8 % de balles au but.)
	const float Rad = EON::PxToCm(Ehr());
	const float Half = Rad + EON::PxToCm(Ehh()) * 0.5f;
	Capsule->SetCapsuleSize(Rad, Half);
	Capsule->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	Capsule->SetRelativeLocation(FVector(0.f, 0.f, EON::PxToCm(EON::ArmHeightPx)));
}

FVector AEONEnemy::GetAimPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, EON::PxToCm(EON::ArmHeightPx));
}

void AEONEnemy::MarkDead()
{
	bDead = true;
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEONEnemy::PlayAnim(UAnimSequence* A, bool bLoop, float Rate)
{
	if (!A) { return; }
	for (UMeshComponent* C : Meshes)
	{
		if (USkeletalMeshComponent* K = Cast<USkeletalMeshComponent>(C))
		{
			if (A != CurAnim || !bLoop) { K->PlayAnimation(A, bLoop); }
			K->SetPlayRate(Rate);
		}
	}
	CurAnim = A;
}

void AEONEnemy::UpdateAnim(float Dt, float DistToTarget)
{
	if (!AnimMove) { return; }
	// Début d'un coup : l'anim d'attaque (ou l'explosion de l'Onibi qui s'arme) part une fois, la boucle reprend ensuite.
	const bool bEnter = St != PrevSt;
	PrevSt = St;
	UAnimSequence* Shot = nullptr;
	float Rate = AttackRate;
	if (Type == TEXT("kamikaze")) { if (bEnter && St == EEONEnemySt::Arm) { Shot = AnimExplode; Rate = 1.f; } }
	else if (Type == TEXT("slasher")) { if (bEnter && St == EEONEnemySt::Wind) { Shot = AnimAttack; } }
	else if (Type == TEXT("invocateur")) { if (bEnter && St == EEONEnemySt::Lay) { Shot = AnimAttack; } }
	else if (bContact && ActionT <= 0.f && DistToTarget < R + PlayerR + 4.f) { Shot = AnimAttack; }   // Nuke au contact de Jira
	if (Shot)
	{
		PlayAnim(Shot, false, Rate);
		ActionT = Shot->GetPlayLength() / FMath::Max(0.1f, Rate);
		UE_LOG(LogEON, Verbose, TEXT("EON_ENEMY coup %s : %s"), *Type.ToString(), *Shot->GetName());
		return;
	}
	if (ActionT > 0.f) { ActionT -= Dt; return; }
	const float V = Vel.Size();
	if (V > 8.f) { PlayAnim(AnimMove, true, FMath::Clamp(V / FMath::Max(10.f, Spd), 0.6f, 2.f)); }
	else if (AnimIdle) { PlayAnim(AnimIdle, true, 1.f); }
	else { PlayAnim(AnimMove, true, 0.25f); }
}

bool AEONEnemy::BeginCorpse()
{
	if (bFell || !AnimMove) { return false; }
	UAnimSequence* A = nullptr;
	float Left = 0.f;
	if (bExploded && AnimExplode)
	{
		// L'Onibi a commencé son explosion en s'armant : elle continue là où elle en est.
		A = AnimExplode;
		Left = FMath::Max(0.f, ActionT);
		if (CurAnim != A) { PlayAnim(A, false, 1.f); Left = A->GetPlayLength(); }
	}
	else if (AnimDeaths.Num())
	{
		A = AnimDeaths[FMath::RandRange(0, AnimDeaths.Num() - 1)];
		PlayAnim(A, false, 1.f);
		Left = A->GetPlayLength();
	}
	if (!A) { return false; }
	UE_LOG(LogEON, Verbose, TEXT("EON_ENEMY cadavre %s : %s, %.2f s"), *Type.ToString(), *A->GetName(), Left);
	SetFlash(false);
	// Reste à terre 0,6 s après l'anim, puis s'enfonce en 0,5 s (le proto : fondu immédiat).
	CorpseT = Left + 0.6f;
	SinkT = 0.f;
	SetActorTickEnabled(true);
	SetLifeSpan(CorpseT + 2.f);   // filet si le tick s'arrête (changement de salle)
	return true;
}

void AEONEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bDead) { return; }
	if (CorpseT > 0.f) { CorpseT -= DeltaSeconds; return; }
	SinkT += DeltaSeconds;
	ModelRoot->SetRelativeLocation(FVector(0.f, 0.f, -SinkT / 0.5f * EON::PxToCm(Ehh())));
	if (SinkT >= 0.5f) { Destroy(); }
}

void AEONEnemy::ApplyHit(float Damage, FVector Dir, float KnockMul)
{
	const FVector2D D = FVector2D(Dir.X, Dir.Y).GetSafeNormal();
	HitPx(Damage, D.X, D.Y, KnockMul);
}

void AEONEnemy::PunchHole(float Dmg, float Dx, float Dy)
{
	// punchHole du proto (l. 3290) : impact dispersé sur le corps, biaisé du côté d'où vient le tir ; surface retirée
	// = max(0,02, dégâts / PV max) × chair × 1,3 (chair = moitié du rectangle du corps), 62 % dans le trou principal.
	const AEONTileRoom* Rf = Room.Get();
	if (!Rf || HoleMids.Num() == 0 || Meshes.Num() == 0 || !Meshes[0] || UEONStatics::GetGameFeel(this).Holes <= 0.f) { return; }
	const float Hcm = EON::PxToCm(Ehh());
	const FBoxSphereBounds Wb = Meshes[0]->Bounds;   // bornes monde (échelle et élite comprises)
	const float Rcm = FMath::Max(FMath::Min(Wb.BoxExtent.X, Wb.BoxExtent.Y), EON::PxToCm(Ehr()));
	const FVector Base = Rf->RoomPxToWorld(Pos, 0.f);
	const FVector Dw = (Rf->RoomPxToWorld(Pos + FVector2D(Dx, Dy)) - Base).GetSafeNormal2D();   // sens du tir, monde
	const FVector SideV(-Dw.Y, Dw.X, 0.f);
	// 24/11 : le trou est un tube dans l'axe de la caméra (TrouAxe), qui traverse le corps et se voit toujours ;
	// impact autour du centre du corps (± 35 % du rayon), un peu du côté du tireur.
	const FVector P = Base - Dw * Rcm * 0.15f + SideV * (FMath::FRand() - 0.5f) * Rcm * 0.7f
		+ FVector(0.f, 0.f, Hcm * (0.5f + (FMath::FRand() - 0.5f) * 0.7f));
	const float Chair = (2.f * Rcm) * Hcm * 0.5f;
	const float Hk = UEONStatics::GetGameFeel(this).Holes;   // réglage « Trous » (proto : × holes²)
	const float A = FMath::Max(0.02f, Dmg / FMath::Max(1.f, MaxHp)) * Chair * 1.3f * Hk * Hk;
	// Vincent, 24/11 : trous trop gros sur le modèle 3D ; rayon × 0,45 (le corps 3D est plus large que le sprite).
	const float RHole = FMath::Clamp(FMath::Sqrt(A * 0.62f / PI) * 0.45f, EON::PxToCm(0.8f), Rcm * 0.45f);
	const FTransform& Xf = Meshes[0]->GetComponentTransform();
	const FVector Lp = Xf.InverseTransformPosition(P);
	const float Lr = RHole / FMath::Max(0.001f, Xf.GetScale3D().X);
	const FName Slot(*FString::Printf(TEXT("Trou%d"), HoleNext));
	HoleNext = (HoleNext + 1) % 8;
	FVector Cam = FVector(0.f, 0.f, -1.f);
	if (const APlayerCameraManager* Pcm = UGameplayStatics::GetPlayerCameraManager(this, 0)) { Cam = Pcm->GetCameraRotation().Vector(); }
	const FVector La = Xf.InverseTransformVectorNoScale(Cam).GetSafeNormal();
	for (UMaterialInstanceDynamic* M : HoleMids)
	{
		if (!M) { continue; }
		M->SetVectorParameterValue(Slot, FLinearColor(Lp.X, Lp.Y, Lp.Z, Lr));
		M->SetVectorParameterValue(TEXT("TrouAxe"), FLinearColor(La.X, La.Y, La.Z, 0.f));
	}
	// Sang qui gicle à la formation du trou (Vincent, 24/11) : sort par le trou dans le sens du tir (cône ±63°),
	// rouge vif et rouge sombre (1 sur 3), 40 à 170 px/s, 0,5 s ; un peu plus pour un gros trou.
	const int32 Nb = FMath::Clamp(FMath::RoundToInt(6.f + RHole / EON::PxToCm(1.f) * 2.f), 6, 16);
	EONFx::GerbeDir2(this, P, Dw, Nb, 3, EONFx::Hex(TEXT("c0142e")), EONFx::Hex(TEXT("5e0816")), 170.f, 0.5f, 1.3f, 3.f);
}

void AEONEnemy::HitPx(float Dmg, float Dx, float Dy, float KMul)
{
	// hitEnemy(e, dmg, dx, dy, kmul), branche des mobs ordinaires
	if (bDead || Falling > 0.f) { return; }
	const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
	Hp -= Dmg;
	Flash = F.HitFlash ? 0.07f : 0.f;
	const float M = Mass * MassM;
	Knock.X += Dx * 95.f * F.Knock * KMul / M;
	Knock.Y += Dy * 95.f * F.Knock * KMul / M;
	// punchHole (COM-V9) : un trou dans le corps, du côté du tireur. Puis l'éclaboussure de chair du proto :
	// 4 éclats vers le tireur (-d × 70 px/s ± 35), 0,3 s, couleur du mob (le premier blanc), frein 5.
	PunchHole(Dmg, Dx, Dy);
	AEONCombatDirector* Dir = Director.Get();
	if (const AEONTileRoom* Rf = Room.Get())
	{
		const TCHAR* Hx = Type == TEXT("goule") ? TEXT("e8a24a") : Type == TEXT("gouleP") ? TEXT("6fbf4a") : Type == TEXT("kamikaze") ? TEXT("ff7a1a")
			: Type == TEXT("meduse") ? TEXT("8f7be0") : Type == TEXT("sniper") ? TEXT("e8923a") : Type == TEXT("slasher") ? TEXT("e0405a")
			: Type == TEXT("brainless") ? TEXT("c86a6a") : Type == TEXT("invocateur") ? TEXT("8a6aaa") : TEXT("c84a7a");
		const FVector Body = Rf->RoomPxToWorld(Pos, EON::PxToCm(Ehh() * 0.5f));
		EONFx::GerbeDir2(this, Body, FVector(-Dx, -Dy, 0.f), 4, 4, EONFx::Hex(Hx), FLinearColor::White, 105.f, 0.3f, 1.f, 5.f);
		// hitEnemy : 4 éclats blancs dans le sens du coup (d × 60 px/s ± 40), 0,25 s.
		EONFx::GerbeDir(this, Rf->RoomPxToWorld(Pos, EON::PxToCm(4.f)), FVector(Dx, Dy, 0.f), 4, FLinearColor::White, 100.f, 0.25f);
	}
	if (AEONCharacter* J = Dir ? Dir->GetJira() : nullptr) { J->AddUlt(Dmg); }   // P.ult += dmg × 0,8
	AEONTileRoom* Rm = Room.Get();
	if (Dir && Rm && F.DmgNums)
	{
		const FVector2D NumPx(Pos.X + (FRand01() - 0.5f) * 6.f, Pos.Y);
		const FVector At = Rm->RoomPxToWorld(NumPx, EON::PxToCm(Ehh() + 8.f));
		// round(dmg × 10) / 10, affiché comme en JS (« 1 », « 1.5 »)
		const int32 D10 = FMath::RoundToInt(Dmg * 10.f);
		const FString Txt = D10 % 10 == 0 ? FString::FromInt(D10 / 10) : FString::Printf(TEXT("%.1f"), D10 / 10.f);
		Dir->AddNum(At, Txt, Dmg >= 3.f ? ColAm : FColor::White);
	}
	if (Dir && Dmg >= 3.f) { Dir->Hitstop(F.Hitstop * 0.3f); }
	if (Dir && Hp <= 0.f) { Dir->KillEnemy(this, Dx, Dy); }
}

FVector2D AEONEnemy::ChaseDir(const FVector2D& Tg) const
{
	// chaseDir(e, P) : descend le champ de distances (computeFlow) vers la case voisine la plus proche de Jira,
	// diagonales seulement si les deux cases d'angle sont libres ; tout droit à moins d'une case.
	const FVector2D D = Tg - Pos;
	const float Dl = FMath::Max(D.Size(), KINDA_SMALL_NUMBER);
	const FVector2D Straight = D / (D.Size() > 0.f ? Dl : 1.f);
	const AEONTileRoom* Rm = Room.Get();
	if (!Rm) { return Straight; }
	const int32 Tx = FMath::FloorToInt(Pos.X / TS), Ty = FMath::FloorToInt(Pos.Y / TS);
	const int32 Fi = Rm->FlowAt(Tx, Ty);
	if (Fi < 0 || Fi <= 1) { return Straight; }
	int32 Best = Fi, Bx = 0, By = 0;
	for (int32 Oy = -1; Oy <= 1; ++Oy)
	{
		for (int32 Ox = -1; Ox <= 1; ++Ox)
		{
			if (!Ox && !Oy) { continue; }
			const int32 Fj = Rm->FlowAt(Tx + Ox, Ty + Oy);
			if (Fj < 0 || Fj >= Best) { continue; }
			if (Ox && Oy && (Rm->FlowAt(Tx + Ox, Ty) < 0 || Rm->FlowAt(Tx, Ty + Oy) < 0)) { continue; }
			Best = Fj; Bx = Ox; By = Oy;
		}
	}
	if (!Bx && !By) { return Straight; }
	const FVector2D G((Tx + Bx + 0.5f) * TS - Pos.X, (Ty + By + 0.5f) * TS - Pos.Y);
	const float Gd = G.Size() > 0.f ? G.Size() : 1.f;
	return G / Gd;
}

void AEONEnemy::UpdateEnemy(float Dt, const FVector2D& Tg)
{
	if (bDead) { return; }
	AEONCombatDirector* Dir = Director.Get();
	AEONTileRoom* Rm = Room.Get();
	if (!Dir || !Rm) { return; }
	T += Dt;
	Flash -= Dt;

	if (Falling > 0.f)
	{
		Falling -= Dt;
		Pos += Knock * Dt * 0.3f;
		if (Falling <= 0.f) { bFell = true; MarkDead(); Dir->OnEnemyFell(this); }
		return;
	}
	if (bStalk) { StalkUpdate(Dt, Dir->GetJiraPx()); return; }

	const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
	// Écart 3D (MANQUANTS §9) : les vitesses de croisière suivent SpeedMul3D comme Jira ;
	// les élans (dash, charge 235 px/s) et le recul restent ceux du proto, liés aux distances en tuiles.
	const float Mul = F.SpeedMul3D * F.EnemySpeed;
	const float Dx = Tg.X - Pos.X, Dy = Tg.Y - Pos.Y;
	const float D = FMath::Max(FMath::Sqrt(Dx * Dx + Dy * Dy), 1e-4f);
	const float Nx = Dx / D, Ny = Dy / D, Sp = Spd * Mul;
	float Tvx = 0.f, Tvy = 0.f;
	const TArray<TObjectPtr<AEONEnemy>>& All = Dir->GetEnemies();

	auto Separate = [&](bool bSameTypeOnly)
	{
		for (const AEONEnemy* O : All)
		{
			if (!O || O == this || O->bDead) { continue; }
			if (bSameTypeOnly && O->Type != Type) { continue; }
			const FVector2D Od = Pos - O->Pos;
			const float L2 = Od.Size();
			if (L2 < 10.f && L2 > 0.f) { Tvx += Od.X / L2 * 30.f * Mul; Tvy += Od.Y / L2 * 30.f * Mul; }
		}
	};

	const FName N = Type;
	if (N == TEXT("goule"))
	{
		// Nuke - comportement NUÉE : forme des meutes de 3 et déborde, fuit si seule ;
		// seule depuis 3,5 s elle s'enrage et charge, pour ne jamais bloquer la salle.
		int32 Total = 0;
		const AEONEnemy* Near = nullptr;
		float Nd = 1e9f;
		int32 Grp = 1;
		for (const AEONEnemy* O : All)
		{
			if (!O || O->Type != TEXT("goule") || O->bDead) { continue; }
			++Total;
			if (O == this) { continue; }
			const float Od = (O->Pos - Pos).Size();
			if (Od < 70.f) { ++Grp; }
			if (Od < Nd) { Nd = Od; Near = O; }
		}
		const int32 Need = FMath::Min(3, Total);
		if (Total <= 1) { Al += Dt; Rg = 0.f; } else { Al = 0.f; }
		bool bGroup = false;
		if (!(Total <= 1 && Al < 3.5f) && Grp < Need && Near)
		{
			Rg += Dt;
			bGroup = Rg < 3.f;
		}
		if (Total <= 1 && Al < 3.5f)
		{
			const float Want = D < 120.f ? -1.f : 0.f;
			Tvx = Nx * Sp * Want * 1.1f; Tvy = Ny * Sp * Want * 1.1f;
		}
		else if (bGroup)
		{
			const FVector2D G = Near->Pos - Pos;
			const float Gd = G.Size() > 0.f ? G.Size() : 1.f;
			Tvx = G.X / Gd * Sp * 1.1f; Tvy = G.Y / Gd * Sp * 1.1f;
		}
		else
		{
			if (Grp >= Need) { Rg = 0.f; }
			const FVector2D C = ChaseDir(Tg);
			const float K = Total <= 1 ? 1.3f : 1.f;
			Tvx = (C.X * Sp + (-Ny) * Sp * 0.35f * Sd) * K;
			Tvy = (C.Y * Sp + Nx * Sp * 0.35f * Sd) * K;
		}
		Separate(true);
	}
	else if (N == TEXT("gouleP"))
	{
		const FVector2D C = ChaseDir(Tg);
		Tvx = C.X * Sp; Tvy = C.Y * Sp;
		Separate(false);
	}
	else if (N == TEXT("kamikaze"))
	{
		if (St == EEONEnemySt::Move)
		{
			const float W = 1.f + 0.3f * FMath::Sin(T * 8.f);
			const FVector2D C = ChaseDir(Tg);
			Tvx = C.X * Sp * W; Tvy = C.Y * Sp * W;
			if (D < 18.f) { St = EEONEnemySt::Arm; StT = 0.45f; }
		}
		else
		{
			StT -= Dt;
			if (StT <= 0.f)
			{
				// Explose : mort sans crédit, AOE 28 px, 3 dégâts, blesse Jira
				bExploded = true;
				MarkDead();
				Dir->Aoe(Pos, 28.f, 3.f, true, FLinearColor(FColor::FromHex(TEXT("ff7a1a"))));
				return;
			}
		}
	}
	else if (N == TEXT("meduse"))
	{
		const float Want = D > 115.f ? 1.f : D < 70.f ? -1.f : 0.f;
		Tvx = Nx * Sp * Want + (-Ny) * Sp * 0.6f * Side;
		Tvy = Ny * Sp * Want + Nx * Sp * 0.6f * Side;
		if (FRand01() < Dt * 0.4f) { Side *= -1.f; }
		Ft -= Dt;
		if (Ft <= 0.f)
		{
			const float Base = FMath::Atan2(Dy, Dx);
			for (int32 K = -1; K <= 1; ++K)
			{
				const float Aa = Base + K * 0.22f;
				Dir->FireEnemyBullet(FVector2D(Pos.X, Pos.Y - 9.f), FVector2D(FMath::Cos(Aa), FMath::Sin(Aa)) * 82.f, 3.f, false, 9.f);
			}
			Ft = 1.7f + FRand01() * 0.7f;
		}
	}
	else if (N == TEXT("slasher"))
	{
		Cd -= Dt;
		if (St == EEONEnemySt::Move)
		{
			const FVector2D C = ChaseDir(Tg);
			Tvx = C.X * Sp; Tvy = C.Y * Sp;
			if (D < 85.f && Cd <= 0.f) { St = EEONEnemySt::Wind; StT = 0.55f; L = FVector2D(Nx, Ny); }
		}
		else if (St == EEONEnemySt::Wind)
		{
			StT -= Dt;
			if (StT <= 0.f) { St = EEONEnemySt::Dash; StT = 0.32f; }
		}
		else if (St == EEONEnemySt::Dash)
		{
			StT -= Dt;
			Tvx = L.X * 235.f; Tvy = L.Y * 235.f;
			// Traînée du dash : points rouges immobiles de 2 px (0,2 s), 8 images sur 10 du proto (48 par seconde).
			if (FRand01() < 48.f * Dt) { EONFx::Gerbe(this, Rm->RoomPxToWorld(Pos, EON::PxToCm(2.f)), 1, EONFx::Hex(TEXT("e0405a")), 0.f, 0.2f, 2.f, 0.f); }
			if (StT <= 0.f) { St = EEONEnemySt::Rec; StT = 0.6f; }
		}
		else
		{
			StT -= Dt;
			if (StT <= 0.f) { St = EEONEnemySt::Move; Cd = 1.2f; }
		}
	}
	else if (N == TEXT("sniper"))
	{
		// Ikiryo : garde ses distances, vise longuement puis tire un coup unique et rapide
		Cd -= Dt;
		if (St == EEONEnemySt::Move)
		{
			const float Flee = D < 170.f ? -1.f : D > 280.f ? 1.f : 0.f;
			Tvx = Nx * Sp * Flee + (-Ny) * Sp * 0.45f * Side;
			Tvy = Ny * Sp * Flee + Nx * Sp * 0.45f * Side;
			if (FRand01() < Dt * 0.5f) { Side *= -1.f; }
			if (Cd <= 0.f && D > 110.f) { St = EEONEnemySt::Aim; StT = 1.1f; }
		}
		else if (St == EEONEnemySt::Aim)
		{
			StT -= Dt;
			L = FVector2D(Nx, Ny);   // suit la cible pendant la visée
			if (StT <= 0.f) { St = EEONEnemySt::Fire; StT = 0.12f; }
		}
		else if (St == EEONEnemySt::Fire)
		{
			StT -= Dt;
			if (StT <= 0.f)
			{
				const float V = 170.f;
				Dir->FireEnemyBullet(FVector2D(Pos.X + L.X * 12.f, Pos.Y - 13.f + L.Y * 12.f), L * V, 3.f, true, 13.f);
				Dir->Shake(0.14f);
				// IKI-V2 : éclair au canon (halo orange, trait et cœur blancs) et lumière de 22 px, 0,08 s.
				if (const AEONTileRoom* Rk = Room.Get())
				{
					const FVector Mz = Rk->RoomPxToWorld(Pos + L * 12.f, EON::PxToCm(13.f));
					const FVector Dk = Rk->RoomPxToWorld(Pos + L) - Rk->RoomPxToWorld(Pos);
					EONFx::Eclair(this, Mz, Dk, EONFx::Hex(TEXT("e8923a")), 0.08f, 1.5f, 22.f);
				}
				St = EEONEnemySt::Rec; StT = 0.5f; Cd = 2.6f + FRand01() * 0.9f;
			}
		}
		else
		{
			StT -= Dt;
			const float F2 = D < 150.f ? -1.f : 0.f;
			Tvx = Nx * Sp * F2; Tvy = Ny * Sp * F2;
			if (StT <= 0.f) { St = EEONEnemySt::Move; }
		}
	}
	else if (N == TEXT("brainless"))
	{
		// Brainless : fonce tête la première, étourdi s'il percute un mur
		Cd -= Dt;
		if (St == EEONEnemySt::Move)
		{
			const FVector2D C = ChaseDir(Tg);
			Tvx = C.X * Sp; Tvy = C.Y * Sp;
			if (D < 150.f && Cd <= 0.f) { St = EEONEnemySt::Lock; StT = 0.65f; }
		}
		else if (St == EEONEnemySt::Lock)
		{
			StT -= Dt;
			L = FVector2D(Nx, Ny);
			Tvx = -Nx * 18.f * Mul; Tvy = -Ny * 18.f * Mul;   // recule pour prendre son élan
			if (StT <= 0.f) { St = EEONEnemySt::Chg; StT = 1.7f; }
		}
		else if (St == EEONEnemySt::Chg)
		{
			StT -= Dt;
			Tvx = L.X * 235.f; Tvy = L.Y * 235.f;
			// Poussière de la charge : grains beiges (±25 px/s, 0,25 s), 6 images sur 10 du proto (36 par seconde).
			if (FRand01() < 36.f * Dt) { EONFx::Gerbe(this, Rm->RoomPxToWorld(Pos, EON::PxToCm(2.f)), 1, EONFx::Hex(TEXT("b0a488")), 35.f, 0.25f); }
			if (D < R + PlayerR + 3.f)
			{
				Dir->HurtJira(1, Pos, L * 170.f);
				St = EEONEnemySt::Stun; StT = 0.9f;
				Dir->Shake(0.45f);
			}
			else if (StT <= 0.f) { St = EEONEnemySt::Move; Cd = 2.f; }
		}
		else
		{
			StT -= Dt;
			if (StT <= 0.f) { St = EEONEnemySt::Move; Cd = 2.f; }
		}
	}
	else if (N == TEXT("invocateur"))
	{
		// Ubume : reste à distance et pond des œufs qui éclosent en petites Nuke
		Cd -= Dt;
		if (St == EEONEnemySt::Move)
		{
			const float Flee = D < 120.f ? -1.f : D > 200.f ? 1.f : 0.f;
			Tvx = Nx * Sp * Flee + (-Ny) * Sp * 0.5f * Side;
			Tvy = Ny * Sp * Flee + Nx * Sp * 0.5f * Side;
			if (FRand01() < Dt * 0.5f) { Side *= -1.f; }
			if (Cd <= 0.f && Dir->NumEggs() < 3) { St = EEONEnemySt::Lay; StT = 0.75f; }
		}
		else
		{
			StT -= Dt;
			if (StT <= 0.f)
			{
				const float A = FMath::Atan2(Dy, Dx) + (FRand01() - 0.5f) * 0.6f, Len = 45.f + FRand01() * 55.f;
				Dir->LayEgg(FVector2D(Pos.X, Pos.Y - 10.f), Pos + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Len);
				St = EEONEnemySt::Move; Cd = 3.4f + FRand01() * 1.2f;
			}
		}
	}

	// Intégration commune
	const float K = FMath::Min(1.f, Dt * 8.f);
	Vel.X += (Tvx - Vel.X) * K; Vel.Y += (Tvy - Vel.Y) * K;
	if (St == EEONEnemySt::Dash || St == EEONEnemySt::Chg) { Vel = FVector2D(Tvx, Tvy); }
	Pos += (Vel + Knock) * Dt;
	const float Kd = FMath::Min(1.f, Dt * 9.f);
	const bool bPushed = Knock.Size() > 45.f || St == EEONEnemySt::Dash || St == EEONEnemySt::Chg;
	Knock -= Knock * Kd;
	// Le vide est un mur pour eux, sauf quand on les pousse (recul, dash, charge) : c'est ainsi qu'ils tombent.
	const bool bBump = Rm->CollideCircle(Pos, R, !bFly && !bPushed);
	if (bBump && St == EEONEnemySt::Dash) { St = EEONEnemySt::Rec; StT = 0.7f; Dir->Shake(0.12f); }
	if (bBump && St == EEONEnemySt::Chg)
	{
		St = EEONEnemySt::Stun; StT = 1.6f; Vel = FVector2D::ZeroVector; Dir->Shake(0.3f);
		// Choc contre le mur : 12 éclats crème et blanc (110 px/s), 0,35 s.
		EONFx::Gerbe2(this, Rm->RoomPxToWorld(Pos, EON::PxToCm(6.f)), 12, 2, EONFx::Hex(TEXT("e8e0c8")), FLinearColor::White, 110.f, 0.35f);
	}
	if (!bFly && Rm->PitAtPx(Pos))
	{
		Falling = 0.45f; St = EEONEnemySt::Move;
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}
	if (bContact && D < R + PlayerR + 1.f) { Dir->HurtJira(1, Pos); }
}

void AEONEnemy::UpdateVisual(float Dt, const FVector2D& Tg)
{
	AEONTileRoom* Rm = Room.Get();
	if (!Rm) { return; }
	float Lift = 0.f, Scale = 1.f;
	if (Type == TEXT("meduse")) { Lift = 5.f - FMath::RoundToFloat(FMath::Sin(T * 4.f)); }   // flotte
	FVector2D P = Pos;
	if ((St == EEONEnemySt::Arm || St == EEONEnemySt::Wind) && Falling <= 0.f) { P.X += FMath::RoundToFloat(FRand01() * 2.f - 1.f); }   // tremble
	float SinkCm = 0.f;
	if (Falling > 0.f)
	{
		// Chute : rétrécit (0,2 + 0,8 k) et s'enfonce dans le vide (le proto : fondu + 5 px vers le bas)
		const float K = Falling / 0.45f;
		Scale = 0.2f + 0.8f * K;
		SinkCm = (1.f - K) * 200.f;
	}
	SetActorLocation(Rm->RoomPxToWorld(P, EON::PxToCm(Lift) - SinkCm));
	SetActorScale3D(FVector(Scale));

	// Modèle : regarde où il marche, ou vers Jira à l'arrêt et pendant ses attaques (visée, charge) ; anim au rythme de la marche.
	const bool bMoving = Vel.Size() > 8.f;
	const bool bLocked = St == EEONEnemySt::Aim || St == EEONEnemySt::Fire || St == EEONEnemySt::Lock || St == EEONEnemySt::Wind || St == EEONEnemySt::Dash || St == EEONEnemySt::Chg;
	const FVector2D Fd = bLocked ? L : bMoving ? Vel : Tg - Pos;
	if (!Fd.IsNearlyZero())
	{
		const float Want = FMath::RadiansToDegrees(FMath::Atan2(Fd.Y, Fd.X));
		FaceYaw = FMath::FixedTurn(FaceYaw, Want, 540.f * Dt);
	}
	ModelRoot->SetRelativeRotation(FRotator(0.f, FaceYaw + YawOff, 0.f));
	if (AnimMove) { UpdateAnim(Dt, FVector2D::Distance(Tg, Pos)); }
	else
	{
		for (UMeshComponent* C : Meshes)
		{
			if (USkeletalMeshComponent* K = Cast<USkeletalMeshComponent>(C)) { K->SetPlayRate(bMoving ? FMath::Clamp(Vel.Size() / FMath::Max(10.f, Spd), 0.6f, 2.f) : 0.25f); }
		}
	}
	// Clignotement blanc : coup reçu, et l'Onibi qui s'arme.
	const bool bWhite = Flash > 0.f || (St == EEONEnemySt::Arm && FMath::FloorToInt(T * 20.f) % 2 == 0);
	SetFlash(bWhite);

#if ENABLE_DRAW_DEBUG
	// Télégraphes du proto (drawEnemy) : provisoires en lignes de debug, absents des builds Shipping.
	UWorld* W = GetWorld();
	if (!W || bDead || Falling > 0.f) { return; }
	auto At = [&](const FVector2D& Px, float Zpx) { return Rm->RoomPxToWorld(Px, EON::PxToCm(Zpx)); };
	if (Type == TEXT("sniper") && (St == EEONEnemySt::Aim || St == EEONEnemySt::Fire))
	{
		const bool bF = St == EEONEnemySt::Fire;
		DrawDebugLine(W, At(Pos + L * 14.f, 13.f), At(Pos + L * 240.f, 13.f), bF ? FColor::White : ColRed, false, -1.f, 0, bF ? 3.f : 1.f);
	}
	if (Type == TEXT("brainless") && St == EEONEnemySt::Lock && FMath::FloorToInt(T * 20.f) % 2 == 0)
	{
		DrawDebugLine(W, At(Pos + L * 10.f, 7.f), At(Pos + L * 90.f, 7.f), ColRed, false, -1.f, 0, 2.f);
	}
	if (Type == TEXT("brainless") && St == EEONEnemySt::Stun)
	{
		for (int32 I = 0; I < 3; ++I)
		{
			const float A = T * 5.f + I * 2.1f;
			DrawDebugPoint(W, At(Pos + FVector2D(FMath::Cos(A) * 7.f, 0.f), Ehh() + 2.f + FMath::Sin(A) * 2.f), 6.f, ColAm);
		}
	}
	if (St == EEONEnemySt::Wind)
	{
		DrawDebugLine(W, At(Pos, 6.f), At(Pos + L * 70.f, 6.f), ColRed, false, -1.f, 0, 1.5f);
	}
	if (Type == TEXT("kamikaze") && St == EEONEnemySt::Arm)
	{
		DrawDebugCircle(W, At(Pos, 1.f), EON::PxToCm(10.f + FMath::Sin(T * 40.f) * 2.f), 24, FColor(255, 154, 61), false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
	if (Type == TEXT("invocateur") && St == EEONEnemySt::Lay)
	{
		DrawDebugCircle(W, At(Pos, 1.f), EON::PxToCm(8.f), 20, FColor(216, 240, 106), false, -1.f, 0, 2.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
	// Barre de vie (élite : nom + barre)
	const float TopPx = Ehh() + 4.f;
	if (bElite || Hp < MaxHp)
	{
		const float Half = bElite ? 10.f : 6.f;
		const FVector A0 = At(FVector2D(Pos.X - Half, Pos.Y), TopPx), A1 = At(FVector2D(Pos.X + Half, Pos.Y), TopPx);
		DrawDebugLine(W, A0, A1, FColor(42, 16, 32), false, -1.f, 0, 3.f);
		DrawDebugLine(W, A0, FMath::Lerp(A0, A1, FMath::Clamp(Hp / MaxHp, 0.05f, 1.f)), ColRed, false, -1.f, 1, 3.f);
	}
	if (bElite)
	{
		DrawDebugString(W, At(Pos, TopPx + 8.f), DisplayName, nullptr, FColor(255, 176, 192), 0.f, false, 1.1f);
	}
#endif
}

// ---------------------------------------------------------------- la chose (Irruption, stalkUpdate)

void AEONEnemy::MakeStalker()
{
	bStalk = true;
	StalkSt = 0;
	StalkT = 2.5f + FMath::FRand() * 1.5f;
	StalkA = FMath::FRand() * 2.f * PI;
	DisplayName = TEXT("La chose");
	Hp = MaxHp = 1.f;
	R = 6.f;
}

void AEONEnemy::StalkUpdate(float Dt, const FVector2D& P)
{
	AEONCombatDirector* Dir = Director.Get();
	AEONTileRoom* Rm = Room.Get();
	if (!Dir || !Rm) { return; }
	StalkT -= Dt;
	if (StalkSt == 0)
	{
		// Rôde : vise un point qui tourne autour de Jira (110 × 80 px), à 70 px/s au plus.
		StalkA += Dt * 0.9f;
		const FVector2D Tg(P.X + FMath::Cos(StalkA) * 110.f, P.Y + FMath::Sin(StalkA) * 80.f);
		const FVector2D D = Tg - Pos;
		const float Ln = FMath::Max(1.f, D.Size());
		Pos += D / Ln * FMath::Min(70.f, Ln * 3.f) * Dt;
		Rm->CollideCircle(Pos, R, true);
		if (StalkT <= 0.f) { StalkSt = 1; StalkT = 0.65f; EONSfx::Play(this, TEXT("SFX_Proto_roar"), 0.9f); }
	}
	else if (StalkSt == 1)
	{
		if (StalkT <= 0.f)
		{
			StalkSt = 2;
			StalkT = 0.75f;
			const float A = FMath::Atan2(P.Y - Pos.Y, P.X - Pos.X);
			StalkV = FVector2D(FMath::Cos(A), FMath::Sin(A)) * 240.f;
		}
	}
	else
	{
		Pos += StalkV * Dt;
		// Traînée de la charge : points lie-de-vin immobiles de 2 px (0,25 s), 6 images sur 10 du proto (36 par seconde).
		if (FRand01() < 36.f * Dt) { EONFx::Gerbe(this, Rm->RoomPxToWorld(Pos, EON::PxToCm(2.f)), 1, EONFx::Hex(TEXT("3a1020")), 0.f, 0.25f, 2.f, 0.f); }
		if (Rm->CollideCircle(Pos, R, true) || StalkT <= 0.f)
		{
			StalkSt = 0;
			StalkT = 2.f + FMath::FRand() * 1.5f;
			StalkA = FMath::Atan2(Pos.Y - P.Y, Pos.X - P.X);
		}
	}
	Vel = FVector2D::ZeroVector;
	// Un contact tue (hurtPlayer(P.hp + 5)) : seuls le bouclier et la Renaissance sauvent.
	if (StalkSt != 0 && FVector2D::Distance(P, Pos) < R + PlayerR + 2.f)
	{
		const AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		Dir->HurtJira((J ? J->GetHP() : 6) + 5, Pos);
	}
}
