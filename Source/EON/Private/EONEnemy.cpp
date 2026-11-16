#include "EONEnemy.h"
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

	/** Modèle 3D de chaque mob. Placeholders (MANQUANTS.md §2) : les modèles du studio ne sont pas encore exportés,
	 *  sauf l'Ikiryo (Shooter_mode.fbx, sans anim) ; Nuke = Ghoul d'Unity (le vrai modèle), le reste = meshes Unity approchants. */
	struct FLook
	{
		const TCHAR* Type;
		TArray<const TCHAR*> Meshes;   // le premier donne la taille ; les suivants se posent au même endroit
		const TCHAR* Anim;             // boucle de marche, ou nullptr
		float YawOff;                  // lacet du modèle pour qu'il regarde vers +X
		float HeightK;                 // hauteur du modèle = HH du proto × HeightK
		float HoverPx;                 // flotte au-dessus du sol (Méduse)
	};
	const TArray<FLook>& Looks()
	{
		static const TArray<FLook> L = {
			{ TEXT("goule"), { TEXT("/Game/EON/Unity/import/fbx/chara/Ghoul_Anim.Ghoul_Anim") }, TEXT("/Game/EON/Unity/import/fbx/chara/Ghoul_Anim_Anim.Ghoul_Anim_Anim"), -90.f, 1.f, 0.f },
			{ TEXT("gouleP"), { TEXT("/Game/EON/Unity/import/fbx/chara/Ghoul_Anim.Ghoul_Anim") }, TEXT("/Game/EON/Unity/import/fbx/chara/Ghoul_Anim_Anim.Ghoul_Anim_Anim"), -90.f, 1.f, 0.f },
			{ TEXT("sniper"), { TEXT("/Game/EON/Ennemis/Ikiryo/body.body"), TEXT("/Game/EON/Ennemis/Ikiryo/shooter_arm.shooter_arm"), TEXT("/Game/EON/Ennemis/Ikiryo/jaws.jaws") }, nullptr, -90.f, 1.f, 0.f },
			{ TEXT("brainless"), { TEXT("/Game/EON/Unity/import/fbx/chara/Brain_Crawler.Brain_Crawler") }, TEXT("/Game/EON/Unity/import/fbx/chara/Brain_CrawlerArmature_Armature_Armature_Armature_Unreal_Take_baselayer.Brain_CrawlerArmature_Armature_Armature_Armature_Unreal_Take_baselayer"), -90.f, 1.f, 0.f },
			{ TEXT("slasher"), { TEXT("/Game/EON/Unity/import/fbx/chara/Mutant.Mutant") }, TEXT("/Game/EON/Unity/import/fbx/chara/Mutant_Anim.Mutant_Anim"), -90.f, 1.f, 0.f },
			{ TEXT("kamikaze"), { TEXT("/Game/EON/Unity/import/fbx/chara/Minion.Minion") }, nullptr, -90.f, 1.f, 0.f },
			{ TEXT("invocateur"), { TEXT("/Game/EON/Unity/import/fbx/xeno/Monster05.Monster05") }, nullptr, -90.f, 1.f, 0.f },
			{ TEXT("meduse"), { TEXT("/Game/EON/Unity/import/fbx/xeno/Meshy_AI_Unyielding_Chimera_0111100029_generate.Meshy_AI_Unyielding_Chimera_0111100029_generate") }, nullptr, -90.f, 1.f, 6.f },
			{ TEXT("dummy"), { TEXT("/Engine/BasicShapes/Cylinder.Cylinder") }, nullptr, 0.f, 1.f, 0.f },
			{ TEXT("gen"), { TEXT("/Engine/BasicShapes/Cube.Cube") }, nullptr, 0.f, 1.f, 0.f },
		};
		return L;
	}
}

AEONEnemy::AEONEnemy()
{
	PrimaryActorTick.bCanEverTick = false;   // piloté par AEONCombatDirector

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
	const float TargetH = EON::PxToCm(HH > 0.f ? HH : 16.f) * Lk->HeightK;
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
		for (int32 I = 0; I < C->GetNumMaterials(); ++I) { OrigMats.Add(C->GetMaterial(I)); }
	}
	if (bFirst) { return; }
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

void AEONEnemy::ApplyHit(float Damage, FVector Dir, float KnockMul)
{
	const FVector2D D = FVector2D(Dir.X, Dir.Y).GetSafeNormal();
	HitPx(Damage, D.X, D.Y, KnockMul);
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
	// punchHole (trous dans le sprite) : pas porté, le placeholder 3D n'a pas de masque.
	AEONCombatDirector* Dir = Director.Get();
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
		if (Falling <= 0.f) { MarkDead(); Dir->OnEnemyFell(this); }
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
				MarkDead();
				Dir->Aoe(Pos, 28.f, 3.f, true);
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
	if (bBump && St == EEONEnemySt::Chg) { St = EEONEnemySt::Stun; StT = 1.6f; Vel = FVector2D::ZeroVector; Dir->Shake(0.3f); }
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
	for (UMeshComponent* C : Meshes)
	{
		if (USkeletalMeshComponent* K = Cast<USkeletalMeshComponent>(C)) { K->SetPlayRate(bMoving ? FMath::Clamp(Vel.Size() / FMath::Max(10.f, Spd), 0.6f, 2.f) : 0.25f); }
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
