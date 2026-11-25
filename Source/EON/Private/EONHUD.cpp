#include "EONHUD.h"
#include "EONHudWidget.h"
#include "EONBoss.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONUI.h"
#include "Misc/App.h"
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
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Couleurs du proto (COL) : rouge PV, cyan énergie, fond sombre.
	const FLinearColor Red = FLinearColor(FColor(255, 61, 85));
	const FLinearColor Cyan = FLinearColor(FColor(62, 242, 224));
	const FLinearColor Dim = FLinearColor(FColor(90, 86, 120));
	const FLinearColor Ink = FLinearColor(0.004f, 0.004f, 0.012f, 0.75f);
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

	// --- Réticule (JIR-U2, proto l. 3435) : à la souris, en jeu seulement. Quatre traits de 3 px autour d'un point,
	// écartés de 3 + crossT × 4 px (le tir ouvre, ça se referme en 0,25 s). Blanc, rouge en mutation. Le curseur
	// Windows est caché tant que le réticule est dessiné.
	if (APlayerController* PCr = GetOwningPlayerController())
	{
		const AEONPlayerController* EPC = Cast<AEONPlayerController>(PCr);
		const UEONUISubsystem* UIr = UEONUISubsystem::Get(this);
		const bool bReticle = EPC && !EPC->IsUsingGamepad() && !EPC->GetDevMenu().IsOpen() && !(UIr && UIr->IsBlocking())
			&& !J->IsLevelUpOpen() && !J->IsDead();
		float Mx = 0.f, My = 0.f;
		if (bReticle && PCr->GetMousePosition(Mx, My))
		{
			PCr->CurrentMouseCursor = EMouseCursor::None;
			const float O = (3.f + FMath::RoundToFloat(J->GetCrossT() * 4.f)) * U;
			const FLinearColor Rc = Mut > 0 ? Red : FLinearColor::White;
			Rect(Mx - O - 3 * U, My - 0.5f * U, 3 * U, U, Rc);
			Rect(Mx + O, My - 0.5f * U, 3 * U, U, Rc);
			Rect(Mx - 0.5f * U, My - O - 3 * U, U, 3 * U, Rc);
			Rect(Mx - 0.5f * U, My + O, U, 3 * U, Rc);
			Rect(Mx - 0.5f * U, My - 0.5f * U, U, U, Rc);
		}
		else { PCr->CurrentMouseCursor = EMouseCursor::Crosshairs; }
	}

	// PV, énergie, dash, greffes, ADN, Onde / Holo / Hypercharge, barre du monstre et mini-carte : UEONHudWidget (charte, 25/11).
	EnsureHudWidget();

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
		// La peluche en poche (Lapin), sous la carte rouge.
		if (FC->HasDoudou()) { Txt(TEXT("Lapin (peluche)"), 6 * U, (FC->HasRedCard() ? 42 : 34) * U, FLinearColor(FColor(232, 176, 192)), U / 4.f); }
		// Repère au-dessus du PNJ d'event : « ? » de la gamine, « ?! » puis « ! » d'Ulysse.
		{
			FVector Ew; FString Et; FLinearColor Ec; FVector2D Es;
			if (FC->GetEvtHud(Ew, Et, Ec) && GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(Ew, Es))
			{
				Txt(Et, Es.X, Es.Y - 4 * U + FMath::RoundToFloat(FMath::Sin(Time * 5.f)) * U, Ec, U / 2.5f, true);
			}
		}
		// Log trouvé (drawLog) : « TRANSMISSION - LOG n/8 », le texte s'écrit à 40 caractères par seconde.
		{
			FString Lt; int32 Ln = 0; float El = 0.f;
			if (FC->GetLogMsg(Lt, Ln, El))
			{
				const float Dur = Lt.Len() / 40.f + 5.f;
				const float A = FMath::Clamp(FMath::Min3(1.f, El * 4.f, (Dur - El) * 2.f), 0.f, 1.f);
				TArray<FString> Wr;
				{
					TArray<FString> Words; Lt.ParseIntoArray(Words, TEXT(" "));
					FString Cur;
					for (const FString& Wd : Words) { if (Cur.Len() && Cur.Len() + 1 + Wd.Len() > 60) { Wr.Add(Cur); Cur = Wd; } else { Cur = Cur.Len() ? Cur + TEXT(" ") + Wd : Wd; } }
					if (Cur.Len()) { Wr.Add(Cur); }
				}
				const float Bw = 276 * U, Bx = SX * 0.5f - Bw * 0.5f, By = 24 * U, Bh = (15 + Wr.Num() * 8) * U;
				Rect(Bx, By, Bw, Bh, FLinearColor(0.027f, 0.024f, 0.06f, 0.9f * A));
				FLinearColor Cy(FColor(62, 242, 224)); Cy.A = A;
				Rect(Bx, By, Bw, U, Cy);
				Rect(Bx, By + Bh - U, Bw, U, Cy);
				Txt(FString::Printf(TEXT("TRANSMISSION - LOG %d/%d"), Ln, UEONUISubsystem::NumLogs()), Bx + 4 * U, By + 3 * U, Cy, U / 4.f);
				int32 Left = FMath::FloorToInt(El * 40.f);
				FLinearColor Tc(FColor(232, 228, 255)); Tc.A = A;
				for (int32 I = 0; I < Wr.Num() && Left > 0; ++I) { Txt(Wr[I].Left(Left), Bx + 4 * U, By + (13 + I * 8) * U, Tc, U / 4.f); Left -= Wr[I].Len(); }
			}
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
			if (const UEONUISubsystem* UI = UEONUISubsystem::Get(this); !UI || !UI->IsBlocking())
			{
				Txt(TEXT("Interagir : consulter le C.O.M"), SX * 0.5f, SY * 0.7f, FC->IsFloorDone() ? Cyan : Dim, U / 3.f, true);
			}
		}
	}

	// --- Faille : jauge d'oxygène, étiquettes de la console et de la brèche, repère de bord d'écran vers la console.
	if (const AEONFloorManager* FR = AEONFloorManager::Get(this); FR && FR->GetCurrentRole() == TEXT("FAILLE") && FR->GetRoom())
	{
		const FEONFloorCell* Cc = FR->GetCells().Find(FR->GetCurrentCell());
		APlayerController* PC = GetOwningPlayerController();
		if (Cc && PC && !Cc->bRiftDone && Cc->RiftTiles.Num())
		{
			const bool bOn = FR->IsConfinementReady();
			const FLinearColor Grn(FColor(138, 240, 122)), Yel(FColor(255, 210, 74));
			const FVector ConfW = FR->GetConfWorld();
			FVector2D Cs;
			const bool bConfVis = PC->ProjectWorldLocationToScreen(ConfW + FVector(0, 0, 60.f), Cs);
			if (bConfVis) { Txt(bOn ? TEXT("CONFINEMENT") : TEXT("HORS LIGNE"), Cs.X, Cs.Y - 10 * U, bOn ? Cyan : Dim, U / 4.f, true); }
			if (FR->IsRiftActive())
			{
				const bool bFl = FMath::Fmod(Time * 6.f, 2.f) < 1.f;
				FVector2D Bs;
				if (PC->ProjectWorldLocationToScreen(FR->GetRoom()->RoomPxToWorld(Cc->RiftPx, 20.f), Bs))
				{
					Txt(TEXT("BRECHE"), Bs.X, Bs.Y - 6 * U, bFl ? FLinearColor::White : Red, U / 4.f, true);
				}
				// Jauge d'oxygène, en bas au centre.
				const float O2 = FR->GetRiftO2(), Bw = 70 * U, Bx = SX * 0.5f - Bw * 0.5f, By = SY - 36 * U;
				const bool bLo = O2 < 0.25f, bF8 = bLo && FMath::Fmod(Time * 8.f, 2.f) < 1.f;
				const FLinearColor Oc = bLo ? (bF8 ? FLinearColor::White : Red) : Cyan;
				Txt(TEXT("OXYGENE"), SX * 0.5f, By - 11 * U, Oc, U / 4.f, true);
				Rect(Bx - U, By - U, Bw + 2 * U, 6 * U, Ink);
				Rect(Bx, By, Bw, 4 * U, FLinearColor(FColor(26, 24, 56)));
				Rect(Bx, By, Bw * O2, 4 * U, Oc);
				if (O2 <= 0.f) { Txt(TEXT("ASPHYXIE"), SX * 0.5f, By + 8 * U, bF8 ? FLinearColor::White : Red, U / 4.f, true); }
				else if (bOn) { Txt(TEXT("CONFINEMENT PRET"), SX * 0.5f, By + 8 * U, Grn, U / 4.f, true); }
				// Repère de bord d'écran : la console hors champ, flèche qui pulse et distance (px du proto).
				const bool bOff = !bConfVis || Cs.X < 24 * U || Cs.X > SX - 24 * U || Cs.Y < 30 * U || Cs.Y > SY - 30 * U;
				if (bOff)
				{
					const FVector2D Ctr(SX * 0.5f, SY * 0.5f);
					FVector2D Dv = Cs - Ctr;
					if (!bConfVis)
					{
						// Derrière la caméra : direction au sol de Jira vers la console.
						const FVector D3 = ConfW - (GetOwningPawn() ? GetOwningPawn()->GetActorLocation() : ConfW);
						Dv = FVector2D(D3.Y, -D3.X);
					}
					const FVector2D Ud = Dv.GetSafeNormal();
					const float Mx = SX * 0.5f - 44 * U, My = SY * 0.5f - 52 * U;
					const float K = FMath::Min(FMath::Abs(Ud.X) > 0.001f ? Mx / FMath::Abs(Ud.X) : 1e9f, FMath::Abs(Ud.Y) > 0.001f ? My / FMath::Abs(Ud.Y) : 1e9f);
					const float Pl = 0.5f + 0.5f * FMath::Sin(Time * (bOn ? 9.f : 5.f));
					const FVector2D Ep = Ctr + Ud * K + Ud * Pl * 4 * U;
					const FLinearColor Ac = bOn ? Cyan : Yel;
					const float Rr = (17.f + Pl * 6.f) * U;
					Rect(Ep.X - Rr * 0.7f, Ep.Y - Rr * 0.7f, Rr * 1.4f, Rr * 1.4f, FLinearColor(Ac.R, Ac.G, Ac.B, 0.12f + 0.15f * Pl));
					const FVector2D Nn(-Ud.Y, Ud.X);
					const FVector2D Tip = Ep + Ud * 17 * U, B1 = Ep - Ud * 13 * U;
					Canvas->K2_DrawLine(B1, Tip, 4 * U, Ac);
					Canvas->K2_DrawLine(Tip, Ep + Ud * 3 * U + Nn * 11 * U, 4 * U, Ac);
					Canvas->K2_DrawLine(Tip, Ep + Ud * 3 * U - Nn * 11 * U, 4 * U, Ac);
					const float Dpx = FVector2D::Distance(FR->GetRoom()->WorldToRoomPx(GetOwningPawn() ? GetOwningPawn()->GetActorLocation() : ConfW), Cc->ConfPx);
					Txt(FString::Printf(TEXT("%s%dM"), bOn ? TEXT("CONFINEMENT ") : TEXT("CONSOLE "), FMath::RoundToInt(Dpx)), Ep.X, Ep.Y + 19 * U, Ac, U / 4.f, true);
				}
			}
		}
	}

	// --- Cartographe : « ! » au-dessus de lui tant qu'il attend, sa barre de PV pendant la défense.
	if (const AEONFloorManager* FK = AEONFloorManager::Get(this))
	{
		FVector Kw; uint8 Ks = 0; int32 Kh = 0; FVector2D Ks2;
		if (FK->GetKidHud(Kw, Ks, Kh) && GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(Kw, Ks2))
		{
			const FLinearColor Kc(FColor(122, 255, 176));
			if (Ks == 0) { Txt(TEXT("!"), Ks2.X, Ks2.Y - 4 * U + FMath::Sin(Time * 4.f) * U, Kc, U / 2.5f, true); }
			if (Ks == 2)
			{
				Rect(Ks2.X - 9 * U, Ks2.Y, 18 * U, 4 * U, Ink);
				Rect(Ks2.X - 8 * U, Ks2.Y + U, 16 * U, 2 * U, FLinearColor(FColor(31, 90, 58)));
				Rect(Ks2.X - 8 * U, Ks2.Y + U, 16 * U * FMath::Clamp(Kh / 6.f, 0.f, 1.f), 2 * U, Kc);
			}
		}
	}

	// --- Gardien : carton-titre (nom lettre par lettre, sous-titre, 2,6 s). Sa barre de vie est dans UEONHudWidget (haut centre).
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
	}

	// --- Kibo, bannière centrale (la mini-carte, l'étage et la salle sont dans UEONHudWidget)
	if (const AEONFloorManager* F = AEONFloorManager::Get(this))
	{
		if (F->IsDockedMode())
		{
			const uint8 Kb = F->GetKiboRoom();
			// Salle de réveil : la consigne du proto et les greffes en main, avec leur touche (« 1 Standard 2 Rafale… »).
			if (Kb == 1 && !F->IsCinematic())
			{
				Txt(TEXT("Teste chaque greffe sur les cibles"), SX * 0.5f - 60 * U, SY - 40 * U, Dim, U / 3.5f);
				FString L;
				const UEONTuningSubsystem* Tu = UEONTuningSubsystem::Get(this);
				UDataTable* Gt = Tu ? Tu->GetGreffes() : nullptr;
				const TArray<FName> Set = J->GetGreffeSet();
				for (int32 I = 0; I < Set.Num(); ++I)
				{
					const FEONGreffeRow* Row = Gt ? Gt->FindRow<FEONGreffeRow>(Set[I], TEXT("HUD"), false) : nullptr;
					L += FString::Printf(TEXT("%s%d %s"), I ? TEXT("  ") : TEXT(""), I + 1, Row ? *Row->DisplayName : *Set[I].ToString());
				}
				Txt(L, SX * 0.5f - 60 * U, SY - 33 * U, Dim, U / 3.5f);
			}
		}
		// --- Bannière centrale (1,8 s)
		if (F->BannerT > 0.f && !(UEONUISubsystem::Get(this) && UEONUISubsystem::Get(this)->IsTitleOpen()))
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
		// JIR-V23 : Jira (rendu provisoire détouré, buste) glisse depuis la gauche avec le texte, débordant de la bande.
		if (!CutInTex) { CutInTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/EON/UI/T_Jira_CutIn.T_Jira_CutIn")); }
		if (CutInTex && Canvas)
		{
			const float Ih = 96.f * U, Iw = Ih * CutInTex->GetSizeX() / FMath::Max(1, CutInTex->GetSizeY());
			FCanvasTileItem It(FVector2D(Px - 30.f * U, Cy + 72.f * U - Ih), CutInTex->GetResource(), FVector2D(Iw, Ih), FLinearColor(1.f, 1.f, 1.f, A));
			It.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(It);
		}
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
		if (const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this); MS && MS->GetRunUnlocks().Num())
		{
			const FLinearColor Am(FColor(255, 194, 61));
			Txt(TEXT("DEBLOQUE PENDANT LA RUN"), SX * 0.5f, SY * 0.4f + 36 * U, Am, U / 3.5f, true);
			int32 K = 0;
			for (const FName& Id : MS->GetRunUnlocks())
			{
				for (const FEONUnlockDef& Ud : UEONMetaSubsystem::Unlocks())
				{
					if (Ud.Id == Id) { Txt(Ud.Kind + TEXT(" : ") + Ud.Name, SX * 0.5f, SY * 0.4f + (46 + 9 * K) * U, FLinearColor::White, U / 4.f, true); ++K; }
				}
			}
		}
	}
	else { DrawToast(); }
	// Dialogue : UEONHudWidget::PaintDialog (maquette du 25/11).
	DrawCom();
	DrawTitle();
	DrawDevMenu();
}

