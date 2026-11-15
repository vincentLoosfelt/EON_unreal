#include "EONEnemy.h"
#include "EONCharacter.h"
#include "EON.h"
#include "EONCombatDirector.h"
#include "EONSpriteComponent.h"
#include "EONStatics.h"
#include "EONTileRoom.h"
#include "EONTuningSubsystem.h"
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

	Sprite = CreateDefaultSubobject<UEONSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
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

	if (const UDataTable* SDT = Tun ? Tun->GetSprites() : nullptr)
	{
		if (const FEONSpriteRow* SR = SDT->FindRow<FEONSpriteRow>(InType, TEXT("EONEnemy"), false)) { SpriteFrames = FMath::Max(1, SR->N); }
	}
	Sprite->SpriteName = InType;
	Sprite->RefreshSprite();
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
	Sprite->DisplayScale = 1.45f;
	Sprite->RefreshSprite();
	ApplyCapsuleSize();
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

	// Image : animée en mouvement (7 i/s, méduse 4 i/s), retournée si Jira est à gauche
	const bool bMoving = Vel.Size() > 8.f || Type == TEXT("meduse");
	Sprite->Frame = bMoving ? FMath::FloorToInt(T * (Type == TEXT("meduse") ? 4.f : 7.f)) % SpriteFrames : 0;
	Sprite->bFlipX = Tg.X < Pos.X;
	const bool bWhite = Flash > 0.f || (St == EEONEnemySt::Arm && FMath::FloorToInt(T * 20.f) % 2 == 0);
	// Clignotement blanc : la teinte du sprite (multiplicative) poussée au-delà de 1.
	Sprite->Tint = bWhite ? FLinearColor(8.f, 8.f, 8.f) : FLinearColor::White;

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
