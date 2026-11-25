// EON — vitrine des effets (Vincent, 24/11 : « fais-moi un rendu par FX en Full HD »).
// eon.vitrine : déclenche un à un les effets codés le 24/11 devant Jira, fige le temps au bon instant, cadre la caméra
// de près (même plongée que le jeu, via la vue de dev de l'étage) et enregistre une capture à la taille de la fenêtre
// dans Saved/Shots/Vitrine (lancer le jeu en -ResX=1920 -ResY=1080). Les effets passent par le vrai code du jeu
// (tir, dash, apparition, coups, tirs ennemis, explosion, Ikiryo qui tire) ; seuls les gardiens 2 et 3, pas encore
// portés, passent par EONFx::Sol avec les réglages de eon.fx.
#include "EONCharacter.h"
#include "EONCombatDirector.h"
#include "EONEnemy.h"
#include "EONEnemyProjectile.h"
#include "EONFloorManager.h"
#include "EONFx.h"
#include "EONFxPiece.h"
#include "EONTileRoom.h"
#include "EONTypes.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/WorldSettings.h"

// Ennemi de la vitrine COM-V9_Video (25/11 : membres coupés pour tous les ennemis) : goule, sniper, slasher, kamikaze, invocateur.
static TAutoConsoleVariable<FString> CVarVitrineEnnemi(TEXT("eon.vitrine.ennemi"), TEXT("slasher"), TEXT("Type d'ennemi de la vitrine COM-V9_Video."));

namespace EONVitrine
{
	struct FStep
	{
		FString Name;
		TFunction<void()> Start;
		/** Vrai quand il faut figer (T = secondes réelles depuis Start). */
		TFunction<bool(double)> Ready;
		/** Point à cadrer (monde) et largeur du cadre en px ; Largeur <= 0 : caméra du jeu. */
		TFunction<FVector()> Target;
		float WidthPx = 0.f;
		bool bUI = false;
		/** Garde le temps figé et enchaîne l'étape suivante sans nettoyage (avant / après). */
		bool bHold = false;
		/** Vidéo : une capture par image pendant VideoFrames images (Saved/Shots/Video), Ready appelé chaque image. */
		int32 VideoFrames = 0;
		/** Numéro dans la liste complète (noms de fichiers stables avec un filtre). */
		int32 Num = 0;
		TFunction<void()> After;
	};

	static TWeakObjectPtr<UWorld> GVWorld;
	static TArray<FStep> GSteps;
	static int32 GIdx = -1;
	static int32 GPhase = 0;   // 0 attente du départ, 1 effet en cours, 2 figé, 3 capture demandée, 4 nettoyage
	static double GT0 = 0.0;
	static FTSTicker::FDelegateHandle GHandle;
	static TWeakObjectPtr<AEONEnemy> GEnemy;
	static int32 GBullets0 = 0;
	static FVector2D GAt = FVector2D::ZeroVector;
	static FVector2D GJiraAt = FVector2D::ZeroVector;   // place de Jira dans la vitrine COM-V9_Video
	static bool GFlag = false;
	static int32 GFrame = 0;

