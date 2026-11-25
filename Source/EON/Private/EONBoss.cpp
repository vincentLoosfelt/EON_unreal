#include "EONBoss.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONCombatDirector.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONPickup.h"
#include "EONStatics.h"
#include "EONTileRoom.h"
#include "EONTypes.h"
#include "EONFx.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EONSfx.h"

TWeakObjectPtr<AEONBoss> AEONBoss::Active;

namespace
{
	constexpr float TS = 16.f;
	constexpr float PlayerR = 4.f;   // P.r du proto
	const FLinearColor ColFlesh(FColor(200, 74, 122)), ColRedL(FColor(255, 61, 85)), ColAmL(FColor(255, 194, 61));
	FORCEINLINE float Rand01() { return FMath::FRand(); }
	/** px -> cm */
	FORCEINLINE float Cm(float Px) { return EON::PxToCm(Px); }
}

AEONBoss::AEONBoss()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	SphereMesh = Sph.Object;
	CylMesh = Cyl.Object;
}

UStaticMeshComponent* AEONBoss::NewPart(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Mat)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, FName(Name));
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(true);
	C->SetupAttachment(RootComponent);
	C->RegisterComponent();
	C->SetUsingAbsoluteLocation(true);
	C->SetUsingAbsoluteRotation(true);
	C->SetUsingAbsoluteScale(true);
	if (Mat) { C->SetMaterial(0, Mat); }
	UEONStatics::MarkCelComponent(C);   // celluloïd
	return C;
}

AEONBoss* AEONBoss::GetActive(const UObject* WorldContext)
{
	AEONBoss* B = Active.Get();
	return (B && WorldContext && B->GetWorld() == WorldContext->GetWorld() && !B->bDead) ? B : nullptr;
}

void AEONBoss::Destroyed()
{
	if (Active.Get() == this) { Active.Reset(); }
	Super::Destroyed();
}

FVector AEONBoss::GetTankWorld() const
{
	const AEONTileRoom* Rm = Room.Get();
	return Rm ? Rm->RoomPxToWorld(TankC, 0.f) : GetActorLocation();
}

