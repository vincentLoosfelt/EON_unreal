#include "EONHUD.h"
#include "EONBoss.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONPedestal.h"
#include "EONPlayerController.h"
#include "EONTileRoom.h"
#include "EngineUtils.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "Engine/Canvas.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Couleurs du proto (COL) : rouge PV, cyan énergie, fond sombre.
	const FLinearColor Red = FLinearColor(FColor(255, 61, 85));
	const FLinearColor RedDark = FLinearColor(FColor(70, 20, 34));
	const FLinearColor Cyan = FLinearColor(FColor(62, 242, 224));
	const FLinearColor Dim = FLinearColor(FColor(90, 86, 120));
	const FLinearColor Ink = FLinearColor(0.004f, 0.004f, 0.012f, 0.75f);
	FLinearColor RoleColor(FName R)
	{
		if (R == TEXT("COMBAT") || R == TEXT("CORRIDOR")) { return FLinearColor(FColor(229, 70, 196)); }
		if (R == TEXT("TENSION") || R == TEXT("FAILLE")) { return FLinearColor(FColor(255, 140, 50)); }
		if (R == TEXT("CALME")) { return FLinearColor(FColor(120, 220, 120)); }
		if (R == TEXT("OPPORTUNITE") || R == TEXT("MARCHAND") || R == TEXT("TRESOR")) { return FLinearColor(FColor(255, 194, 61)); }
		if (R == TEXT("ASCENSEUR") || R == TEXT("SECURITE")) { return Red; }
		if (R == TEXT("EVENEMENT") || R == TEXT("CARTO")) { return FLinearColor(FColor(184, 156, 255)); }
		return FLinearColor(FColor(216, 212, 240));
	}
}