// ---------------------------------------------------------------- jalon 8

void AEONHUD::Box(float X, float Y, float W, float H, const FLinearColor& Border, const FLinearColor& Fill)
{
	if (Fill.A > 0.f) { Rect(X, Y, W, H, Fill); }
	Rect(X, Y, W, U * 0.5f, Border); Rect(X, Y + H - U * 0.5f, W, U * 0.5f, Border);
	Rect(X, Y, U * 0.5f, H, Border); Rect(X + W - U * 0.5f, Y, U * 0.5f, H, Border);
}

TArray<FString> AEONHUD::Wrap(const FString& S, int32 N)
{
	TArray<FString> Out, Words;
	S.ParseIntoArray(Words, TEXT(" "), true);
	FString Cur;
	for (const FString& W : Words)
	{
		if (Cur.Len() && Cur.Len() + 1 + W.Len() > N) { Out.Add(Cur); Cur.Reset(); }
		Cur += (Cur.Len() ? TEXT(" ") : TEXT("")) + W;
	}
	if (Cur.Len() || !Out.Num()) { Out.Add(Cur); }
	return Out;
}

int32 AEONHUD::Lines(const TArray<FString>& L, float X, float Y, const FLinearColor& C, int32 N)
{
	int32 K = 0;
	for (const FString& S : L) { for (const FString& Ln : Wrap(S, N)) { Txt(Ln, X, Y + K * 8 * U, C, U / 4.1f); ++K; } }
	return K;
}