AEONBoss* AEONBoss::SpawnInRoom(AEONCombatDirector* Director, AEONTileRoom* Room, int32 FloorN)
{
	if (!Director || !Room) { return nullptr; }
	// La cuve : boîte englobante des tuiles K ; le gardien se tient devant, du côté du centre de la salle.
	const FEONTileGrid& G = Room->GetGrid();
	int32 X0 = 999, Y0 = 999, X1 = -1, Y1 = -1;
	for (int32 Y = 0; Y < G.H; ++Y) for (int32 X = 0; X < G.W; ++X)
	{
		if (G.Get(X, Y) == EEONTile::Tank) { X0 = FMath::Min(X0, X); Y0 = FMath::Min(Y0, Y); X1 = FMath::Max(X1, X); Y1 = FMath::Max(Y1, Y); }
	}
	const FVector2D Pad((G.W >> 1) * TS, (G.H >> 1) * TS);
	FVector2D TC = Pad + FVector2D(0.f, 60.f), TSz(64.f, 16.f);
	if (X1 >= 0)
	{
		TSz = FVector2D((X1 - X0 + 1) * TS, (Y1 - Y0 + 1) * TS);
		TC = FVector2D(X0 * TS + TSz.X * 0.5f, Y0 * TS + TSz.Y * 0.5f);
	}
	else { UE_LOG(LogEON, Warning, TEXT("EON gardien : la salle n'a pas de cuve (K), le gardien sort d'une cuve imaginaire")); }
	const FVector2D D = Pad - TC;
	FVector2D U = FMath::Abs(D.X) > FMath::Abs(D.Y) ? FVector2D(FMath::Sign(D.X), 0.f) : FVector2D(0.f, FMath::Sign(D.Y));
	if (U.IsNearlyZero()) { U = FVector2D(0.f, -1.f); }
	const FVector2D Anchor = TC + FVector2D(U.X * (TSz.X * 0.5f + 24.f), U.Y * (TSz.Y * 0.5f + 24.f));

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEONBoss* B = Room->GetWorld()->SpawnActor<AEONBoss>(AEONBoss::StaticClass(), Room->RoomPxToWorld(Anchor), FRotator::ZeroRotator, P);
	if (!B) { return nullptr; }
	if (!B->Init(Director, Room, TEXT("boss"), Anchor, 0)) { B->Destroy(); return nullptr; }
	B->FloorN = FMath::Max(1, FloorN);
	B->Hp = B->MaxHp = FMath::RoundToFloat(170.f * UEONStatics::GetGameFeel(B).BossHp * (1.f + 0.35f * (B->FloorN - 1)));
	B->R = 20.f;
	B->Anchor = Anchor; B->TankC = TC; B->TankU = U; B->TankSize = TSz;

	// --- Visuel
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"));
	auto Mk = [&](const FLinearColor& C, const FLinearColor& Em)
	{
		UMaterialInstanceDynamic* M = Base ? UMaterialInstanceDynamic::Create(Base, B) : nullptr;
		if (M) { M->SetVectorParameterValue(TEXT("BC1"), C); M->SetVectorParameterValue(TEXT("EmisAdd"), Em); }
		return M;
	};
	B->MatFlesh = Mk(FLinearColor(FColor(138, 42, 74)), FLinearColor(0.25f, 0.02f, 0.08f));
	B->MatEye = Mk(FLinearColor(FColor(230, 236, 220)), FLinearColor(0.6f, 0.6f, 0.55f));
	B->MatPupil = Mk(FLinearColor(FColor(255, 61, 85)), FLinearColor(4.f, 0.2f, 0.3f));
	B->MatRed = Mk(FLinearColor(FColor(255, 61, 85)), FLinearColor(3.f, 0.15f, 0.25f));
	B->MatOrange = Mk(FLinearColor(FColor(255, 154, 61)), FLinearColor(3.f, 1.2f, 0.2f));
	B->MatGlass = Mk(FLinearColor(FColor(31, 90, 72)), FLinearColor(0.15f, 0.9f, 0.6f));
	B->Body = B->NewPart(TEXT("Corps"), B->SphereMesh, B->MatFlesh);
	B->Eye = B->NewPart(TEXT("Oeil"), B->SphereMesh, B->MatEye);
	B->Pupil = B->NewPart(TEXT("Pupille"), B->SphereMesh, B->MatPupil);
	for (int32 I = 0; I < 4 * 6; ++I) { B->RestTents.Add(B->NewPart(*FString::Printf(TEXT("TentRepos_%02d"), I), B->SphereMesh, B->MatFlesh)); }
	B->TankGlass = B->NewPart(TEXT("CuveVitre"), B->CylMesh, B->MatGlass);
	B->TankGlass->SetCastShadow(false);

	Active = B;
	Director->AddEnemy(B);
	UE_LOG(LogEON, Display, TEXT("EON gardien : Le Tentaculaire, %d PV, cuve en (%.0f, %.0f) px, devant en (%.0f, %.0f)"), FMath::RoundToInt(B->MaxHp), TC.X, TC.Y, Anchor.X, Anchor.Y);
	B->UpdateVisual(0.f, Anchor);
	return B;
}

// ---------------------------------------------------------------- attaques

float AEONBoss::AimAt(const FVector2D& Tg, bool bLead) const
{
	const FVector2D P = Tg + (bLead ? JiraVel * 0.45f : FVector2D::ZeroVector);
	return FMath::Atan2(P.Y - Pos.Y, P.X - Pos.X);
}

void AEONBoss::Shot(float A, float Speed, float Life)
{
	if (AEONCombatDirector* D = Director.Get())
	{
		D->FireEnemyBullet(FVector2D(Pos.X, Pos.Y - 9.f), FVector2D(FMath::Cos(A), FMath::Sin(A)) * Speed, Life, false, 9.f);
	}
}

void AEONBoss::FanShot(int32 N, float Spread, float Speed)
{
	const float B = AimAt(LastTg, false);
	for (int32 I = 0; I < N; ++I)
	{
		const float A = B + (N > 1 ? (float(I) / (N - 1) - 0.5f) * Spread : 0.f);
		Shot(A, Speed, 4.f);
	}
}

void AEONBoss::AddTent(float Ang, float Tele)
{
	const AEONTileRoom* Rm = Room.Get();
	if (!Rm) { return; }
	const FVector2D Dv(FMath::Cos(Ang), FMath::Sin(Ang));
	float Dist = 0.f;
	while (Dist < 230.f && !Rm->SolidAtPx(Pos + Dv * (Dist + 14.f))) { Dist += 4.f; }
	FTent Nt;
	Nt.Ang = Ang; Nt.Max = FMath::Max(40.f, Dist); Nt.Len = 0.f; Nt.St = 0; Nt.T = Tele; Nt.TMax = Tele;
	Tents.Add(Nt);
}