	static AEONCharacter* Jira() { return GVWorld.IsValid() ? Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(GVWorld.Get(), 0)) : nullptr; }
	static AEONCombatDirector* Dir() { return GVWorld.IsValid() ? AEONCombatDirector::Get(GVWorld.Get()) : nullptr; }
	static FVector Feet() { const AEONCharacter* J = Jira(); return J ? J->GetActorLocation() - FVector(0.f, 0.f, J->GetSimpleCollisionHalfHeight()) : FVector::ZeroVector; }
	static FVector RoomW(const FVector2D& Px, float HPx) { AEONCombatDirector* D = Dir(); AEONTileRoom* R = D ? D->GetRoom() : nullptr; return R ? R->RoomPxToWorld(Px, EON::PxToCm(HPx)) : Feet(); }
	static FVector2D JPx() { AEONCombatDirector* D = Dir(); return D ? D->GetJiraPx() : FVector2D::ZeroVector; }
	/** Point de sol à RPx de Jira (premier angle où un rayon vers le bas touche, loin du vide). */
	static FVector2D SolLibre(float RPx)
	{
		const FVector2D J = JPx();
		for (int32 I = 0; I < 24; ++I)
		{
			const float A = PI * 0.5f + I * (2.f * PI / 24.f);
			const FVector2D C = J + FVector2D(FMath::Cos(A), FMath::Sin(A)) * RPx;
			bool bOk = true;
			for (const FVector2D& O : { FVector2D::ZeroVector, FVector2D(20.f, 0.f), FVector2D(-20.f, 0.f), FVector2D(0.f, 20.f), FVector2D(0.f, -20.f) })
			{
				const FVector W = RoomW(C + O, 0.f);
				FHitResult H;
				if (!GVWorld->LineTraceSingleByChannel(H, W + FVector(0, 0, 150.f), W - FVector(0, 0, 150.f), ECC_Visibility)) { bOk = false; break; }
			}
			if (bOk) { return C; }
		}
		return J + FVector2D(0.f, RPx);
	}
	/** Centre de la case la plus dégagée de la salle : la plus loin (en cases) d'un mur, d'un décor, d'un vide ou du
	 *  bord (Vincent, 25/11 : tourner « en plein milieu d'une pièce où il y a de la place », pas au bord du vide). */
	static FVector2D CentreDegage(int32* OutDegagement = nullptr)
	{
		AEONCombatDirector* D = Dir();
		AEONTileRoom* R = D ? D->GetRoom() : nullptr;
		if (!R) { return JPx(); }
		const FEONTileGrid& G = R->GetGrid();
		TArray<FIntPoint> Walk;
		R->GetWalkableTiles(Walk);
		int32 BestD = -1;
		FIntPoint Best(0, 0);
		for (const FIntPoint& T : Walk)
		{
			int32 Dg = 0;
			for (int32 Rr = 1; Rr <= 16; ++Rr)
			{
				bool bLibre = true;
				for (int32 Dy = -Rr; Dy <= Rr && bLibre; ++Dy)
				{
					for (int32 Dx = -Rr; Dx <= Rr; ++Dx)
					{
						if (FMath::Max(FMath::Abs(Dx), FMath::Abs(Dy)) != Rr) { continue; }
						const int32 X = T.X + Dx, Y = T.Y + Dy;
						if (!G.IsValidTile(X, Y) || R->IsTileSolid(Y * G.W + X, true)) { bLibre = false; break; }
					}
				}
				if (!bLibre) { break; }
				Dg = Rr;
			}
			if (Dg > BestD) { BestD = Dg; Best = T; }
		}
		if (OutDegagement) { *OutDegagement = BestD; }
		return AEONTileRoom::TileCenterPx(Best.X, Best.Y);
	}

	static int32 CountBullets() { int32 N = 0; for (TActorIterator<AEONEnemyProjectile> It(GVWorld.Get()); It; ++It) { ++N; } return N; }

	static void SetFrozen(bool bOn)
	{
		if (UWorld* W = GVWorld.Get()) { UGameplayStatics::SetGlobalTimeDilation(W, bOn ? 0.0001f : 1.f); }
	}

	static void SetView(const FStep& S)
	{
		AEONFloorManager* FM = AEONFloorManager::Get(GVWorld.Get());
		if (!FM) { return; }
		if (S.WidthPx <= 0.f || !S.Target) { FM->bDevView = false; return; }
		// Même plongée que la caméra du jeu (EONPlayerCameraManager : pitch du banc, lacet -90), FOV 30°.
		const float Fov = 30.f;
		const float Dist = EON::PxToCm(S.WidthPx) * 0.5f / FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f));
		const FRotator Rot = UGameplayStatics::GetPlayerCameraManager(GVWorld.Get(), 0) ? UGameplayStatics::GetPlayerCameraManager(GVWorld.Get(), 0)->GetCameraRotation() : FRotator(-55.f, -90.f, 0.f);
		const FVector T = S.Target();
		FM->bDevView = true;
		FM->DevViewLoc = T - FRotator(Rot.Pitch, Rot.Yaw, 0.f).Vector() * Dist;
		FM->DevViewRot = FRotator(Rot.Pitch, Rot.Yaw, 0.f);
		FM->DevViewFov = Fov;
	}

	/** Les ennemis de la vitrine ont 1e6 PV : on les remet à 1 avant de tout tuer. */
	static void KillTout()
	{
		AEONCombatDirector* D = Dir();
		if (!D) { return; }
		for (AEONEnemy* E : D->GetEnemies()) { if (E) { E->Hp = 1.f; } }
		D->KillAll();
	}
	static void Passer() { if (GEngine && GVWorld.IsValid()) { GEngine->Exec(GVWorld.Get(), TEXT("eon.ui passer")); } }

	static void SetHUD(bool bOn)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GVWorld.Get(), 0)) { if (PC->GetHUD()) { PC->GetHUD()->bShowHUD = bOn; } }
	}

	static void Build()
	{
		GSteps.Reset();
		const auto Fire = [](bool b) { if (AEONCharacter* J = Jira()) { J->SetAimAngle(0.f); J->SetFireHeld(b); } };
		const auto JiraAt = [](float DxPx, float HPx) { return [DxPx, HPx]() { return RoomW(JPx() + FVector2D(DxPx, 0.f), HPx); }; };

		{ FStep S; S.Name = TEXT("JIR-V13_Trainee_balle"); S.Start = [Fire]() { Fire(true); };
		  S.Ready = [](double T) { return T > 0.42; }; S.Target = JiraAt(70.f, 14.f); S.WidthPx = 190.f; S.After = [Fire]() { Fire(false); }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V2_Eclair_bouche"); S.Start = [Fire]() { Fire(true); };
		  S.Ready = [](double T)
		  {
			  if (T < 0.6) { return false; }
			  for (TActorIterator<AEONFxPiece> It(GVWorld.Get()); It; ++It) { if (!It->bShell && It->Life < 0.07f && It->Age < 0.02f && !It->Light) { return true; } }
			  return T > 4.0;
		  };
		  S.Target = JiraAt(14.f, 15.f); S.WidthPx = 70.f; S.After = [Fire]() { Fire(false); }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V14_Douilles"); S.Start = [Fire]() { Fire(true); };
		  S.Ready = [Fire](double T) { if (T > 2.0) { Fire(false); } return T > 2.3; }; S.Target = []() { return RoomW(JPx() + FVector2D(18.f, -22.f), 4.f); }; S.WidthPx = 110.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-U2_Reticule"); S.bUI = true; S.Start = [Fire]() { Fire(true); };
		  S.Ready = [](double T) { return T > 0.5; }; S.After = [Fire]() { Fire(false); }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V6_Fantomes_dash");
		  S.Start = []() { GFlag = false; if (AEONCharacter* J = Jira()) { J->SetMoveInput(FVector2D(1.f, 0.f)); } };
		  S.Ready = [](double T)
		  {
			  AEONCharacter* J = Jira();
			  if (!J) { return true; }
			  if (!GFlag && T > 0.25) { J->PressDash(); GFlag = true; GT0 = FPlatformTime::Seconds() - 0.25; return false; }
			  return GFlag && T > 0.36;
		  };
		  S.Target = JiraAt(-35.f, 14.f); S.WidthPx = 150.f; S.After = []() { if (AEONCharacter* J = Jira()) { J->SetMoveInput(FVector2D::ZeroVector); } }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V24_Veines_mutation");
		  S.Start = [Fire]() { if (AEONCharacter* J = Jira()) { J->DevSetMut(3); } Fire(true); };
		  S.Ready = [](double T) { if (AEONCharacter* J = Jira()) { J->DevSetMut(3); } return T > 2.2; };
		  S.Target = JiraAt(0.f, 17.f); S.WidthPx = 42.f; S.After = [Fire]() { Fire(false); if (AEONCharacter* J = Jira()) { J->DevSetMut(0); } }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V23_Cutin_ultime"); S.bUI = true;
		  S.Start = []() { if (AEONCharacter* J = Jira()) { J->DevUltFire(); } };
		  S.Ready = [](double T) { return T > 0.4; }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("COM-V8_Poussiere_apparition");
		  S.Start = []() { GAt = JPx() + FVector2D(70.f, 0.f); if (AEONCombatDirector* D = Dir()) { D->SpawnEnemyAt(TEXT("goule"), GAt, false); } };
		  S.Ready = [](double T) { return T > 0.12; }; S.Target = []() { return RoomW(GAt, 4.f); }; S.WidthPx = 90.f;
		  S.After = []() { KillTout(); }; GSteps.Add(S); }
		// COM-V9 en avant / après sur le même Kama : figé loin de Jira (il n'attaque pas), puis 6 trous percés dans le temps figé.
		const auto KamaCible = []() { AEONEnemy* E = GEnemy.Get(); return E ? E->GetActorLocation() + FVector(0.f, 0.f, EON::PxToCm(E->Ehh() * 0.5f)) : RoomW(GAt, 10.f); };
		{ FStep S; S.Name = TEXT("COM-V9a_Trous_avant");
		  S.Start = []()
		  {
			  GAt = SolLibre(160.f);
			  AEONEnemy* E = Dir() ? Dir()->SpawnEnemyAt(TEXT("slasher"), GAt, false) : nullptr;
			  if (E) { E->Hp = 1.0e6f; }
			  GEnemy = E;
		  };
		  S.Ready = [](double T) { return T > 0.5; };
		  S.Target = KamaCible; S.WidthPx = 38.f; S.bHold = true; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("COM-V9b_Trous_apres");
		  S.Start = []()
		  {
			  AEONEnemy* E = GEnemy.Get();
			  if (!E) { return; }
			  for (int32 I = 0; I < 6; ++I) { E->DevPunch(E->MaxHp * 0.35f, -1.f, 0.f); }
		  };
		  S.Ready = [](double) { return true; };
		  S.Target = KamaCible; S.WidthPx = 38.f; S.After = []() { KillTout(); }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("COM-V10_Tirs_ennemis");
		  S.Start = []()
		  {
			  GAt = JPx();
			  if (AEONCombatDirector* D = Dir())
			  {
				  D->FireEnemyBullet(GAt + FVector2D(-90.f, -45.f - 13.f), FVector2D(170.f, 0.f), 3.f, true, 13.f);
				  D->FireEnemyBullet(GAt + FVector2D(-70.f, -80.f - 13.f), FVector2D(150.f, 0.f), 3.f, false, 13.f);
			  }
		  };
		  S.Ready = [](double T) { return T > 0.5; }; S.Target = []() { for (TActorIterator<AEONEnemyProjectile> It(GVWorld.Get()); It; ++It) { return It->GetActorLocation() - FVector(EON::PxToCm(20.f), 0.f, 0.f); } return RoomW(GAt, 13.f); }; S.WidthPx = 70.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("ONI-V3_Lumiere_explosion");
		  S.Start = []() { GAt = JPx() + FVector2D(70.f, 0.f); if (AEONCombatDirector* D = Dir()) { D->Aoe(GAt, 26.f, 0.f, false, EONFx::Hex(TEXT("ff9a3d"))); } };
		  S.Ready = [](double T) { return T > 0.05; }; S.Target = []() { return RoomW(GAt, 4.f); }; S.WidthPx = 150.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("IKI-V2_Eclair_Ikiryo");
		  S.Start = []()
		  {
			  GAt = SolLibre(190.f);
			  GEnemy = Dir() ? Dir()->SpawnEnemyAt(TEXT("sniper"), GAt, false) : nullptr;
			  if (GEnemy.IsValid()) { GEnemy->Hp = 1.0e6f; }
			  GBullets0 = CountBullets();
		  };
		  S.Ready = [](double T) { return CountBullets() > GBullets0 || T > 15.0; };
		  S.Target = []() { AEONEnemy* E = GEnemy.Get(); return E ? E->GetActorLocation() + FVector(0.f, 0.f, EON::PxToCm(12.f)) : RoomW(GAt, 12.f); };
		  S.WidthPx = 80.f; S.After = []() { KillTout(); }; GSteps.Add(S); }
		// Gardiens 2 et 3 (pas encore portés) : mêmes appels que eon.fx.
		const auto SolAt = [](float DxPx) { return [DxPx]() { return RoomW(JPx() + FVector2D(DxPx, 0.f), 0.f); }; };
		{ FStep S; S.Name = TEXT("GAR-V18_Flaque_acide");
		  S.Start = []() { EONFx::Sol(Jira(), EONFx::EForme::Flaque, RoomW(JPx() + FVector2D(55.f, 0.f), 0.f), 0.f, 30.f, 30.f, EONFx::Hex(TEXT("7aff3a")), 4.5f, 0.f, false, 0.6f, 3.7f, 1.2f); };
		  S.Ready = [](double T) { return T > 0.5; }; S.Target = SolAt(45.f); S.WidthPx = 100.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("GAR-V18_Matiere_organique");
		  S.Start = []() { EONFx::Sol(Jira(), EONFx::EForme::Flaque, RoomW(JPx() + FVector2D(-60.f, 0.f), 0.f), 0.f, 40.f, 40.f, EONFx::Hex(TEXT("c84a7a")), 6.f, 0.f, false, 0.6f, 6.1f, 1.2f); };
		  S.Ready = [](double T) { return T > 0.5; }; S.Target = SolAt(-45.f); S.WidthPx = 110.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("GAR-V19_Cercle_pieux");
		  S.Start = []() { EONFx::Sol(Jira(), EONFx::EForme::Disque, Feet(), 0.f, 48.f, 48.f, EONFx::Hex(TEXT("ff3d55")), 0.8f, 8.f, true); };
		  S.Ready = [](double T) { return T > 0.45; }; S.Target = SolAt(0.f); S.WidthPx = 120.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("GAR-V19_Cone_balayage");
		  S.Start = []() { EONFx::Sol(Jira(), EONFx::EForme::Cone, Feet(), 0.f, 152.f, 152.f, EONFx::Hex(TEXT("ff3d55")), 0.55f, 0.f, true, 1.15f); };
		  S.Ready = [](double T) { return T > 0.32; }; S.Target = SolAt(50.f); S.WidthPx = 230.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("GAR-V19_Ligne_bras");
		  S.Start = []() { EONFx::Sol(Jira(), EONFx::EForme::Ligne, RoomW(JPx() + FVector2D(60.f, 0.f), 0.f), 0.f, 120.f, 12.f, EONFx::Hex(TEXT("ff3d55")), 0.55f, 0.f, true); };
		  S.Ready = [](double T) { return T > 0.32; }; S.Target = SolAt(60.f); S.WidthPx = 190.f; GSteps.Add(S); }
		// Armes de Jira et hologramme hors debug, mort de l'Ubume (Vincent, 24/11, questions sur les VFX).
		const auto Arme = [](FName Id) { return [Id]() { if (AEONCharacter* J = Jira()) { J->SetAimAngle(0.f); J->DevWeapon(Id, 1.f); } }; };
		{ FStep S; S.Name = TEXT("JIR-T3_Rayon_laser"); S.Start = Arme(TEXT("laser"));
		  S.Ready = [](double T) { return T > 0.05; }; S.Target = JiraAt(70.f, 14.f); S.WidthPx = 200.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-T4_Arc_lame"); S.Start = Arme(TEXT("lame"));
		  S.Ready = [](double T) { return T > 0.04; }; S.Target = JiraAt(12.f, 6.f); S.WidthPx = 90.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V21_Fouet_griffe"); S.Start = Arme(TEXT("griffe"));
		  S.Ready = [](double T) { return T > 0.05; }; S.Target = JiraAt(50.f, 14.f); S.WidthPx = 160.f; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V10.4_Rayon_ciel");
		  S.Start = []()
		  {
			  GAt = SolLibre(110.f);
			  AEONEnemy* E = Dir() ? Dir()->SpawnEnemyAt(TEXT("slasher"), GAt, false) : nullptr;
			  if (E) { E->Hp = 1.0e6f; }
			  GEnemy = E; GFlag = false;
		  };
		  S.Ready = [](double T)
		  {
			  if (!GFlag && T > 0.8) { if (AEONCharacter* J = Jira()) { J->DevUltStrike(GEnemy.Get()); } GFlag = true; GT0 = FPlatformTime::Seconds() - 0.8; return false; }
			  return GFlag && T > 0.86;
		  };
		  S.Target = []() { AEONEnemy* E = GEnemy.Get(); return E ? E->GetActorLocation() + FVector(0.f, 0.f, EON::PxToCm(14.f)) : RoomW(GAt, 14.f); };
		  S.WidthPx = 110.f; S.After = []() { KillTout(); }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("JIR-V9.2_Hologramme");
		  S.Start = []() { if (AEONCharacter* J = Jira()) { J->PressHolo(); } };
		  S.Ready = [](double T)
		  {
			  AEONCharacter* J = Jira();
			  if (!J) { return true; }
			  J->SetMoveInput(T > 0.15 && T < 0.55 ? FVector2D(0.f, 1.f) : FVector2D::ZeroVector);
			  return T > 0.7 && (J->DevHoloVisible() || T > 3.0);
		  };
		  S.Target = []() { AEONCharacter* J = Jira(); return J ? (J->GetDecoyLocation() + J->GetActorLocation()) * 0.5f : Feet(); };
		  S.WidthPx = 110.f; S.After = []() { if (AEONCharacter* J = Jira()) { J->SetMoveInput(FVector2D::ZeroVector); } }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("UBU-V2_Mort_Ubume");
		  S.Start = []() { GAt = SolLibre(90.f); GEnemy = Dir() ? Dir()->SpawnEnemyAt(TEXT("invocateur"), GAt, false) : nullptr; GFlag = false; };
		  S.Ready = [](double T)
		  {
			  if (!GFlag && T > 1.0) { KillTout(); GFlag = true; GT0 = FPlatformTime::Seconds() - 1.0; return false; }
			  return GFlag && T > 1.12;
		  };
		  S.Target = []() { return RoomW(GAt, 6.f); }; S.WidthPx = 100.f; GSteps.Add(S); }
		// Vidéo COM-V9 (Vincent, 24/11) : Jira tire sur un Kama (1e6 PV) : trous et sang. 2,5 s à vitesse réelle,
		// puis 5 s au ralenti × 0,3. Lancer le jeu en -UseFixedTimeStep -FPS=30 : une image enregistrée = 1/30 s.
		// Fumée toon (Vincent, 24/11) : une grosse bouffée devant Jira à 3 âges, puis le dash et la course.
		for (const double Age : { 0.2, 0.6, 1.1, 1.4 })
		{
			FStep S; S.Name = FString::Printf(TEXT("FUM-1_Bouffee_%03d"), (int32)(Age * 100.0));
			S.Start = []() { GAt = JPx() + FVector2D(45.f, 0.f); EONFx::Fumee(GVWorld.Get(), RoomW(GAt, 0.f), 48, EONFx::Hex(TEXT("9a7c88")), 1.8f, 1.6f); };
			S.Ready = [Age](double T) { return T > Age; }; S.Target = []() { return RoomW(GAt, 14.f); }; S.WidthPx = 110.f; GSteps.Add(S);
		}
		{ FStep S; S.Name = TEXT("FUM-2_Dash");
		  S.Start = []() { GFlag = false; if (AEONCharacter* J = Jira()) { J->SetMoveInput(FVector2D(1.f, 0.f)); } };
		  S.Ready = [](double T)
		  {
			  AEONCharacter* J = Jira();
			  if (!J) { return true; }
			  if (!GFlag && T > 0.25) { J->PressDash(); GFlag = true; GT0 = FPlatformTime::Seconds() - 0.25; return false; }
			  return GFlag && T > 0.6;
		  };
		  S.Target = JiraAt(-45.f, 10.f); S.WidthPx = 160.f; S.After = []() { if (AEONCharacter* J = Jira()) { J->SetMoveInput(FVector2D::ZeroVector); } }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("FUM-3_Course");
		  S.Start = []() { if (AEONCharacter* J = Jira()) { J->SetSprintHeld(true); J->SetMoveInput(FVector2D(-1.f, 0.f)); } };
		  S.Ready = [](double T) { return T > 1.1; };
		  S.Target = JiraAt(0.f, 8.f); S.WidthPx = 360.f;
		  S.After = []() { if (AEONCharacter* J = Jira()) { J->SetSprintHeld(false); J->SetMoveInput(FVector2D::ZeroVector); } }; GSteps.Add(S); }
		// Vidéo de la fumée (Vincent, 24/11) : bouffée, dash puis course, plan fixe, 6 s à 30 i/s (jeu en -FPS=30).
		{ FStep S; S.Name = TEXT("FUM-V_Video"); S.VideoFrames = 180;
		  S.Start = []() { GFrame = 0; GAt = JPx(); };
		  S.Ready = [](double)
		  {
			  AEONCharacter* J = Jira();
			  if (!J) { return false; }
			  if (GFrame == 0) { EONFx::Fumee(GVWorld.Get(), RoomW(GAt + FVector2D(45.f, 0.f), 0.f), 48, EONFx::Hex(TEXT("9a7c88")), 1.8f, 1.6f); }
			  if (GFrame == 55) { J->SetMoveInput(FVector2D(-1.f, 0.f)); }
			  if (GFrame == 62) { J->PressDash(); }
			  if (GFrame == 95) { J->SetMoveInput(FVector2D(1.f, 0.f)); J->SetSprintHeld(true); }
			  if (GFrame == 150) { J->SetSprintHeld(false); J->SetMoveInput(FVector2D::ZeroVector); }
			  return false;
		  };
		  // Plan fixe sur la bouffée, puis la caméra suit Jira pendant le dash et la course.
		  S.Target = []() { return GFrame < 58 ? RoomW(GAt + FVector2D(25.f, 0.f), 10.f) : RoomW(JPx(), 10.f); }; S.WidthPx = 150.f;
		  S.After = []() { if (AEONCharacter* J = Jira()) { J->SetSprintHeld(false); J->SetMoveInput(FVector2D::ZeroVector); } };
		  GSteps.Add(S); }
		// Explosion façon Akira (Vincent, 24/11) : petite boule de 1 à 50 cm en 0,5 s, éclat brutal avec flash à ombres noires, nuage de
		// fumée qui se dissipe en 4 s. Charge (EONFx::Charge) puis le vrai chemin du jeu (Aoe, sans dégâts). Plan fixe, 6 s à 30 i/s.
		{ FStep S; S.Name = TEXT("EXP-V_Video"); S.VideoFrames = 180;
		  S.Start = []() { GFrame = 0; GAt = JPx() + FVector2D(70.f, 0.f); };
		  S.Ready = [](double)
		  {
			  AEONCombatDirector* D = Dir();
			  if (!D) { return false; }
			  if (GFrame == 30) { EONFx::Charge(GVWorld.Get(), RoomW(GAt, 0.f), EON::PxToCm(28.f) * 0.8f, EONFx::Hex(TEXT("ff7a1a")), 0.5f); }
			  if (GFrame == 45) { D->Aoe(GAt, 28.f, 0.f, false, EONFx::Hex(TEXT("ff7a1a"))); }
			  return false;
		  };
		  S.Target = []() { return RoomW(GAt + FVector2D(-15.f, 8.f), 14.f); }; S.WidthPx = 170.f;
		  GSteps.Add(S); }
		// Pouvoir E (25/11) : onde de choc sur 3 ennemis, deux appuis courts puis l'onde maintenue.
		{ FStep S; S.Name = TEXT("ONDE-V_Video"); S.VideoFrames = 170;
		  S.Start = []()
		  {
			  GFrame = 0;
			  if (AEONCombatDirector* D = Dir())
			  {
				  const FVector2D J = JPx();
				  for (const FVector2D& O : { FVector2D(38.f, 0.f), FVector2D(-34.f, 14.f), FVector2D(4.f, -36.f) })
				  {
					  if (AEONEnemy* E = D->SpawnEnemyAt(TEXT("slasher"), J + O, false)) { E->Hp = 1.0e6f; }
				  }
			  }
		  };
		  S.Ready = [](double)
		  {
			  AEONCharacter* J = Jira();
			  if (!J) { return false; }
			  if (GFrame == 20 || GFrame == 60) { J->PressSpell(); }
			  if (GFrame == 100) { J->SetSpellHeld(true); J->PressSpell(); }
			  if (GFrame == 150) { J->SetSpellHeld(false); }
			  return false;
		  };
		  S.Target = []() { return Feet() + FVector(0.f, 0.f, EON::PxToCm(10.f)); }; S.WidthPx = 200.f;
		  S.After = []() { KillTout(); }; GSteps.Add(S); }
		{ FStep S; S.Name = TEXT("COM-V9_Video"); S.VideoFrames = 225;
		  S.Start = []()
		  {
			  // Dans une grande salle (eon.forme large), au milieu de sa partie la plus dégagée : Jira d'un côté, l'ennemi de
			  // l'autre, tous deux dans le cadre (Vincent, 25/11).
			  if (GEngine) { GEngine->Exec(GVWorld.Get(), TEXT("eon.forme large")); }
			  int32 Dg = 0;
			  const FVector2D C = CentreDegage(&Dg);
			  if (AEONCharacter* J = Jira())
			  {
				  J->SetActorLocation(RoomW(C - FVector2D(45.f, 0.f), 0.f) + FVector(0.f, 0.f, J->GetSimpleCollisionHalfHeight()), false, nullptr, ETeleportType::TeleportPhysics);
			  }
			  GAt = C + FVector2D(45.f, 0.f);
			  GJiraAt = C - FVector2D(45.f, 0.f);
			  UE_LOG(LogTemp, Display, TEXT("EON vitrine : centre dégagé %s, %d cases libres autour"), *C.ToString(), Dg);
			  AEONEnemy* E = Dir() ? Dir()->SpawnEnemyAt(FName(*CVarVitrineEnnemi.GetValueOnGameThread()), GAt, false) : nullptr;
			  if (E) { E->Hp = 1.0e6f; }
			  GEnemy = E; GFrame = 0;
		  };
		  S.Ready = [](double)
		  {
			  AEONCharacter* J = Jira(); AEONEnemy* E = GEnemy.Get();
			  // Critiques réglés (25/11) : un aux images 30, 75, 120 et 165, pour une séquence tête, bras, bras, jambe
			  // étalée sur le plan (au hasard, 20 % faisait tout partir en 1 s, 6 % parfois rien).
			  // Un dialogue du jeu (rupture de paroi…) met la partie en pause : la vidéo se figeait (cause de l'arrêt sur le Kama, 25/11).
			  Passer();
			  if (GFrame == 30 || GFrame == 75 || GFrame == 120 || GFrame == 165) { AEONEnemy::bCritSuivant = true; }
			  if (J && E)
			  {
				  const FVector D = E->GetActorLocation() - J->GetActorLocation();
				  J->SetAimAngle(FMath::Atan2(D.Y, D.X));
				  J->SetFireHeld(true);
			  }
			  if (E && E->Hp < 1.0e5f && E->Hp > 0.f) { E->Hp = 1.0e6f; }
			  // Ennemi gardé sur place, dans la zone éclairée (25/11 : le Nuke et l'Ubume en sortaient).
			  if (E) { E->Pos = GAt; E->Vel = FVector2D::ZeroVector; E->Knock = FVector2D::ZeroVector; }
			  // Jira aussi tenu en place (il glissait hors du cadre en tirant).
			  if (J) { J->SetActorLocation(RoomW(GJiraAt, 0.f) + FVector(0.f, 0.f, J->GetSimpleCollisionHalfHeight()), false, nullptr, ETeleportType::TeleportPhysics); }
			  return false;
		  };
		  // Plan entre Jira et l'ennemi, assez large pour voir le tireur et la cible.
		  S.Target = []() { return RoomW((GJiraAt + GAt) * 0.5f, 8.f); };
		  S.WidthPx = 160.f;
		  S.After = []() { if (AEONCharacter* J = Jira()) { J->SetFireHeld(false); } KillTout(); };
		  GSteps.Add(S); }
		for (int32 I = 0; I < GSteps.Num(); ++I) { GSteps[I].Num = I + 1; }
	}

	static bool Tick(float)
	{
		if (!GVWorld.IsValid()) { GHandle.Reset(); return false; }
		const double Now = FPlatformTime::Seconds();
		const double T = Now - GT0;
		if (GPhase == 0)
		{
			// Écran titre : « continuer » (comme le pilote), puis le temps que la salle se charge.
			if (T > 1.0 && !GFlag) { GFlag = true; if (GEngine) { GEngine->Exec(GVWorld.Get(), TEXT("eon.ui continuer")); } }
			if (T > 3.0) { Passer(); }
			if (T < 8.0) { return true; }
			GFlag = false;
			if (AEONCharacter* J = Jira()) { J->bDevDrive = true; }
			KillTout();
			GIdx = 0; GPhase = 1; GT0 = Now;
			SetHUD(GSteps[0].bUI);
			GSteps[0].Start();
			return true;
		}
		FStep& S = GSteps[GIdx];
		if (GPhase == 1 && S.VideoFrames > 0)
		{
			S.Ready(T);
			SetView(S);
			const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Shots/Video") / FString::Printf(TEXT("f_%04d.png"), GFrame));
			FScreenshotRequest::RequestScreenshot(Path, false, false);
			if (++GFrame >= S.VideoFrames)
			{
				if (AEONFloorManager* FM = AEONFloorManager::Get(GVWorld.Get())) { FM->bDevView = false; }
				if (S.After) { S.After(); }
				UE_LOG(LogTemp, Display, TEXT("EON vitrine : vidéo %d images"), GFrame);
				GPhase = 4; GT0 = Now;
			}
			return true;
		}
		if (GPhase == 1)
		{
			if (S.Ready(T))
			{
				SetFrozen(true);
				SetView(S);
				GPhase = 2; GT0 = Now;
			}
			return true;
		}
		SetFrozen(true);   // le hitstop du jeu remet la dilatation à 1 en fin de course : on la retient
		if (GPhase == 2 && T > 0.35)
		{
			const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Shots/Vitrine") / FString::Printf(TEXT("%02d_%s.png"), S.Num, *S.Name));
			FScreenshotRequest::RequestScreenshot(Path, S.bUI, false);
			UE_LOG(LogTemp, Display, TEXT("EON vitrine : capture %s"), *Path);
			GPhase = 3; GT0 = Now;
		}
		else if (GPhase == 3 && T > 1.0 && S.bHold && GSteps.IsValidIndex(GIdx + 1))
		{
			++GIdx;
			GSteps[GIdx].Start();
			SetView(GSteps[GIdx]);
			GPhase = 2; GT0 = Now;
		}
		else if (GPhase == 3 && T > 1.0)
		{
			SetFrozen(false);
			if (AEONFloorManager* FM = AEONFloorManager::Get(GVWorld.Get())) { FM->bDevView = false; }
			if (S.After) { S.After(); }
			GPhase = 4; GT0 = Now;
		}
		else if (GPhase == 4)
		{
			// Le gel cède avant le nettoyage : on attend que les restes de l'effet précédent s'éteignent.
			SetFrozen(false);
			Passer();
			if (T < 2.0) { return true; }
			if (++GIdx >= GSteps.Num())
			{
				SetHUD(true);
				if (AEONCharacter* J = Jira()) { J->bDevDrive = false; }
				UE_LOG(LogTemp, Display, TEXT("EON vitrine : fini (%d captures)"), GSteps.Num());
				GHandle.Reset();
				return false;
			}
			GPhase = 1; GT0 = Now;
			SetHUD(GSteps[GIdx].bUI);
			GSteps[GIdx].Start();
		}
		return true;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GEONCmdVitrine(TEXT("eon.vitrine"),
	TEXT("eon.vitrine : une capture par effet codé le 24/11 (Saved/Shots/Vitrine), départ 8 s après la commande"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
	{
		using namespace EONVitrine;
		if (!W || GHandle.IsValid()) { return; }
		GVWorld = W;
		Build();
		if (Args.Num()) { GSteps.RemoveAll([&Args](const FStep& S) { for (const FString& Fl : Args) { if (S.Name.Contains(Fl)) { return false; } } return true; }); }
		if (GSteps.Num() == 0) { return; }
		GIdx = -1; GPhase = 0; GFlag = false; GT0 = FPlatformTime::Seconds();
		GHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&EONVitrine::Tick));
		UE_LOG(LogTemp, Display, TEXT("EON vitrine : %d effets, départ dans 8 s"), GSteps.Num());
	}));