void AEONHUD::DrawToast()
{
	// drawUnlToast : encart « DÉBLOQUÉ » en haut au centre, 3,2 s, fondu d'entrée et de sortie.
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	float Life = 0.f;
	const FEONUnlockDef* Ud = MS ? MS->GetToast(Life) : nullptr;
	if (!Ud) { return; }
	const float K = FMath::Clamp(FMath::Min(Life * 3.f, (3.2f - Life) * 6.f), 0.f, 1.f);
	const FLinearColor Am(1.f, 0.76f, 0.24f, K);
	const float W = 210 * U, X = Canvas->ClipX * 0.5f - W * 0.5f, Y = 80 * U;
	Box(X, Y, W, 26 * U, Am, FLinearColor(0.027f, 0.024f, 0.06f, 0.9f * K));
	Txt(TEXT("DEBLOQUE  -  ") + Ud->Kind, Canvas->ClipX * 0.5f, Y + 3 * U, Am, U / 4.f, true);
	Txt(Ud->Name, Canvas->ClipX * 0.5f, Y + 12 * U, FLinearColor(1.f, 1.f, 1.f, K), U / 3.f, true);
}

void AEONHUD::DrawCom()
{
	// drawCOMUI : cadre 424 × 236, titre, 5 onglets, liste à gauche (18 lignes), fiche à droite.
	const UEONUISubsystem* UI = UEONUISubsystem::Get(this);
	if (!UI || !UI->IsComOpen()) { return; }
	const AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	if (!J) { return; }
	const float SX = Canvas->ClipX, SY = Canvas->ClipY;
	Ox = SX * 0.5f - 240 * U; Oy = SY * 0.5f - 135 * U;
	auto PX = [this](float X) { return Ox + X * U; };
	auto PY = [this](float Y) { return Oy + Y * U; };
	const FLinearColor Wh(FColor(255, 255, 255)), Txc(FColor(207, 211, 255)), Gr(FColor(138, 240, 122)), Am(FColor(255, 194, 61)), Off(FColor(58, 52, 96)), Dk(FColor(90, 86, 136));
	Rect(0, 0, SX, SY, FLinearColor(0.027f, 0.024f, 0.06f, 0.8f));
	const float Bx = 28, By = 18, Bw = 424, Bh = 236;
	Box(PX(Bx), PY(By), Bw * U, Bh * U, Cyan, FLinearColor(FColor(11, 10, 28)));
	Txt(TEXT("C.O.M // TERMINAL"), PX(Bx + 10), PY(By + 4), Cyan, U / 3.6f);
	Txt(FString::Printf(TEXT("%s  -  Jira"), UI->IsDocked() ? TEXT("A quai") : *FString::Printf(TEXT("Etage %d"), F ? F->GetFloorN() : 0)), PX(Bx + Bw - 90), PY(By + 5), Dim, U / 4.1f);
	static const TCHAR* Tabs[5] = { TEXT("Ameliorations"), TEXT("Logs"), TEXT("Quetes"), TEXT("Descente"), TEXT("Cassettes") };
	const int32 Tab = UI->GetComTab();
	for (int32 I = 0; I < 5; ++I)
	{
		const bool bOn = I == Tab;
		Box(PX(Bx + 8 + I * 82), PY(By + 16), 78 * U, 11 * U, bOn ? Cyan : Off, bOn ? FLinearColor(FColor(29, 42, 68)) : FLinearColor(FColor(20, 17, 40)));
		Txt(FString::Printf(TEXT("%d  %s"), I + 1, Tabs[I]), PX(Bx + 8 + I * 82 + 39), PY(By + 17.5f), bOn ? Wh : Dim, U / 4.8f, true);
	}
	const float Rx = PX(Bx + 172), Ry = PY(By + 36);
	const int32 Rn = 96;
	auto Help = [&](const TCHAR* S) { Txt(S, SX * 0.5f, PY(By + Bh - 11), Dim, U / 5.f, true); };
	const int32 Sel0 = UI->GetComSel(Tab);

	if (Tab == 3)
	{
		// Le levier (drawCOMLever) : prêt quand le gardien est neutralisé.
		const bool bReady = F && F->IsFloorDone();
		const float Lx = PX(Bx + Bw / 2 - 24), Ly = PY(By + 50);
		Box(Lx, Ly, 48 * U, 110 * U, bReady ? Cyan : Off, FLinearColor(FColor(20, 17, 40)));
		Rect(Lx + 22 * U, Ly + 10 * U, 4 * U, 90 * U, Off);
		const float Hy = bReady ? Ly + 14 * U : Ly + 80 * U;
		Rect(Lx + 12 * U, Hy, 24 * U, 8 * U, bReady ? Cyan : Dk);
		Txt(bReady ? TEXT("LEVIER PRET") : TEXT("VERROUILLE"), SX * 0.5f, Ly + 116 * U, bReady ? Cyan : Red, U / 3.5f, true);
		if (!bReady && F)
		{
			Txt(FString::Printf(TEXT("Gardien : %s du C.O.M  -  carte rouge %s"), *F->GuardDir().ToUpper(), F->HasRedCard() ? TEXT("en main") : TEXT("a trouver")), SX * 0.5f, Ly + 128 * U, Txc, U / 4.1f, true);
		}
		Help(TEXT("Entree / (A) : tirer le levier     Q/D : onglet     C ou Echap / (B) : fermer"));
		return;
	}

	if (Tab == 4)
	{
		// drawCOMCass : niveaux de cassette, solde de Données, bouton Installer.
		const TArray<UEONUISubsystem::FCassRow>& Rows = UEONUISubsystem::CassRows();
		const int32 Sel = FMath::Min(Sel0, Rows.Num() - 1);
		Rect(PX(Bx + 164), Ry - 2 * U, U * 0.5f, (Bh - 52) * U, Off);
		for (int32 Jn = 0; Jn < Rows.Num(); ++Jn)
		{
			const UEONUISubsystem::FCassRow& Rw = Rows[Jn];
			FString T; int32 Cost = 0;
			const int32 St = UI->CassState(Rw, T, Cost);
			const float Yy = PY(By + 36 + Jn * 10);
			const bool bOn = Jn == Sel;
			if (bOn) { Rect(PX(Bx + 8), Yy, 152 * U, 9 * U, FLinearColor(FColor(34, 32, 74))); Rect(PX(Bx + 8), Yy, U * 0.5f, 9 * U, Cyan); }
			const FString Lbl = Rw.Lock.Len() ? Rw.Lock : FString::Printf(TEXT("Shield NV%d%s"), Rw.Lvl, Rw.Br.Len() ? *(TEXT(" ") + Rw.Br) : TEXT(""));
			const FLinearColor Cc = St == 0 ? Gr : St == 1 ? Wh : St == 2 ? Am : Dk;
			Txt((St == 0 ? TEXT("[X] ") : St == 1 ? TEXT("[+] ") : TEXT("[ ] ")) + Lbl, PX(Bx + 12), Yy + 0.5f * U, bOn ? Wh : Cc, U / 4.1f);
		}
		Txt(FString::Printf(TEXT("Donnees : %d"), MS ? MS->GetDonnees() : 0), PX(Bx + Bw - 70), Ry, FLinearColor(FColor(154, 216, 255)), U / 4.2f);
		const UEONUISubsystem::FCassRow& Rw = Rows[Sel];
		FString T; int32 Cost = 0;
		const int32 St = UI->CassState(Rw, T, Cost);
		if (Rw.Lock.Len())
		{
			Txt(Rw.Lock, Rx, Ry, Dk, U / 3.6f);
			Lines({ TEXT("Categorie prevue par le GDD Items (Survie, Mobilite, Cobra/Attaque)."), TEXT("A definir ensemble avant de la coder.") }, Rx, Ry + 14 * U, Dim, Rn);
		}
		else
		{
			const int32 L = MS ? MS->CasLvl(TEXT("shield")) : 0;
			const FString Br = MS ? MS->CasBr(TEXT("shield")) : FString();
			Txt(TEXT("Cassette Shield"), Rx, Ry, Cyan, U / 3.6f);
			for (int32 K = 1; K <= 3; ++K) { Rect(Rx + (K * 14 - 14) * U, Ry + 12 * U, 10 * U, 4 * U, L >= K ? Cyan : Off); }
			Txt(L ? FString::Printf(TEXT("NV%d%s"), L, Br.Len() ? *(TEXT(" ") + Br) : TEXT("")) : FString(TEXT("Pas encore trouvee")), Rx + 48 * U, Ry + 10.5f * U, L ? Wh : Dim, U / 4.1f);
			// Textes de dialogues.json (cassettes.shield).
			Lines({ TEXT("NV1 - Absorbe 1 coup. Revient a l'etage suivant."), TEXT("NV2 - Se recharge apres 10 s sans degats."),
				TEXT("NV3 A - Shield nettoyant : a l'impact, explose et nettoie la salle autour de Jira."), TEXT("NV3 B - Shield miroir : a l'impact, renvoie les tirs ennemis a l'expediteur.") }, Rx, Ry + 24 * U, Txc, Rn);
			if (Rw.Lvl == 3) { Txt(TEXT("Choix de branche definitif."), Rx, Ry + 92 * U, Am, U / 4.1f); }
		}
		const bool bCan = St == 1 && UI->IsDocked();
		const float Bbx = PX(Bx + Bw - 110), Bby = PY(By + Bh - 40);
		Box(Bbx, Bby, 100 * U, 14 * U, bCan ? Cyan : Off, bCan ? FLinearColor(FColor(29, 42, 68)) : FLinearColor(FColor(20, 17, 40)));
		const FString Bt = St == 0 ? FString(TEXT("INSTALLE")) : (!UI->IsDocked() && St == 1) ? FString(TEXT("AU KIBO (A QUAI)")) : St == 1 ? TEXT("INSTALLER  ") + T : T.ToUpper();
		Txt(Bt, Bbx + 50 * U, Bby + 3 * U, bCan ? Wh : Dim, U / 4.8f, true);
		Help(TEXT("Z/S : choisir     Entree / (A) : installer     Q/D : onglet     C ou Echap / (B) : fermer"));
		return;
	}

	// Onglets à liste : Améliorations (0), Logs (1), Quêtes (2).
	const TArray<TPair<FName, int32>> Items = Tab == 0 ? UI->ComItems() : TArray<TPair<FName, int32>>();
	const TArray<UEONUISubsystem::FQuest> Qs = Tab == 2 ? UI->Quests() : TArray<UEONUISubsystem::FQuest>();
	const int32 N = Tab == 0 ? Items.Num() : Tab == 1 ? UEONUISubsystem::NumLogs() : Qs.Num();
	const int32 Sel = FMath::Clamp(Sel0, 0, FMath::Max(0, N - 1));
	const int32 Vis = 18;
	const int32 OffI = FMath::Clamp(Sel - Vis + 1, 0, FMath::Max(0, N - Vis));
	const int32 LogN = MS && MS->M() ? FMath::Min(MS->M()->LogN, UEONUISubsystem::NumLogs()) : 0;
	Rect(PX(Bx + 164), Ry - 2 * U, U * 0.5f, (Bh - 52) * U, Off);
	for (int32 Jn = 0; Jn < FMath::Min(Vis, N); ++Jn)
	{
		const int32 I = Jn + OffI;
		const float Yy = PY(By + 36 + Jn * 10), X0 = PX(Bx + 8);
		const bool bOn = I == Sel;
		if (bOn) { Rect(X0, Yy, 152 * U, 9 * U, FLinearColor(FColor(34, 32, 74))); Rect(X0, Yy, U * 0.5f, 9 * U, Cyan); }
		if (Tab == 0)
		{
			const FEONItemDef* It = EONItems::Find(Items[I].Key);
			if (!It) { continue; }
			const FLinearColor Rc = EONItems::Color(It->Rar);
			Rect(X0 + 4 * U, Yy + 3 * U, 3 * U, 3 * U, Rc);
			Txt(It->Name.Left(30) + (Items[I].Value > 1 ? FString::Printf(TEXT(" x%d"), Items[I].Value) : TEXT("")), X0 + 10 * U, Yy + 0.5f * U, bOn ? Wh : Rc, U / 4.1f);
		}
		else if (Tab == 1)
		{
			const bool bGot = I < LogN;
			FString Title; UEONUISubsystem::Log(I).Split(TEXT("."), &Title, nullptr);
			Txt(FString::Printf(TEXT("%02d"), I + 1), X0 + 4 * U, Yy + 0.5f * U, bGot ? Cyan : Off, U / 4.1f);
			Txt(bGot ? Title.Left(31) : FString(TEXT("- - - - - - - -")), X0 + 16 * U, Yy + 0.5f * U, bGot ? (bOn ? Wh : Txc) : Off, U / 4.1f);
		}
		else
		{
			const UEONUISubsystem::FQuest& Q = Qs[I];
			Txt(Q.bOk ? TEXT("[X]") : TEXT("[ ]"), X0 + 4 * U, Yy + 0.5f * U, Q.bOk ? Gr : Dim, U / 4.1f);
			Txt(Q.T.Left(30), X0 + 20 * U, Yy + 0.5f * U, bOn ? Wh : FLinearColor(Q.C), U / 4.1f);
		}
	}
	if (Tab == 0)
	{
		const FEONGreffeRow* Gd = nullptr;
		if (const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this)) { if (UDataTable* G = T->GetGreffes()) { Gd = G->FindRow<FEONGreffeRow>(J->GetGreffeId(), TEXT("HUD"), false); } }
		const int32 Hb = MS && MS->M() ? MS->M()->HpBonus : 0;
		Txt(TEXT("Jira"), Rx, Ry, FLinearColor(FColor(184, 220, 88)), U / 3.6f);
		Lines({ TEXT("Greffe active : ") + (Gd ? Gd->DisplayName : J->GetGreffeId().ToString()),
			FString::Printf(TEXT("Niveau ADN : %d   (%d / %d)"), J->GetLevel(), FMath::FloorToInt(J->GetADN()), FMath::RoundToInt(J->GetADNNeed())),
			FString::Printf(TEXT("PV : %d / %d"), J->GetHP(), J->GetMaxHP()) + (Hb ? FString::Printf(TEXT("   dont +%d permanents (insignes)"), Hb) : FString()),
			FString::Printf(TEXT("Dash : %d charge%s   Scrap : %d"), J->GetDashMax(), J->GetDashMax() > 1 ? TEXT("s") : TEXT(""), J->GetScrap()),
			FString::Printf(TEXT("Ameliorations prises : %d"), J->GetItems().Num()) }, Rx, Ry + 12 * U, Txc, Rn);
		Rect(Rx, Ry + 56 * U, PX(Bx + Bw - 10) - Rx, U * 0.5f, Off);
		if (!Items.IsValidIndex(Sel)) { Lines({ TEXT("Aucune amelioration pour l'instant."), TEXT("Les salles Opportunite, le marchand et les niveaux d'ADN en proposent.") }, Rx, Ry + 64 * U, Dim, Rn); }
		else if (const FEONItemDef* It = EONItems::Find(Items[Sel].Key))
		{
			const FLinearColor Rc = EONItems::Color(It->Rar);
			Rect(Rx + 4 * U, Ry + 66 * U, 6 * U, 6 * U, Rc);
			Txt(It->Name + (Items[Sel].Value > 1 ? FString::Printf(TEXT("  x%d"), Items[Sel].Value) : TEXT("")), Rx + 16 * U, Ry + 64 * U, Rc, U / 3.8f);
			Txt(EONItems::RarName(It->Rar) + TEXT("  -  ") + It->Cat.ToString(), Rx + 16 * U, Ry + 73 * U, Dim, U / 4.1f);
			Lines({ It->Desc }, Rx, Ry + 86 * U, Txc, Rn);
		}
	}
	else if (Tab == 1)
	{
		Txt(FString::Printf(TEXT("Logs retrouves : %d / %d"), LogN, UEONUISubsystem::NumLogs()), PX(Bx + Bw - 80), Ry, Dim, U / 4.1f);
		if (Sel < LogN) { Txt(FString::Printf(TEXT("LOG %02d"), Sel + 1), Rx, Ry, Cyan, U / 3.6f); Lines({ UEONUISubsystem::Log(Sel) }, Rx, Ry + 14 * U, FLinearColor(FColor(232, 228, 255)), Rn); }
		else { Lines({ TEXT("Signal non capte."), TEXT("Ce log est quelque part dans les etages. Les logs se trouvent dans les salles calmes et au sol, apres les combats.") }, Rx, Ry + 14 * U, Dk, Rn); }
	}
	else if (Qs.IsValidIndex(Sel))
	{
		const UEONUISubsystem::FQuest& Q = Qs[Sel];
		Txt(Q.T, Rx, Ry, FLinearColor(Q.C), U / 3.6f);
		Txt(Q.bOk ? TEXT("Terminee") : TEXT("En cours"), PX(Bx + Bw - 50), Ry, Q.bOk ? Gr : Am, U / 4.1f);
		Lines(Q.L, Rx, Ry + 14 * U, Txc, Rn);
	}
	Help(TEXT("Q/D : onglet     Z/S : choisir     C ou Echap / (B) : fermer"));
}