void AEONBoss::AddMortar(const FVector2D& At)
{
	const AEONTileRoom* Rm = Room.Get();
	if (!Rm || Rm->TileAtPx(At) == EEONTile::Wall) { return; }
	FMortar M; M.P = At;
	Mortars.Add(M);
}

void AEONBoss::ClearAttacks()
{
	Tents.Reset();
	Mortars.Reset();
	if (AEONCombatDirector* D = Director.Get()) { D->ClearBullets([](const FVector2D&) { return true; }); }
}

void AEONBoss::StartPhase(int32 N)
{
	Phase = N; BSt = EBossSt::Trans; BStT = 1.5f; bInv = true;
	ClearAttacks();
	if (AEONCombatDirector* D = Director.Get()) { D->Shake(0.8f); }
	if (AEONFloorManager* F = AEONFloorManager::Get(this)) { F->ShowBanner(N == 2 ? TEXT("Phase II") : TEXT("Phase III - Rupture"), ColRedL); }
	UE_LOG(LogEON, Display, TEXT("EON gardien : phase %d (%d PV)"), N, FMath::RoundToInt(Hp));
}

void AEONBoss::HitPx(float Dmg, float Dx, float Dy, float KMul)
{
	// hitEnemy, branche du boss : les dégâts passent, jamais le recul ; rien pendant l'intro, les changements de phase et la mort.
	if (bDead || bDying) { return; }
	AEONTileRoom* Rf = Room.Get();
	if (bInv)
	{
		// Invulnérable : 3 éclats gris-violet (±45 px/s, 0,2 s) au point d'impact.
		if (Rf) { EONFx::Gerbe(this, Rf->RoomPxToWorld(FVector2D(Pos.X - Dx * 12.f, Pos.Y - Dy * 12.f), Cm(20.f)), 3, EONFx::Hex(TEXT("8f8abd")), 64.f, 0.2f); }
		return;
	}
	const FEONGameFeelRow F = UEONStatics::GetGameFeel(this);
	Hp -= Dmg;
	EONSfx::Event(this, TEXT("hit"));
	// 3 éclats vers le tireur (-d × 40 px/s ± 45), 0,3 s, rose-chair (le premier blanc).
	if (Rf) { EONFx::GerbeDir2(this, Rf->RoomPxToWorld(FVector2D(Pos.X - Dx * 12.f, Pos.Y - Dy * 10.f), Cm(20.f)), FVector(-Dx, -Dy, 0.f), 3, 3, EONFx::Hex(TEXT("c84a7a")), FLinearColor::White, 85.f, 0.3f); }
	Flash = F.HitFlash ? 0.07f : 0.f;
	AEONCombatDirector* Dir = Director.Get();
	if (AEONCharacter* J = Dir ? Dir->GetJira() : nullptr) { J->AddUlt(Dmg); }
	if (AEONTileRoom* Rm = Room.Get(); Dir && Rm && F.DmgNums)
	{
		const int32 D10 = FMath::RoundToInt(Dmg * 10.f);
		const FString Txt = D10 % 10 == 0 ? FString::FromInt(D10 / 10) : FString::Printf(TEXT("%.1f"), D10 / 10.f);
		Dir->AddNum(Rm->RoomPxToWorld(FVector2D(Pos.X + (Rand01() - 0.5f) * 16.f, Pos.Y), Cm(48.f)), Txt, Dmg >= 3.f ? FColor(255, 194, 61) : FColor::White);
	}
	if (Hp <= 0.f) { StartDeath(); }
	(void)Dx; (void)Dy; (void)KMul;
}

void AEONBoss::StartDeath()
{
	// bossDeath : tout s'efface, 1,9 s d'explosions en chaîne.
	bDying = true; bInv = true; Hp = 0.f;
	ClearAttacks();
	DeathT = 1.9f; DeathNext = 0.f;
	if (AEONCombatDirector* D = Director.Get()) { D->Hitstop(160.f); D->Shake(0.6f); }
	if (AEONFloorManager* F = AEONFloorManager::Get(this)) { F->ShowBanner(TEXT("Specimen neutralise"), ColAmL); }
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UE_LOG(LogEON, Display, TEXT("EON gardien : neutralisé"));
}