void AEONHUD::Rect(float X, float Y, float W, float H, const FLinearColor& C)
{
	FCanvasTileItem T(FVector2D(X, Y), FVector2D(W, H), C);
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void AEONHUD::Txt(const FString& S, float X, float Y, const FLinearColor& C, float Scale, bool bCenter)
{
	UFont* F = GEngine ? GEngine->GetMediumFont() : nullptr;
	FCanvasTextItem T(FVector2D(X, Y), FText::FromString(S), F, C);
	T.bCentreX = bCenter;
	T.Scale = FVector2D(Scale, Scale);
	T.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(T);
}

void AEONHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }
	Time += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	U = Canvas->ClipY / 270.f;   // écran interne du proto : 270 px de haut
	const AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!J) { return; }
	const float SX = Canvas->ClipX, SY = Canvas->ClipY;

	// --- Mutation : teinte rouge qui pulse ; stade 2 aberration (bandes décalées) ; stade 3 parasites et neige.
	const int32 Mut = J->GetMut();
	if (Mut > 0)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * (4.f + 2.f * Mut));
		Rect(0, 0, SX, SY, FLinearColor(0.6f, 0.02f, 0.08f, (0.05f + 0.04f * Mut) * (0.6f + 0.4f * Pulse)));
		if (Mut >= 2)
		{
			Rect(2 * U, 0, SX, SY, FLinearColor(1.f, 0.f, 0.f, 0.035f));
			Rect(-2 * U, 0, SX, SY, FLinearColor(0.f, 0.9f, 1.f, 0.03f));
		}
		const int32 NBands = Mut >= 3 ? 7 : 2;
		for (int32 I = 0; I < NBands; ++I)
		{
			const float By = FMath::Fmod(Time * (60.f + 37.f * I) + I * 97.f, SY);
			Rect(0, By, SX, (Mut >= 3 ? 3.f : 1.5f) * U, FLinearColor(1.f, 0.9f, 0.95f, Mut >= 3 ? 0.08f : 0.04f));
		}
		if (Mut >= 3)
		{
			for (int32 I = 0; I < 160; ++I) { Rect(FMath::FRand() * SX, FMath::FRand() * SY, U, U, FLinearColor(1.f, 1.f, 1.f, 0.25f)); }
		}
	}

	// --- Flash rouge (flashHurt)
	if (J->GetHurtFlash() > 0.f) { Rect(0, 0, SX, SY, FLinearColor(1.f, 0.05f, 0.1f, J->GetHurtFlash() * 1.2f)); }

	// --- En haut à gauche : PV, énergie, dash
	float X = 6 * U, Y = 6 * U;
	for (int32 I = 0; I < J->GetMaxHP(); ++I) { Rect(X + I * 7 * U, Y, 6 * U, 6 * U, I < J->GetHP() ? Red : RedDark); }
	// Bouclier (cassette Shield) : un carré cyan après les cœurs.
	for (int32 I = 0; I < J->GetShieldMax(); ++I) { Rect(X + J->GetMaxHP() * 7 * U + I * 7 * U, Y, 6 * U, 6 * U, I < J->GetShield() ? Cyan : Ink); }
	Y += 9 * U;
	const float EMax = FMath::Max(1.f, J->GetEnergyMax());
	const float E = J->GetEnergy() / EMax;
	const float CapK = J->GetEnergyCap() / EMax;
	Rect(X, Y, 60 * U, 4 * U, Ink);
	// Part d'énergie max perdue à cause de l'armure : rose au bout de la barre (hachures simplifiées).
	if (CapK < 1.f)
	{
		for (float Hx = 60 * U * CapK; Hx < 60 * U; Hx += 2 * U) { Rect(X + Hx, Y, U, 4 * U, FLinearColor(FColor(224, 90, 138))); }
	}
	if (Mut > 0)
	{
		// En mutation : barre rouge qui clignote de plus en plus vite, « Mutation I / II / III », trois carrés de stade.
		const bool bOn = FMath::Fmod(Time * (2.f + 2.f * Mut), 1.f) < 0.6f;
		Rect(X, Y, 60 * U * CapK, 4 * U, bOn ? Red : RedDark);
		static const TCHAR* Roman[4] = { TEXT(""), TEXT("I"), TEXT("II"), TEXT("III") };
		Txt(FString::Printf(TEXT("Mutation %s"), Roman[FMath::Clamp(Mut, 0, 3)]), X + 62 * U, Y - 2.5f * U, Red, U / 4.f);
		for (int32 I = 0; I < 3; ++I) { Rect(X + 100 * U + I * 4 * U, Y, 3 * U, 3 * U, I < Mut ? Red : RedDark); }
	}
	else
	{
		const bool bBlink = E < 0.25f && FMath::Fmod(Time, 0.3f) < 0.15f;
		Rect(X, Y, 60 * U * E, 4 * U, bBlink ? FLinearColor::White : Cyan);
		Txt(TEXT("Energie"), X + 62 * U, Y - 2.5f * U, Cyan, U / 4.f);
	}
	Y += 7 * U;
	for (int32 I = 0; I < J->GetDashMax(); ++I) { Rect(X + I * 9 * U, Y, 7 * U, 1.5f * U, I < J->GetDashCharges() ? FLinearColor(FColor(138, 132, 184)) : Ink); }
	// ADN (expérience) : barre verte, niveau ; scrap en ambre.
	Y += 4 * U;
	{
		const FLinearColor Green(FColor(138, 240, 122)), Amber(FColor(255, 194, 61));
		Rect(X, Y, 60 * U, 2 * U, Ink);
		Rect(X, Y, 60 * U * FMath::Clamp(J->GetADN() / J->GetADNNeed(), 0.f, 1.f), 2 * U, Green);
		Txt(FString::Printf(TEXT("Niv %d"), J->GetLevel()) + (J->GetLevelPending() > 0 ? TEXT(" +") : TEXT("")), X + 62 * U, Y - 3 * U, Green, U / 4.f);
		Txt(FString::Printf(TEXT("Scrap %d"), J->GetScrap()), X + 82 * U, Y - 3 * U, Amber, U / 4.f);
	}

	// --- En bas à gauche : greffe active
	if (const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this))
	{
		if (UDataTable* G = T->GetGreffes())
		{
			if (const FEONGreffeRow* Row = G->FindRow<FEONGreffeRow>(J->GetGreffeId(), TEXT("HUD"), false))
			{
				Txt(TEXT("Cobra : ") + Row->DisplayName, 6 * U, SY - 24 * U, Row->Col, U / 3.f);
				// Charge de Rafale / Laser
				const float Ch = J->GetCharge01();
				if (Ch > 0.f)
				{
					Rect(6 * U, SY - 27 * U, 40 * U, 1.5f * U, Ink);
					Rect(6 * U, SY - 27 * U, 40 * U * Ch, 1.5f * U, Ch >= 1.f ? FLinearColor::White : Row->Col);
				}
			}
		}
	}

	// --- Onde, Holo, Ultime : touche + barre de recharge ; grisés en recharge, rouge sombre si la mutation les bloque.
	{
		const bool bBlk = J->AreSpellsBlocked();
		auto Slot = [&](float Sx, const TCHAR* Key, const TCHAR* Name, float K01, bool bReady)
		{
			const FLinearColor C = bBlk && FCString::Strcmp(Name, TEXT("Ultime")) != 0 ? RedDark : (bReady ? Cyan : Dim);
			Txt(FString::Printf(TEXT("%s %s"), Key, Name), Sx, SY - 14 * U, C, U / 4.f);
			Rect(Sx, SY - 6 * U, 30 * U, 1.5f * U, Ink);
			Rect(Sx, SY - 6 * U, 30 * U * FMath::Clamp(K01, 0.f, 1.f), 1.5f * U, C);
		};
		Slot(6 * U, TEXT("E"), TEXT("Onde"), 1.f - FMath::Max(0.f, J->GetSpellCooldown()) / 0.9f, J->GetSpellCooldown() <= 0.f);
		Slot(42 * U, TEXT("F"), TEXT("Holo"), 1.f - FMath::Max(0.f, J->GetHoloCooldown()) / 7.f, J->GetHoloCooldown() <= 0.f);
		const float Ult = J->GetUlt();
		const bool bReady = Ult >= 100.f;
		Slot(78 * U, TEXT("R"), TEXT("Ultime"), Ult / 100.f, bReady);
		if (bReady && FMath::Fmod(Time, 0.5f) < 0.3f) { Txt(TEXT("PRET"), 110 * U, SY - 14 * U, FLinearColor::White, U / 4.f); }
		// Armure de mutation : niveau permanent
		if (J->GetArmorPerm() > 0) { Txt(FString::Printf(TEXT("Armure %d"), J->GetArmorPerm()), 6 * U, SY - 34 * U, FLinearColor(FColor(224, 90, 138)), U / 4.f); }
	}
	if (const AEONFloorManager* FC = AEONFloorManager::Get(this))
	{
		// Carte rouge en main : petite carte rouge et son nom (sous les cœurs).
		if (FC->HasRedCard())
		{
			Rect(6 * U, 34 * U, 9 * U, 6 * U, FLinearColor(FColor(90, 10, 20)));
			Rect(7 * U, 35 * U, 7 * U, 4 * U, Red);
			Rect(7 * U, 36 * U, 7 * U, U, FLinearColor::White);
			Txt(TEXT("Carte rouge"), 18 * U, 34 * U, Red, U / 4.f);
		}
		// Porte intérieure fermée à portée : invite.
		if (const AEONTileRoom* Rm = FC->GetRoom(); Rm && !(FC->IsHubMode() && FC->GetCurrentRole() == TEXT("DEPART")))
		{
			if (const APawn* Pw = GetOwningPawn(); Pw && Rm->IsNearClosedInnerDoor(Rm->WorldToRoomPx(Pw->GetActorLocation()), 40.f))
			{
				Txt(TEXT("Interagir : ouvrir la porte"), SX * 0.5f, SY * 0.7f, Cyan, U / 3.f, true);
			}
		}
		// Terminal du C.O.M : invite du levier.
		if (FC->IsNearComTerminal() && !FC->IsComTravelling())
		{
			Txt(FC->IsFloorDone() ? TEXT("Interagir : tirer le levier - descendre") : TEXT("Interagir : terminal du C.O.M"), SX * 0.5f, SY * 0.7f, FC->IsFloorDone() ? Cyan : Dim, U / 3.f, true);
		}
	}

	// --- Gardien : carton-titre (nom lettre par lettre, sous-titre, 2,6 s) et barre de vie en bas au centre, encoches aux seuils de phase.
	if (const AEONBoss* Bs = AEONBoss::GetActive(this))
	{
		if (Bs->GetTitleT() > 0.f)
		{
			const float K = 1.f - Bs->GetTitleT() / 2.6f;
			const FString Nm = Bs->GetBossName();
			const int32 N = FMath::Clamp(FMath::CeilToInt(K * 2.2f * Nm.Len()), 0, Nm.Len());
			const float Al = FMath::Min(1.f, Bs->GetTitleT() / 0.4f);
			Rect(0, SY * 0.36f, SX, 34 * U, FLinearColor(0.02f, 0.02f, 0.05f, 0.7f * Al));
			Txt(Nm.Left(N), SX * 0.5f, SY * 0.36f + 4 * U, FLinearColor(1.f, 0.69f, 0.75f, Al), U / 1.6f, true);
			Txt(Bs->GetBossSub(), SX * 0.5f, SY * 0.36f + 22 * U, FLinearColor(0.85f, 0.83f, 0.94f, Al), U / 3.5f, true);
		}
		if (!Bs->IsInIntro() || Bs->GetTitleT() > 0.f)
		{
			const float W = 150 * U, H = 5 * U, Bx = (SX - W) * 0.5f, By = SY - 20 * U;
			const float R = FMath::Clamp(Bs->Hp / FMath::Max(1.f, Bs->MaxHp), 0.f, 1.f);
			Txt(Bs->GetBossName(), SX * 0.5f, By - 9 * U, FLinearColor(1.f, 0.69f, 0.75f), U / 3.5f, true);
			Rect(Bx - U, By - U, W + 2 * U, H + 2 * U, FLinearColor(0.f, 0.f, 0.f, 0.8f));
			Rect(Bx, By, W * R, H, Bs->IsInvulnerableNow() ? FLinearColor(0.45f, 0.45f, 0.5f) : Red);
			for (const float Q : { 0.66f, 0.33f }) { Rect(Bx + W * Q - U * 0.5f, By - U, U, H + 2 * U, FLinearColor(0.9f, 0.9f, 0.95f)); }
		}
	}

	// --- En haut à droite : mini-carte, rôle, étage
	if (const AEONFloorManager* F = AEONFloorManager::Get(this))
	{
		DrawMinimap(U);
		const FName R = F->GetCurrentRole();
		Txt(R.ToString(), SX - 80 * U, 64 * U, RoleColor(R), U / 3.5f);
		Txt(FString::Printf(TEXT("Etage %d"), F->GetFloorN()), SX - 80 * U, 72 * U, Dim, U / 3.5f);
		// --- Bannière centrale (1,8 s)
		if (F->BannerT > 0.f)
		{
			const float A = FMath::Min(1.f, F->BannerT / 0.3f);
			Rect(0, SY * 0.3f, SX, 16 * U, FLinearColor(0.f, 0.f, 0.02f, 0.6f * A));
			FLinearColor C = F->BannerColor; C.A = A;
			Txt(F->BannerText, SX * 0.5f, SY * 0.3f + 3.5f * U, C, U / 2.5f, true);
		}
	}

	// --- QTE de l'ultime : « Clavier thoracique », 5 flèches, chrono.
	if (J->IsInQTE())
	{
		Rect(0, 0, SX, SY, FLinearColor(0.027f, 0.024f, 0.06f, 0.6f));
		const TArray<uint8>& Seq = J->GetQTESeq();
		const int32 N = Seq.Num();
		const float Bw = 26 * U, Gp = 8 * U, Tot = N * Bw + (N - 1) * Gp, X0 = (SX - Tot) * 0.5f, Y0 = SY * 0.5f - 14 * U;
		Txt(TEXT("Clavier thoracique"), SX * 0.5f, Y0 - 24 * U, Cyan, U / 2.5f, true);
		static const TCHAR* Arrow[5] = { TEXT("?"), TEXT("^"), TEXT("v"), TEXT("<"), TEXT(">") };
		static const FColor QCol[5] = { FColor::White, FColor(62, 242, 224), FColor(255, 194, 61), FColor(255, 61, 139), FColor(111, 191, 74) };
		for (int32 I = 0; I < N; ++I)
		{
			const float Bx = X0 + I * (Bw + Gp);
			const bool bDone = I < J->GetQTEIndex(), bCur = I == J->GetQTEIndex();
			if (bCur) { Rect(Bx - 2 * U, Y0 - 2 * U, Bw + 4 * U, Bw + 4 * U, FLinearColor::White); }
			Rect(Bx, Y0, Bw, Bw, bDone ? FLinearColor(QCol[Seq[I]]) : FLinearColor(FColor(26, 24, 56)));
			Txt(Arrow[FMath::Clamp((int32)Seq[I], 0, 4)], Bx + Bw * 0.5f, Y0 + 4 * U, bDone ? FLinearColor(FColor(13, 11, 26)) : FLinearColor(QCol[Seq[I]]), U / 1.6f, true);
		}
		const float K = J->GetQTETime01();
		Rect(X0, Y0 + Bw + 8 * U, Tot, 3 * U, FLinearColor(FColor(26, 24, 56)));
		Rect(X0, Y0 + Bw + 8 * U, Tot * K, 3 * U, K < 0.3f ? Red : Cyan);
		Txt(TEXT("ZQSD / fleches / croix directionnelle"), SX * 0.5f, Y0 + Bw + 16 * U, Dim, U / 4.f, true);
	}
	// --- Cut-in « Protocole thoracique » (1,1 s)
	if (J->GetCutin() > 0.f)
	{
		const float Ct = J->GetCutin(), K = 1.f - Ct / 1.1f, A = FMath::Min3(1.f, K * 6.f, Ct * 5.f);
		const float Cy = SY * 0.5f - 34 * U;
		Rect(0, Cy + 2 * U, SX, 70 * U, FLinearColor(0.027f, 0.024f, 0.06f, 0.95f * A));
		Rect(0, Cy + 9 * U, SX, U, FLinearColor(0.24f, 0.95f, 0.88f, A));
		Rect(0, Cy + 71 * U, SX, U, FLinearColor(0.24f, 0.95f, 0.88f, A));
		const float Px = (-60.f + FMath::Min(1.f, K * 3.f) * 100.f) * U;
		FLinearColor W = FLinearColor::White; W.A = A;
		FLinearColor C = Cyan; C.A = A;
		Txt(TEXT("Protocole thoracique"), Px + 80 * U, Cy + 22 * U, W, U / 2.f);
		Txt(TEXT("Frappe de saturation - tous les ennemis de la salle"), Px + 82 * U, Cy + 46 * U, C, U / 4.f);
	}

	// --- Fiche de l'objet du socle le plus proche (40 px) : nom, rareté, effet, prix.
	if (const AEONFloorManager* FP = AEONFloorManager::Get(this))
	{
		if (AEONTileRoom* Rm = FP->GetRoom())
		{
			if (AEONPedestal* Pd = AEONPedestal::Nearest(GetWorld(), Rm->WorldToRoomPx(J->GetActorLocation()), 40.f))
			{
				if (const FEONItemDef* It = EONItems::Find(Pd->Item))
				{
					const FLinearColor Rc = EONItems::Color(It->Rar);
					const float Cw = 150 * U, Ch = 40 * U, Cx = SX * 0.5f - Cw * 0.5f, Cy = SY * 0.72f;
					Rect(Cx, Cy, Cw, Ch, FLinearColor(0.05f, 0.04f, 0.1f, 0.88f));
					Rect(Cx, Cy, Cw, U, Rc);
					Txt(It->Name, SX * 0.5f, Cy + 3 * U, Rc, U / 3.f, true);
					Txt(EONItems::RarName(It->Rar) + TEXT(" - ") + It->Cat.ToString(), SX * 0.5f, Cy + 14 * U, Dim, U / 4.5f, true);
					Txt(It->Desc, SX * 0.5f, Cy + 22 * U, FLinearColor(FColor(207, 211, 255)), U / 4.f, true);
					if (Pd->Price > 0) { Txt(FString::Printf(TEXT("%d scrap"), Pd->Price), SX * 0.5f, Cy + 31 * U, J->GetScrap() >= Pd->Price ? FLinearColor(FColor(255, 194, 61)) : Red, U / 4.f, true); }
				}
			}
		}
	}

	// --- Choix de niveau (drawLevelUp) : « NIVEAU N - ADN ASSIMILE », 3 cartes.
	if (J->IsLevelUpOpen())
	{
		const float K = 1.f;   // jeu en pause : pas de fondu
		Rect(0, 0, SX, SY, FLinearColor(FColor(7, 6, 15, (uint8)(200 * K))));
		const FLinearColor Green(FColor(138, 240, 122));
		Txt(FString::Printf(TEXT("NIVEAU %d - ADN ASSIMILE"), J->GetLevel()), SX * 0.5f, SY * 0.5f - 70 * U, Green, U / 2.f, true);
		Txt(TEXT("Choisis une amelioration  [1] [2] [3]  ou greffe suivante puis tir"), SX * 0.5f, SY * 0.5f - 52 * U, Dim, U / 4.f, true);
		const TArray<FName>& Opts = J->GetLevelOpts();
		const float Cw = 132 * U, Chh = 74 * U, Gap = 10 * U, X0 = SX * 0.5f - (Cw * 3 + Gap * 2) * 0.5f, Cy = SY * 0.5f - Chh * 0.5f + 6 * U;
		for (int32 I = 0; I < Opts.Num(); ++I)
		{
			const FEONItemDef* It = EONItems::Find(Opts[I]);
			if (!It) { continue; }
			const float Cx = X0 + I * (Cw + Gap);
			const bool bSel = I == J->GetLevelSel();
			const FLinearColor Rc = EONItems::Color(It->Rar);
			Rect(Cx, Cy, Cw, Chh, bSel ? FLinearColor(FColor(34, 29, 68)) : FLinearColor(FColor(20, 17, 40)));
			const FLinearColor Bc = bSel ? Rc : FLinearColor(FColor(58, 52, 96));
			Rect(Cx, Cy, Cw, U, Bc); Rect(Cx, Cy + Chh - U, Cw, U, Bc); Rect(Cx, Cy, U, Chh, Bc); Rect(Cx + Cw - U, Cy, U, Chh, Bc);
			Txt(FString::Printf(TEXT("[%d]"), I + 1), Cx + 4 * U, Cy + 4 * U, Dim, U / 4.f);
			Txt(It->Cat.ToString(), Cx + Cw - 40 * U, Cy + 4 * U, It->Cat == TEXT("PERK") ? FLinearColor(FColor(229, 70, 196)) : Cyan, U / 4.5f);
			Rect(Cx + Cw * 0.5f - 6 * U, Cy + 16 * U, 12 * U, 12 * U, Rc);
			Txt(It->Name, Cx + Cw * 0.5f, Cy + 32 * U, Rc, U / 3.2f, true);
			Txt(EONItems::RarName(It->Rar), Cx + Cw * 0.5f, Cy + 42 * U, Rc, U / 4.5f, true);
			Txt(It->Desc, Cx + Cw * 0.5f, Cy + 54 * U, FLinearColor(FColor(207, 211, 255)), U / 4.5f, true);
		}
	}

	// --- Mort : « Signal perdu » (drawDead)
	if (J->IsDead())
	{
		Rect(0, 0, SX, SY, FLinearColor(0.01f, 0.f, 0.02f, 0.75f));
		Txt(TEXT("SIGNAL PERDU"), SX * 0.5f, SY * 0.4f, Red, U / 1.6f, true);
		Txt(TEXT("R : nouvelle run"), SX * 0.5f, SY * 0.4f + 22 * U, Dim, U / 3.f, true);
	}
	DrawDevMenu();
}