void AEONHUD::DrawTitle()
{
	// drawTitle : E.O.N, la progression gardée, Continuer / Nouvelle partie (le second appui efface).
	const UEONUISubsystem* UI = UEONUISubsystem::Get(this);
	if (!UI || !UI->IsTitleOpen()) { return; }
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	const UEONSaveGame* M = MS ? MS->M() : nullptr;
	const float SX = Canvas->ClipX, SY = Canvas->ClipY, Cx = SX * 0.5f, Cy = SY * 0.5f;
	Rect(0, 0, SX, SY, FLinearColor(0.027f, 0.024f, 0.06f, 0.86f));
	Txt(TEXT("E.O.N"), Cx, Cy - 64 * U, Cyan, U / 1.3f, true);
	FString Info = TEXT("Partie en cours sur ce PC");
	if (M)
	{
		if (M->bUlysse) { Info += TEXT(" - Ulysse au Kibo"); }
		if (M->bCobUnlock) { Info += TEXT(" - Cobaye"); }
		if (M->Crew.Num()) { Info += FString::Printf(TEXT(" - insignes %d/6"), M->Crew.Num()); }
		if (M->Donnees) { Info += FString::Printf(TEXT(" - donnees %d"), M->Donnees); }
		Info += FString::Printf(TEXT(" - %d run%s, %d deblocage%s"), M->Runs, M->Runs > 1 ? TEXT("s") : TEXT(""), M->Unl.Num(), M->Unl.Num() > 1 ? TEXT("s") : TEXT(""));
	}
	Txt(Info, Cx, Cy - 20 * U, FLinearColor(FColor(207, 211, 255)), U / 3.8f, true);
	for (int32 I = 0; I < 2; ++I)
	{
		const bool bOn = UI->GetTitleSel() == I, bWarn = I == 1 && UI->IsTitleConfirm();
		const float Bx = I == 0 ? Cx - 130 * U : Cx + 10 * U, By = Cy + 6 * U;
		Box(Bx, By, 120 * U, 22 * U, bWarn ? Red : bOn ? Cyan : FLinearColor(FColor(58, 52, 96)), bOn ? FLinearColor(FColor(29, 42, 68)) : FLinearColor(FColor(20, 17, 40)));
		Txt(I == 0 ? TEXT("Continuer") : bWarn ? TEXT("Confirmer : tout effacer") : TEXT("Nouvelle partie"), Bx + 60 * U, By + 8 * U, bWarn ? Red : bOn ? FLinearColor::White : Dim, U / 3.6f, true);
	}
	Txt(UI->IsTitleConfirm() ? TEXT("La progression sera effacee : Ulysse, Cobaye, insignes, logs, cassettes, donnees.") : TEXT("Fleches ou Q/D : choisir     Entree / (A) : valider"),
		Cx, Cy + 42 * U, UI->IsTitleConfirm() ? FLinearColor(FColor(255, 194, 61)) : Dim, U / 4.2f, true);
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
		if (!L.Hint.IsEmpty()) { Txt(TEXT("studio ") + L.Hint, X0 + W - 20 * U, Y + 1.4f * U, FLinearColor(FColor(150, 146, 185)), U / 4.1f); }
		Y += Lh;
	}
	Txt(Footer, X0 + 4 * U, Y + Lh * 0.6f, Footer.StartsWith(TEXT("Fl")) ? FLinearColor(FColor(170, 166, 205)) : Amber, U / 4.4f);
}

void AEONHUD::EnsureHudWidget()
{
	// Widget UMG du HUD (charte du studio) : créé une fois, au-dessus du Canvas.
	if (HudWidget || !GetOwningPlayerController()) { return; }
	HudWidget = CreateWidget<UEONHudWidget>(GetOwningPlayerController(), UEONHudWidget::StaticClass());
	if (HudWidget) { HudWidget->SetVisibility(ESlateVisibility::HitTestInvisible); HudWidget->AddToViewport(5); }
}