void AEONBoss::FinishDeath()
{
	AEONCombatDirector* D = Director.Get();
	AEONTileRoom* Rm = Room.Get();
	UWorld* W = GetWorld();
	if (D) { D->Shake(1.f); D->Hitstop(220.f); ++D->Kills; D->Scrap += 12; D->AdnGained += 25; }
	// Mort : 90 éclats (40 à 280 px/s, 0,6 à 1,2 s ; 2/3 rose-chair, puis jaune et blanc), anneau blanc de 70 px (0,4 s).
	if (Rm)
	{
		const FVector At = Rm->RoomPxToWorld(Pos, Cm(6.f));
		EONFx::Gerbe(this, At, 60, EONFx::Hex(TEXT("c84a7a")), 280.f, 0.9f, 1.25f, 3.f);
		EONFx::Gerbe(this, At, 15, EONFx::Hex(TEXT("ffc23d")), 280.f, 0.9f, 1.25f, 3.f);
		EONFx::Gerbe(this, At, 15, FLinearColor::White, 280.f, 0.9f, 1.25f, 3.f);
		EONFx::Anneau(this, Rm->RoomPxToWorld(Pos, 0.f), 70.f, 0.4f, FLinearColor::White);
	}
	// Butin : 12 Données (4 × 3, méta : jalon 8, créditées), 12 scrap, 25 ADN, 2 cœurs.
	if (W && Rm)
	{
		for (int32 K = 0; K < 12; ++K) { AEONPickup::Spawn(W, Rm, EEONPickup::Scrap, Pos, FVector2D((Rand01() - 0.5f) * 220.f, (Rand01() - 0.5f) * 220.f)); }
		AEONPickup::DropADN(W, Rm, Pos, 25);
		for (int32 K = 0; K < 4; ++K) { AEONPickup::Spawn(W, Rm, EEONPickup::Donnee, Pos, FVector2D((Rand01() - 0.5f) * 160.f, (Rand01() - 0.5f) * 160.f), 3.f); }
		AEONPickup::Spawn(W, Rm, EEONPickup::Heart, Pos + FVector2D(-10.f, 0.f), FVector2D(-30.f, 20.f));
		AEONPickup::Spawn(W, Rm, EEONPickup::Heart, Pos + FVector2D(10.f, 0.f), FVector2D(30.f, 20.f));
	}
	MarkDead();
	if (AEONFloorManager* F = AEONFloorManager::Get(this)) { F->OnBossDefeated(Pos); }
}

// ---------------------------------------------------------------- boucle