void AEONHUD::DrawDevMenu()
{
	const AEONPlayerController* PC = Cast<AEONPlayerController>(GetOwningPlayerController());
	if (!PC || !PC->GetDevMenu().IsOpen()) { return; }
	FString Title, Footer;
	TArray<FEONMenuLine> Lines;
	PC->GetDevMenu().Build(PC, Title, Lines, Footer);
	const bool bTweak = PC->GetDevMenu().GetOpen() == EEONDevMenu::Tweak;
	const FLinearColor Accent = bTweak ? Cyan : FLinearColor(FColor(255, 194, 61));
	const FLinearColor Amber = FLinearColor(FColor(255, 194, 61));
	const FLinearColor Text = FLinearColor(FColor(216, 212, 240));
	const FLinearColor On = FLinearColor(FColor(120, 230, 120));

	// Panneau à droite, sous la mini-carte : 150 px du proto de large.
	const float Lh = 7.5f * U, W = 160.f * U;
	const float X0 = Canvas->ClipX - W - 6 * U, Y0 = 40 * U;
	const float H = (Lines.Num() + 5) * Lh;
	Rect(X0, Y0, W, H, FLinearColor(0.012f, 0.01f, 0.03f, 0.88f));
	Rect(X0, Y0, W, U * 0.6f, Accent);
	Txt(Title, X0 + 4 * U, Y0 + 2 * U, Accent, U / 3.2f);
	float Y = Y0 + 2 * Lh;
	for (const FEONMenuLine& L : Lines)
	{
		if (L.bSection)
		{
			Txt(L.Label.ToUpper(), X0 + 4 * U, Y + U, Dim, U / 4.2f);
			Rect(X0 + 4 * U, Y + Lh - 1.5f * U, W - 8 * U, U * 0.3f, FLinearColor(Dim.R, Dim.G, Dim.B, 0.5f));
			Y += Lh;
			continue;
		}
		if (L.bSelected)
		{
			Rect(X0 + 2 * U, Y, W - 4 * U, Lh, FLinearColor(Accent.R, Accent.G, Accent.B, 0.18f));
			Rect(X0 + 2 * U, Y, U, Lh, Accent);
		}
		Txt(L.Label, X0 + 6 * U, Y + 1.2f * U, L.bSelected ? FLinearColor::White : Text, U / 4.f);
		const FLinearColor Vc = L.bChanged ? Amber : (L.Value == TEXT("OUI") ? On : (L.bSelected ? FLinearColor::White : Text));
		Txt(L.Value, X0 + W - 34 * U, Y + 1.2f * U, Vc, U / 4.f);
		if (!L.Hint.IsEmpty()) { Txt(TEXT("studio ") + L.Hint, X0 + W - 20 * U, Y + 1.4f * U, FLinearColor(FColor(150, 146, 185)), U / 4.6f); }
		Y += Lh;
	}
	Txt(Footer, X0 + 4 * U, Y + Lh * 0.6f, Footer.StartsWith(TEXT("Fl")) ? FLinearColor(FColor(170, 166, 205)) : Amber, U / 4.4f);
}