void AEONBoss::UpdateEnemy(float Dt, const FVector2D& Tg)
{
	AEONCombatDirector* Dir = Director.Get();
	AEONTileRoom* Rm = Room.Get();
	if (!Dir || !Rm || bDead) { return; }
	T += Dt;
	Flash = FMath::Max(0.f, Flash - Dt);
	TitleT = FMath::Max(0.f, TitleT - Dt);
	// Vitesse de Jira (pour viser en anticipant), en px/s.
	if (Dt > 0.f) { JiraVel = FMath::Lerp(JiraVel, (Tg - JiraLast) / Dt, FMath::Min(1.f, Dt * 8.f)); }
	JiraLast = Tg; LastTg = Tg;

	UpdateFx(Dt);
	if (bDying)
	{
		DeathT -= Dt; DeathNext -= Dt;
		if (DeathNext <= 0.f && DeathT > 0.2f)
		{
			Dir->Shake(0.35f); Flash = 0.06f; DeathNext = 0.13f;
			// Explosion en chaîne : 16 éclats à ±20 × ±17 px du centre (30 à 150 px/s, 0,4 s, 1 sur 2 blanc, frein 5).
			if (const AEONTileRoom* Rf = Room.Get())
			{
				const FVector2D At(Pos.X + (Rand01() - 0.5f) * 40.f, Pos.Y + (Rand01() - 0.5f) * 34.f);
				EONFx::Gerbe2(this, Rf->RoomPxToWorld(At, Cm(10.f)), 16, 2, EONFx::Hex(TEXT("c84a7a")), FLinearColor::White, 150.f, 0.4f, 1.3f, 5.f);
			}
		}
		if (DeathT <= 0.f) { FinishDeath(); }
		return;
	}
	if (BSt == EBossSt::Intro)
	{
		// La cuve se fissure, la vitre éclate (à 0,7 s de la fin), il sort.
		BStT -= Dt;
		Crack = FMath::Clamp((3.f - BStT) / 2.2f, 0.f, 1.f);
		if (Rand01() < Dt * 6.f) { Dir->Shake(0.05f + Crack * 0.15f); }
		if (BStT < 0.7f && !bTankBroken)
		{
			bTankBroken = true;
			Dir->Shake(1.f); Dir->Hitstop(140.f);
			// La cuve éclate : 60 éclats vert d'eau et blancs (50 à 250 px/s, 0,5 à 1 s, 1 sur 4 de 2 px, frein 3).
			if (const AEONTileRoom* Rf = Room.Get()) { EONFx::Gerbe2(this, Rf->RoomPxToWorld(TankC, Cm(14.f)), 60, 3, EONFx::Hex(TEXT("7affd0")), FLinearColor::White, 250.f, 0.75f, 1.25f, 3.f, 10.f); }
			TitleT = 2.6f;
		}
		if (bTankBroken) { Pop = FMath::Min(1.f, Pop + Dt * 3.f); }
		if (BStT <= 0.f) { BSt = EBossSt::Fight; bInv = false; }
		return;
	}
	Pop = 1.f;
	if (BSt == EBossSt::Trans)
	{
		BStT -= Dt;
		if (Rand01() < Dt * 10.f) { Dir->Shake(0.1f); }
		if (BStT <= 0.f) { BSt = EBossSt::Fight; bInv = false; }
		return;
	}
	const float HpR = Hp / FMath::Max(1.f, MaxHp);
	if (Phase == 1 && HpR <= 0.66f) { StartPhase(2); return; }
	if (Phase == 2 && HpR <= 0.33f) { StartPhase(3); return; }

	// Déplacement : phases 1-2, oscille de ±52 px devant la cuve ; phase 3, avance sur Jira et charge.
	if (Phase < 3)
	{
		Osc += Dt * 0.7f;
		const FVector2D Perp(-TankU.Y, TankU.X);
		const FVector2D To = Anchor + Perp * FMath::Sin(Osc) * 52.f;
		Pos += (To - Pos) * FMath::Min(1.f, Dt * 3.f);
	}
	else
	{
		ChT -= Dt;
		if (BSt == EBossSt::Fight)
		{
			const float A = AimAt(Tg, false);
			Pos += FVector2D(FMath::Cos(A), FMath::Sin(A)) * 22.f * Dt;
			if (ChT <= 0.f) { BSt = EBossSt::ChWind; BStT = 0.6f; Cha = A; }
		}
		else if (BSt == EBossSt::ChWind) { BStT -= Dt; if (BStT <= 0.f) { BSt = EBossSt::Charge; BStT = 0.55f; } }
		else if (BSt == EBossSt::Charge)
		{
			BStT -= Dt;
			Pos += FVector2D(FMath::Cos(Cha), FMath::Sin(Cha)) * 200.f * Dt;
			if (BStT <= 0.f) { BSt = EBossSt::Fight; ChT = 4.5f; }
		}
		if (Rm->CollideCircle(Pos, R, false) && BSt == EBossSt::Charge) { BSt = EBossSt::Fight; ChT = 4.5f; Dir->Shake(0.35f); }
	}

	// Attaques
	FireT -= Dt; SpT -= Dt; MorT -= Dt; TentT -= Dt;
	if (Phase == 1)
	{
		if (FireT <= 0.f) { FanShot(5, 0.7f, 95.f); FireT = 1.25f; }
		if (SpT <= 0.f) { for (int32 I = 0; I < 16; ++I) { Shot(SpA + I / 16.f * 6.283f, 58.f, 5.f); } SpA += 0.2f; SpT = 3.6f; }
	}
	else if (Phase == 2)
	{
		if (FireT <= 0.f) { FanShot(3, 0.4f, 100.f); FireT = 2.f; }
		if (MorT <= 0.f)
		{
			AddMortar(Tg);
			for (int32 K = 0; K < 2; ++K) { const float A = Rand01() * 6.283f, Dd = 30.f + Rand01() * 50.f; AddMortar(Tg + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Dd); }
			MorT = 2.8f;
		}
		if (TentT <= 0.f) { AddTent(AimAt(Tg, false), 0.75f); TentT = 2.3f; }
	}
	else
	{
		if (SpT <= 0.f) { for (const float Off : { 0.f, PI }) { Shot(SpA + Off, 68.f, 5.f); } SpA += 0.42f; SpT = 0.16f; }
		if (TentT <= 0.f && BSt == EBossSt::Fight) { AddTent(AimAt(Tg, false), 0.55f); AddTent(AimAt(Tg, true) + 0.25f, 0.6f); TentT = 1.8f; }
		if (MorT <= 0.f)
		{
			for (int32 K = 0; K < 2; ++K) { const float A = Rand01() * 6.283f, Dd = 20.f + Rand01() * 50.f; AddMortar(Tg + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Dd); }
			MorT = 4.2f;
		}
	}
	// Toucher le boss : 1 PV.
	if (FVector2D::Distance(Tg, Pos) < R + PlayerR && Dir->JiraHittable()) { Dir->HurtJira(1, Pos); }
}

void AEONBoss::UpdateFx(float Dt)
{
	AEONCombatDirector* Dir = Director.Get();
	if (!Dir) { return; }
	const FVector2D J = Dir->GetJiraPx();
	// Mortiers : explosent au bout de 1,1 s, 1 PV dans le cercle.
	for (int32 I = Mortars.Num() - 1; I >= 0; --I)
	{
		FMortar& M = Mortars[I];
		M.T -= Dt;
		if (M.T <= 0.f)
		{
			Dir->Shake(0.3f);
			if (FVector2D::Distance(J, M.P) < M.R && Dir->JiraHittable()) { Dir->HurtJira(1, M.P); }
			// Impact : 26 éclats orange (30 à 150 px/s, 0,35 à 0,65 s, 1 sur 3 blanc, frein 5) et l'anneau du cercle.
			if (const AEONTileRoom* Rf = Room.Get())
			{
				EONFx::Gerbe2(this, Rf->RoomPxToWorld(M.P, Cm(3.f)), 26, 3, EONFx::Hex(TEXT("ff9a3d")), FLinearColor::White, 150.f, 0.5f, 1.25f, 5.f);
				EONFx::Anneau(this, Rf->RoomPxToWorld(M.P, 0.f), M.R, 0.25f, EONFx::Hex(TEXT("ff9a3d")));
			}
			Mortars.RemoveAt(I);
		}
	}
	// Tentacules : annonce, frappe 0,16 s, tenue 0,4 s, retrait 0,3 s ; blesse pendant la frappe et la tenue.
	for (int32 I = Tents.Num() - 1; I >= 0; --I)
	{
		FTent& Tn = Tents[I];
		Tn.T -= Dt;
		if (Tn.St == 0) { if (Tn.T <= 0.f) { Tn.St = 1; Tn.T = 0.16f; Dir->Shake(0.12f); } }
		else if (Tn.St == 1) { Tn.Len = Tn.Max * (1.f - FMath::Max(0.f, Tn.T) / 0.16f); if (Tn.T <= 0.f) { Tn.St = 2; Tn.T = 0.4f; Tn.Len = Tn.Max; } }
		else if (Tn.St == 2) { if (Tn.T <= 0.f) { Tn.St = 3; Tn.T = 0.3f; } }
		else { Tn.Len = Tn.Max * FMath::Max(0.f, Tn.T) / 0.3f; if (Tn.T <= 0.f) { Tents.RemoveAt(I); continue; } }
		if ((Tn.St == 1 || Tn.St == 2) && !bDead)
		{
			const FVector2D Dv(FMath::Cos(Tn.Ang), FMath::Sin(Tn.Ang));
			const FVector2D Q = J - Pos;
			const float Pr = FMath::Clamp(FVector2D::DotProduct(Q, Dv), 0.f, Tn.Len);
			if ((Q - Dv * Pr).Size() < 9.f && Dir->JiraHittable()) { Dir->HurtJira(1, Pos + Dv * Pr); }
		}
	}
}

// ---------------------------------------------------------------- visuel

void AEONBoss::UpdateVisual(float Dt, const FVector2D& Tg)
{
	AEONTileRoom* Rm = Room.Get();
	if (!Rm || !Body) { return; }
	SetActorLocation(Rm->RoomPxToWorld(Pos, 0.f));
	UpdateMeshes(Dt);
}