void AEONHUD::DrawMinimap(float S)
{
	// Une case 8 × 6 px par salle, seulement les salles vues ; portes = traits ; salle courante qui clignote.
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	if (!F) { return; }
	const float CW = 8 * S, CH = 6 * S, Gap = 3 * S;
	const float X0 = Canvas->ClipX - 6 * S - 7 * (CW + Gap), Y0 = 6 * S;
	for (const auto& KV : F->GetCells())
	{
		const FEONFloorCell& C = KV.Value;
		if (!C.bSeen) { continue; }
		const float X = X0 + C.Pos.X * (CW + Gap), Y = Y0 + C.Pos.Y * (CH + Gap);
		const bool bCur = C.Pos == F->GetCurrentCell();
		FLinearColor Col = C.bVisited ? FLinearColor(FColor(70, 66, 100)) : FLinearColor(FColor(36, 34, 54));
		if (bCur && FMath::Fmod(Time, 0.6f) < 0.4f) { Col = FLinearColor(FColor(216, 212, 240)); }
		Rect(X, Y, CW, CH, Col);
		// Points : Opportunité ambre, gardien rouge, Événement violet, hostile entamée magenta.
		FLinearColor Dot(0, 0, 0, 0);
		if (C.Role == TEXT("OPPORTUNITE")) { Dot = RoleColor(C.Role); }
		else if (C.Role == TEXT("ASCENSEUR")) { Dot = F->HasRedCard() ? Cyan : Red; }
		else if (C.Role == TEXT("SECURITE") && !F->HasRedCard()) { Dot = FLinearColor(FColor(255, 194, 61)); }
		else if (C.Role == TEXT("EVENEMENT")) { Dot = RoleColor(C.Role); }
		else if (C.bVisited && !C.bCleared) { Dot = FLinearColor(FColor(229, 70, 196)); }
		if (Dot.A > 0.f) { Rect(X + CW * 0.5f - S, Y + CH * 0.5f - S, 2 * S, 2 * S, Dot); }
		if (C.Doors & EONDoor::E) { Rect(X + CW, Y + CH * 0.5f - 0.5f * S, Gap, S, Dim); }
		if (C.Doors & EONDoor::S) { Rect(X + CW * 0.5f - 0.5f * S, Y + CH, S, Gap, Dim); }
	}
}