void AEONBoss::UpdateMeshes(float Dt)
{
	AEONTileRoom* Rm = Room.Get();
	if (!Rm) { return; }
	const FVector Base = Rm->RoomPxToWorld(Pos, 0.f);
	const FVector2D J = LastTg;
	const float S = FMath::Max(0.01f, bDying ? FMath::Max(0.25f, DeathT / 1.9f) : Pop);
	const bool bShow = bTankBroken || bDying;

	// Corps : boule de chair qui respire ; rouge plus vif en phase 3, blanc quand elle est touchée.
	const float Br = 1.f + 0.04f * FMath::Sin(T * 3.f) + (BSt == EBossSt::ChWind ? 0.08f * FMath::Sin(T * 40.f) : 0.f);
	const float BodyR = Cm(20.f);
	const FVector BodyC = Base + FVector(0.f, 0.f, BodyR * 0.9f * S);
	Body->SetVisibility(bShow);
	Body->SetWorldLocation(BodyC);
	Body->SetWorldScale3D(FVector(BodyR * 2.f / 100.f * S * Br, BodyR * 2.f / 100.f * S * Br, BodyR * 1.8f / 100.f * S * Br));
	if (MatFlesh)
	{
		const bool bW = Flash > 0.f;
		const FLinearColor Em = bW ? FLinearColor(6.f, 6.f, 6.f) : Phase >= 3 ? FLinearColor(0.7f, 0.04f, 0.12f) : bInv && bShow ? FLinearColor(0.12f, 0.12f, 0.16f) : FLinearColor(0.25f, 0.02f, 0.08f);
		MatFlesh->SetVectorParameterValue(TEXT("EmisAdd"), Em);
	}
	// Œil : sur la face tournée vers Jira, pupille rouge qui la suit.
	const FVector2D Dj = (J - Pos).GetSafeNormal();
	const FVector Dir3 = FVector(Dj.X, Dj.Y, 0.35f).GetSafeNormal();
	const FVector EyeC = BodyC + Dir3 * BodyR * 0.82f * S;
	Eye->SetVisibility(bShow);
	Eye->SetWorldLocation(EyeC);
	Eye->SetWorldScale3D(FVector(0.55f * S));
	Pupil->SetVisibility(bShow);
	Pupil->SetWorldLocation(EyeC + Dir3 * 22.f * S);
	Pupil->SetWorldScale3D(FVector(0.22f * S));

	// 4 tentacules au repos : chapelets de 6 sphères qui ondulent autour du corps.
	for (int32 I = 0; I < 4; ++I)
	{
		const float A0 = PI * 0.25f + I * PI * 0.5f + 0.25f * FMath::Sin(T * 1.3f + I);
		const FVector2D Dv(FMath::Cos(A0), FMath::Sin(A0)), Pp(-Dv.Y, Dv.X);
		for (int32 K = 0; K < 6; ++K)
		{
			UStaticMeshComponent* C = RestTents[I * 6 + K];
			const float D = BodyR * 0.8f + K * 38.f;
			const float W = FMath::Sin(T * 2.6f + K * 0.7f + I * 1.7f) * K * 9.f;
			const FVector2D P2 = FVector2D(Base.X, Base.Y) + (Dv * D + Pp * W) * S;
			C->SetVisibility(bShow);
			C->SetWorldLocation(FVector(P2.X, P2.Y, Base.Z + (70.f - K * 10.f) * S));
			C->SetWorldScale3D(FVector((0.62f - K * 0.08f) * S));
		}
	}

	// Vitre de la cuve : verte, tremble en se fissurant, disparaît quand elle éclate.
	if (TankGlass)
	{
		const FVector TC = Rm->RoomPxToWorld(TankC, 0.f);
		const float Jit = bTankBroken ? 0.f : Crack * 4.f;
		TankGlass->SetVisibility(!bTankBroken);
		TankGlass->SetWorldLocation(TC + FVector(FMath::FRandRange(-Jit, Jit), FMath::FRandRange(-Jit, Jit), 110.f));
		TankGlass->SetWorldScale3D(FVector(Cm(TankSize.X) / 100.f * 0.9f, Cm(FMath::Max(TankSize.Y, 24.f)) / 100.f * 0.9f, 2.2f));
		if (MatGlass) { MatGlass->SetVectorParameterValue(TEXT("EmisAdd"), FLinearColor(0.15f + Crack * 2.f, 0.9f + Crack * 2.f, 0.6f + Crack * 2.f)); }
	}

	// Tentacules d'attaque : ligne rouge au sol pendant l'annonce, puis cylindre de chair.
	auto Pool = [&](TArray<TObjectPtr<UStaticMeshComponent>>& Arr, int32 N, const TCHAR* Prefix, UStaticMesh* Mesh, UMaterialInterface* M)
	{
		while (Arr.Num() < N) { UStaticMeshComponent* C = NewPart(*FString::Printf(TEXT("%s_%02d"), Prefix, Arr.Num()), Mesh, M); C->SetCastShadow(false); Arr.Add(C); }
		for (int32 I = N; I < Arr.Num(); ++I) { Arr[I]->SetVisibility(false); }
	};
	Pool(TeleMeshes, Tents.Num(), TEXT("Annonce"), CylMesh, MatRed);
	Pool(TentMeshes, Tents.Num(), TEXT("Tentacule"), CylMesh, MatFlesh);
	for (int32 I = 0; I < Tents.Num(); ++I)
	{
		const FTent& Tn = Tents[I];
		const float Yaw = FMath::RadiansToDegrees(Tn.Ang);
		const FVector2D Dv(FMath::Cos(Tn.Ang), FMath::Sin(Tn.Ang));
		const bool bTele = Tn.St == 0;
		const bool bBlink = FMath::Fmod(T * 14.f, 1.f) < 0.6f;
		UStaticMeshComponent* Tl = TeleMeshes[I];
		Tl->SetVisibility(bTele && bBlink);
		const FVector2D Mid = Pos + Dv * (Tn.Max * 0.5f);
		Tl->SetWorldLocation(Rm->RoomPxToWorld(Mid, 4.f));
		Tl->SetWorldRotation(FRotator(-90.f, Yaw, 0.f));
		Tl->SetWorldScale3D(FVector(0.03f, 0.18f * (1.f - Tn.T / FMath::Max(0.01f, Tn.TMax)) + 0.06f, Cm(Tn.Max) / 100.f));
		UStaticMeshComponent* Tm = TentMeshes[I];
		Tm->SetVisibility(!bTele && Tn.Len > 2.f);
		const FVector2D M2 = Pos + Dv * (Tn.Len * 0.5f);
		Tm->SetWorldLocation(Rm->RoomPxToWorld(M2, 60.f));
		Tm->SetWorldRotation(FRotator(-90.f, Yaw, 0.f));
		Tm->SetWorldScale3D(FVector(0.5f, 0.5f, FMath::Max(0.01f, Cm(Tn.Len) / 100.f)));
	}

	// Mortiers : cercle orange au sol, un disque plein qui grandit jusqu'à l'explosion.
	Pool(MortarMeshes, Mortars.Num() * 2, TEXT("Mortier"), CylMesh, MatOrange);
	for (int32 I = 0; I < Mortars.Num(); ++I)
	{
		const FMortar& M = Mortars[I];
		const float K = 1.f - FMath::Max(0.f, M.T) / M.Max;
		const FVector P = Rm->RoomPxToWorld(M.P, 3.f);
		UStaticMeshComponent* Ring = MortarMeshes[I * 2];
		UStaticMeshComponent* Fill = MortarMeshes[I * 2 + 1];
		Ring->SetVisibility(FMath::Fmod(T * 8.f, 1.f) < 0.7f);
		Ring->SetWorldLocation(P);
		Ring->SetWorldRotation(FRotator::ZeroRotator);
		Ring->SetWorldScale3D(FVector(Cm(M.R) * 2.f / 100.f, Cm(M.R) * 2.f / 100.f, 0.01f));
		Ring->SetMaterial(0, MatRed);
		Fill->SetVisibility(true);
		Fill->SetWorldLocation(P + FVector(0.f, 0.f, 2.f));
		Fill->SetWorldRotation(FRotator::ZeroRotator);
		Fill->SetWorldScale3D(FVector(Cm(M.R) * 2.f / 100.f * K, Cm(M.R) * 2.f / 100.f * K, 0.015f));
	}
	(void)Dt;
}
