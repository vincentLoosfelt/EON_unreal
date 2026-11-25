#include "EONHudWidget.h"
#include "EONBoss.h"
#include "EONCharacter.h"
#include "EONEnemy.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONPedestal.h"
#include "EONPlayerController.h"
#include "EONTileRoom.h"
#include "EONTuningSubsystem.h"
#include "EONTileGrid.h"
#include "EONTypes.h"
#include "EONUI.h"
#include "Engine/DataTable.h"
#include "EngineUtils.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

namespace EONHud
{
	// Palette de la charte (page 03) : ambre de titrage, fond, rouge ; secondaires ≤ 15 % de la composition.
	const FLinearColor Amber = FLinearColor(FColor(0xE9, 0xA1, 0x00));
	const FLinearColor Bg = FLinearColor(FColor(0x01, 0x06, 0x0C));
	const FLinearColor RedC = FLinearColor(FColor(0xEF, 0x14, 0x00));
	const FLinearColor Orange = FLinearColor(FColor(0xF4, 0x6C, 0x51));
	const FLinearColor CyanC = FLinearColor(FColor(0x55, 0xF6, 0xF6));
	const FLinearColor Green = FLinearColor(FColor(0x61, 0xD6, 0x4C));
	const FLinearColor Grey = FLinearColor(FColor(0x30, 0x3A, 0x3D));
	const FLinearColor White = FLinearColor::White;

	FLinearColor A(FLinearColor C, float Alpha) { C.A *= Alpha; return C; }
	/** Majuscules, lettres accentuées comprises (FString::ToUpper laisse « é » en minuscule). */
	FString Upper(const FString& S)
	{
		FString R = S.ToUpper();
		static const TCHAR* Lo = TEXT("àâäçéèêëîïôöùûüÿœæ");
		static const TCHAR* Up = TEXT("ÀÂÄÇÉÈÊËÎÏÔÖÙÛÜŸŒÆ");
		for (TCHAR& C : R.GetCharArray()) { for (int32 I = 0; Lo[I]; ++I) { if (C == Lo[I]) { C = Up[I]; break; } } }
		return R;
	}

	/** Polices de la charte, lues depuis leurs fichiers (Content/EON/UI/FontsTTF, OFL) : Kode Mono SemiBold pour le HUD
	 *  et les chiffres, Oxanium pour les textes. */
	const TSharedPtr<const FCompositeFont>& Face(int32 I)
	{
		static TSharedPtr<const FCompositeFont> F[3];
		static const TCHAR* Files[3] = { TEXT("KodeMono-SemiBold.ttf"), TEXT("Oxanium-Medium.ttf"), TEXT("Oxanium-Regular.ttf") };
		if (!F[I].IsValid())
		{
			const FString Path = FPaths::ProjectContentDir() / TEXT("EON/UI/FontsTTF") / Files[I];
			F[I] = MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}
		return F[I];
	}

	FString RoleName(FName Role)
	{
		static const TMap<FName, FString> N = {
			{ TEXT("DEPART"), TEXT("C.O.M") }, { TEXT("COMBAT"), TEXT("Salle de combat") }, { TEXT("TENSION"), TEXT("Salle sous tension") },
			{ TEXT("CALME"), TEXT("Salle calme") }, { TEXT("OPPORTUNITE"), TEXT("Opportunité") }, { TEXT("ASCENSEUR"), TEXT("Gardien") },
			{ TEXT("SECURITE"), TEXT("Sécurité") }, { TEXT("EVENEMENT"), TEXT("Événement") }, { TEXT("MARCHAND"), TEXT("Marchand") },
			{ TEXT("CORRIDOR"), TEXT("Le corridor") }, { TEXT("FAILLE"), TEXT("La faille") }, { TEXT("TRESOR"), TEXT("Trésor") },
			{ TEXT("IRRUPTION"), TEXT("Irruption") }, { TEXT("CARTO"), TEXT("Cartographe") } };
		const FString* S = N.Find(Role);
		return S ? *S : Role.ToString();
	}

	/** Peintre : coordonnées dans un écran de référence 1920 × 1080 (K = hauteur réelle / 1080). */
	struct FPaint
	{
		const FGeometry& G;
		FSlateWindowElementList& L;
		int32 Layer;
		float K;
		FVector2f Size;
		FSlateResourceHandle White;

		FPaint(const FGeometry& InG, FSlateWindowElementList& InL, int32 InLayer)
			: G(InG), L(InL), Layer(InLayer)
		{
			Size = FVector2f(G.GetLocalSize());
			K = Size.Y / 1080.f;
			White = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")));
		}
		float W() const { return Size.X / K; }   // largeur de l'écran de référence (1920 en 16/9)

		/** Polygone convexe plein (éventail de triangles). */
		void Poly(std::initializer_list<FVector2f> P, const FLinearColor& C, int32 Dl = 0) const
		{
			if (C.A <= 0.f || P.size() < 3) { return; }
			TArray<FSlateVertex> V; TArray<SlateIndex> I;
			const FSlateRenderTransform& T = G.GetAccumulatedRenderTransform();
			const FColor Col = C.ToFColor(true);
			for (const FVector2f& Pt : P) { V.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, Pt * K, FVector2f(0.5f, 0.5f), Col)); }
			for (int32 J = 1; J + 1 < V.Num(); ++J) { I.Add(0); I.Add(J); I.Add(J + 1); }
			FSlateDrawElement::MakeCustomVerts(L, Layer + Dl, White, V, I, nullptr, 0, 0);
		}
		void Rect(float X, float Y, float Wd, float H, const FLinearColor& C, int32 Dl = 0) const
		{
			Poly({ { X, Y }, { X + Wd, Y }, { X + Wd, Y + H }, { X, Y + H } }, C, Dl);
		}
		/** Parallélogramme penché à 45° (segments des barres). */
		void Slant(float X, float Y, float Wd, float H, const FLinearColor& C, int32 Dl = 0) const
		{
			Poly({ { X + H, Y }, { X + Wd, Y }, { X + Wd - H, Y + H }, { X, Y + H } }, C, Dl);
		}
		void Line(std::initializer_list<FVector2f> P, const FLinearColor& C, float Th = 2.f, int32 Dl = 1) const
		{
			if (C.A <= 0.f) { return; }
			TArray<FVector2f> Pts;
			for (const FVector2f& Pt : P) { Pts.Add(Pt * K); }
			FSlateDrawElement::MakeLines(L, Layer + Dl, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, C, true, Th * K);
		}
		FSlateFontInfo Font(int32 F, float Px) const { return FSlateFontInfo(Face(F), FMath::Max(1.f, Px * K * 0.75f)); }
		FVector2f Measure(const FString& S, int32 F, float Px) const
		{
			return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(S, Font(F, Px))) / K;
		}
		/** Texte ; Align 0 gauche, 1 centre, 2 droite. Y = haut de la ligne. */
		void Text(const FString& S, float X, float Y, int32 F, float Px, const FLinearColor& C, int32 Align = 0, int32 Dl = 2) const
		{
			if (S.IsEmpty() || C.A <= 0.f) { return; }
			const FVector2f Sz = Measure(S, F, Px);
			const float X0 = Align == 1 ? X - Sz.X * 0.5f : Align == 2 ? X - Sz.X : X;
			FSlateDrawElement::MakeText(L, Layer + Dl, G.ToPaintGeometry(FVector2f(Sz.X + 4.f, Sz.Y) * K, FSlateLayoutTransform(FVector2f(X0, Y) * K)),
				S, Font(F, Px), ESlateDrawEffect::None, C);
		}
		/** Halo de la charte (Glow) : contours élargis et transparents. */
		void Glow(float X, float Y, float Wd, float H, const FLinearColor& C) const
		{
			for (int32 I = 1; I <= 3; ++I)
			{
				const float E = I * 2.f;
				Line({ { X - E, Y - E }, { X + Wd + E, Y - E }, { X + Wd + E, Y + H + E }, { X - E, Y + H + E }, { X - E, Y - E } }, A(C, 0.12f / I), 2.f, 0);
			}
		}
	};

	const AEONCharacter* Jira(const UObject* Ctx) { return Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(Ctx, 0)); }
	bool Blink(float Period, float On) { return FMath::Fmod(FPlatformTime::Seconds(), Period) < On; }
}

using namespace EONHud;

void UEONHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Clock += InDeltaTime;
	const AEONCharacter* J = Jira(this);
	if (!J) { return; }

	// Annonces : un objet de plus dans l'inventaire de la run -> « +1 NOUVELLE <catégorie> », 3 s.
	const TArray<FName>& It = J->GetItems();
	if (SeenItems < 0 || It.Num() < SeenItems) { SeenItems = It.Num(); }
	for (; SeenItems < It.Num(); ++SeenItems)
	{
		const FEONItemDef* D = EONItems::Find(It[SeenItems]);
		if (!D) { continue; }
		FToast T;
		if (D->Cat == TEXT("GREFFE")) { T.Txt = TEXT("NOUVELLE GREFFE"); T.Col = Green; }
		else if (D->Cat == TEXT("CASSETTE")) { T.Txt = TEXT("NOUVELLE CASSETTE"); T.Col = Orange; }
		else if (D->Cat == TEXT("DISQUETTE")) { T.Txt = TEXT("NOUVELLE DISQUETTE"); T.Col = CyanC; }
		else if (D->Cat == TEXT("PERK")) { T.Txt = TEXT("NOUVEAU PERK"); T.Col = Amber; }
		else { T.Txt = TEXT("NOUVEL ÉCHO"); T.Col = White; }
		Toasts.Add(T);
	}
	for (FToast& T : Toasts) { T.T += InDeltaTime; }
	Toasts.RemoveAll([](const FToast& T) { return T.T > 3.4f; });
	while (Toasts.Num() > 5) { Toasts.RemoveAt(0); }

	// Barre du monstre : le dernier ennemi touché (flash de coup), 4 s ; à sa mort, encore 0,8 s.
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AEONEnemy> E(W); E; ++E)
		{
			if (E->Flash > 0.f && !E->IsDead()) { Foe = *E; FoeT = 4.f; }
		}
	}
	if (Foe.IsValid() && Foe->IsDead()) { FoeT = FMath::Min(FoeT, 0.8f); }
	FoeT -= InDeltaTime;
	if (FoeT <= 0.f) { Foe.Reset(); }
}

int32 UEONHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const AEONCharacter* J = Jira(this);
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	const UEONUISubsystem* UI = UEONUISubsystem::Get(this);
	const APlayerController* OPC = GetOwningPlayer();
	const AHUD* Hud = OPC ? OPC->GetHUD() : nullptr;
	const bool bTitle = UI && UI->IsTitleOpen();
	const FEONDlgLine* Dl = UI && !bTitle ? UI->DlgLine() : nullptr;
	const bool bHud = J && !(Hud && !Hud->bShowHUD) && !bTitle && !(UI && UI->IsComOpen()) && !(F && F->IsCinematic());   // terminal du C.O.M ouvert : pas de HUD (Vincent, 25/11)
	const bool bCom = J && UI && UI->IsComOpen() && !bTitle;
	const bool bNotice = J && !bTitle && !bCom && !(Hud && !Hud->bShowHUD);   // bannières et invites, même en cinématique
	if (!bHud && !Dl && !bCom && !bNotice) { return LayerId; }
	const FPaint P(AllottedGeometry, OutDrawElements, LayerId + 1);
	if (bHud) { PaintHud(P, J, F, Dl != nullptr); }
	if (bCom) { PaintCom(P, *UI, *J, F); }
	if (bNotice)
	{
		const AEONPlayerController* EPC = Cast<AEONPlayerController>(OPC);
		PaintNotices(P, *J, F, EPC && EPC->IsUsingGamepad());
	}
	if (Dl) { PaintDialog(P, *Dl, UI->DlgShown()); }
	return LayerId + 12;
}

void UEONHudWidget::PaintHud(const EONHud::FPaint& P, const AEONCharacter* J, const AEONFloorManager* F, bool bDialog) const
{
	const FGeometry& AllottedGeometry = P.G;
	FSlateWindowElementList& OutDrawElements = P.L;
	const float SW = P.W(), SH = 1080.f, M = 60.f;   // marges de la maquette

	// ---------------------------------------------------------------- haut gauche : annonces « +1 »
	for (int32 I = 0; I < Toasts.Num(); ++I)
	{
		const FToast& T = Toasts[I];
		const float In = FMath::Clamp(T.T / 0.18f, 0.f, 1.f), Out = FMath::Clamp((3.4f - T.T) / 0.4f, 0.f, 1.f);
		const float Al = FMath::Min(In, Out);
		const float X = M - (1.f - In) * 40.f, Y = 40.f + I * 50.f, H = 34.f;
		const float Tw = P.Measure(T.Txt, 0, 19.f).X;
		const float Wd = 34.f + 14.f + Tw + 18.f;
		P.Rect(X, Y, Wd, H, A(Bg, 0.85f * Al));
		P.Rect(X, Y, 34.f, H, A(T.Col, 0.22f * Al));
		P.Line({ { X, Y }, { X + Wd, Y }, { X + Wd, Y + H }, { X, Y + H }, { X, Y } }, A(T.Col, Al));
		P.Line({ { X + 34.f, Y }, { X + 34.f, Y + H } }, A(T.Col, Al));
		P.Text(TEXT("+1"), X + 17.f, Y + 6.f, 0, 17.f, A(T.Col, Al), 1);
		P.Text(T.Txt, X + 48.f, Y + 5.f, 0, 19.f, A(T.Col, Al));
	}

	// ---------------------------------------------------------------- haut centre : barre du monstre
	{
		FString Name; float R = -1.f; bool bGrey = false;
		if (const AEONBoss* B = AEONBoss::GetActive(this); B && (!B->IsInIntro() || B->GetTitleT() > 0.f))
		{
			Name = B->GetBossName(); R = B->Hp / FMath::Max(1.f, B->MaxHp); bGrey = B->IsInvulnerableNow();
		}
		else if (Foe.IsValid())
		{
			const AEONEnemy* E = Foe.Get();
			Name = E->DisplayName.IsEmpty() ? E->Type.ToString() : E->DisplayName;
			R = E->IsDead() ? 0.f : E->Hp / FMath::Max(1.f, E->MaxHp);
		}
		if (R >= 0.f)
		{
			R = FMath::Clamp(R, 0.f, 1.f);
			const float Bw = 330.f, Bh = 30.f, Cw = 64.f, X0 = SW * 0.5f - (Cw + Bw) * 0.5f, Y = 70.f;
			const FLinearColor C = bGrey ? Grey : Amber;
			// Cartouche du crâne (coins coupés à 45°) et crâne en formes simples.
			P.Poly({ { X0 + 10.f, Y - 4.f }, { X0 + Cw, Y - 4.f }, { X0 + Cw - 2.f, Y + Bh + 4.f }, { X0, Y + Bh + 4.f }, { X0, Y + 6.f } }, A(Bg, 0.9f));
			P.Line({ { X0 + 10.f, Y - 4.f }, { X0 + Cw, Y - 4.f }, { X0 + Cw - 2.f, Y + Bh + 4.f }, { X0, Y + Bh + 4.f }, { X0, Y + 6.f }, { X0 + 10.f, Y - 4.f } }, C);
			{
				const FVector2f Ck(X0 + Cw * 0.5f, Y + Bh * 0.5f);
				P.Poly({ { Ck.X - 10.f, Ck.Y - 3.f }, { Ck.X - 6.f, Ck.Y - 10.f }, { Ck.X + 6.f, Ck.Y - 10.f }, { Ck.X + 10.f, Ck.Y - 3.f }, { Ck.X + 7.f, Ck.Y + 5.f }, { Ck.X - 7.f, Ck.Y + 5.f } }, C);
				P.Rect(Ck.X - 5.f, Ck.Y + 5.f, 10.f, 5.f, C);
				P.Rect(Ck.X - 6.f, Ck.Y - 4.f, 4.f, 4.f, Bg, 1);
				P.Rect(Ck.X + 2.f, Ck.Y - 4.f, 4.f, 4.f, Bg, 1);
			}
			// Cadre de la barre : bord droit à 45°, créneaux au-dessus (CHARTE_BETA p. 4).
			const float Bx = X0 + Cw + 6.f;
			P.Poly({ { Bx, Y }, { Bx + Bw, Y }, { Bx + Bw - Bh, Y + Bh }, { Bx, Y + Bh } }, A(Bg, 0.75f));
			P.Line({ { Bx, Y }, { Bx + Bw, Y }, { Bx + Bw - Bh, Y + Bh }, { Bx, Y + Bh }, { Bx, Y } }, C);
			for (const FVector2f Cr : { FVector2f(150.f, 30.f), FVector2f(186.f, 10.f), FVector2f(202.f, 10.f), FVector2f(236.f, 40.f) })
			{
				P.Rect(Bx + Cr.X, Y - 7.f, Cr.Y, 5.f, C);
			}
			// Segments : 10, pleins selon les PV.
			const int32 N = 10;
			const float Sx0 = Bx + 6.f, Sw = (Bw - Bh - 12.f + 18.f) / N, Sh = Bh - 10.f;
			for (int32 I = 0; I < N; ++I)
			{
				const float K01 = FMath::Clamp(R * N - I, 0.f, 1.f);
				const float X = Sx0 + I * Sw;
				P.Slant(X, Y + 5.f, Sw - 4.f, Sh, A(C, 0.18f));
				if (K01 > 0.f) { P.Slant(X, Y + 5.f, FMath::Max(Sh, (Sw - 4.f) * K01), Sh, C, 1); }
			}
			P.Text(Name, Bx + 4.f, Y - 32.f, 0, 22.f, C);
			P.Text(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(R * 100.f)), Bx + Bw + 6.f, Y + 1.f, 0, 24.f, C);
		}
	}

	// ---------------------------------------------------------------- haut droite : mini-carte
	{
		const float Mw = 256.f, Mh = 176.f, X0 = SW - M - Mw, Y0 = 40.f;
		P.Rect(X0, Y0, Mw, Mh, A(Bg, 0.88f));
		// Grille fine (charte page 04).
		for (float Gx = X0 + 16.f; Gx < X0 + Mw; Gx += 16.f) { P.Line({ { Gx, Y0 }, { Gx, Y0 + Mh } }, A(Amber, 0.07f), 1.f, 0); }
		for (float Gy = Y0 + 16.f; Gy < Y0 + Mh; Gy += 16.f) { P.Line({ { X0, Gy }, { X0 + Mw, Gy } }, A(Amber, 0.07f), 1.f, 0); }

		FString L1, L2;
		if (F)
		{
			if (F->IsDockedMode())
			{
				const uint8 Kb = F->GetKiboRoom();
				L1 = TEXT("Kibo");
				L2 = Kb == 1 ? TEXT("Salle de réveil") : Kb == 2 ? TEXT("Passerelle") : TEXT("C.O.M à quai");
			}
			else
			{
				L1 = FString::Printf(TEXT("Étage %02d"), F->GetFloorN());
				L2 = RoleName(F->GetCurrentRole());
			}
			// Salles vues : aplat ambre translucide et contour, couloirs étroits entre elles ; centrée sur la salle courante.
			const float CW = 34.f, CH = 24.f, Gp = 12.f;
			const FIntPoint Cur = F->GetCurrentCell();
			const FVector2f Ctr(X0 + Mw * 0.5f, Y0 + Mh * 0.5f - 8.f);
			auto CellPos = [&](const FIntPoint& Pp) { return Ctr + FVector2f((Pp.X - Cur.X) * (CW + Gp) - CW * 0.5f, (Pp.Y - Cur.Y) * (CH + Gp) - CH * 0.5f); };
			FGeometry Clip = AllottedGeometry.MakeChild(FVector2f(Mw - 4.f, Mh - 4.f) * P.K, FSlateLayoutTransform(FVector2f(X0 + 2.f, Y0 + 2.f) * P.K));
			OutDrawElements.PushClip(FSlateClippingZone(Clip));
			if (!F->IsDockedMode())
			{
				for (const auto& KV : F->GetCells())
				{
					const FEONFloorCell& C = KV.Value;
					if (!C.bSeen && !F->IsMapped()) { continue; }
					const FVector2f Pp = CellPos(C.Pos);
					const bool bCur = C.Pos == Cur;
					// Couloirs vers l'est et le sud (traits étroits).
					if (C.Doors & EONDoor::E) { P.Rect(Pp.X + CW, Pp.Y + CH * 0.5f - 4.f, Gp, 8.f, A(Amber, 0.55f)); }
					if (C.Doors & EONDoor::S) { P.Rect(Pp.X + CW * 0.5f - 4.f, Pp.Y + CH, 8.f, Gp, A(Amber, 0.55f)); }
					const float Fill = bCur ? 0.45f : C.bVisited ? 0.25f : 0.08f;
					P.Rect(Pp.X, Pp.Y, CW, CH, A(Amber, Fill));
					P.Line({ { Pp.X, Pp.Y }, { Pp.X + CW, Pp.Y }, { Pp.X + CW, Pp.Y + CH }, { Pp.X, Pp.Y + CH }, { Pp.X, Pp.Y } }, A(Amber, C.bVisited || bCur ? 1.f : 0.45f), 1.5f);
					// Repères : gardien rouge (cyan avec la carte), carte rouge orange, Opportunité blanc, Événement cyan, combat entamé orange.
					FLinearColor Dot(0, 0, 0, 0);
					if (C.Role == TEXT("ASCENSEUR")) { Dot = F->HasRedCard() ? CyanC : RedC; }
					else if (C.Role == TEXT("SECURITE") && !F->HasRedCard()) { Dot = Orange; }
					else if (C.Role == TEXT("OPPORTUNITE")) { Dot = White; }
					else if (C.Role == TEXT("EVENEMENT")) { Dot = CyanC; }
					else if (C.bVisited && !C.bCleared) { Dot = Orange; }
					if (Dot.A > 0.f && !bCur) { P.Rect(Pp.X + CW * 0.5f - 3.f, Pp.Y + CH * 0.5f - 3.f, 6.f, 6.f, Dot, 1); }
				}
			}
			// Jira : flèche blanche, orientée comme à l'écran.
			{
				float Ang = -PI * 0.5f;
				if (APlayerController* PC = GetOwningPlayer())
				{
					FVector2D S0, S1;
					const FVector Lc = J->GetActorLocation();
					if (PC->ProjectWorldLocationToScreen(Lc, S0) && PC->ProjectWorldLocationToScreen(Lc + J->GetActorForwardVector() * 100.f, S1) && !(S1 - S0).IsNearlyZero())
					{
						Ang = FMath::Atan2(S1.Y - S0.Y, S1.X - S0.X);
					}
				}
				const FVector2f D(FMath::Cos(Ang), FMath::Sin(Ang)), Nn(-D.Y, D.X);
				const FVector2f Tip = Ctr + D * 10.f, Bl = Ctr - D * 7.f + Nn * 8.f, Br = Ctr - D * 7.f - Nn * 8.f, Nt = Ctr - D * 3.f;
				P.Poly({ Tip, Bl, Nt }, White, 3);
				P.Poly({ Tip, Nt, Br }, White, 3);
			}
			OutDrawElements.PopClip();
		}
		// Cadre : bord ambre, onglet en haut, coin coupé plein en bas à gauche (maquette « 09 CARTE »).
		P.Line({ { X0, Y0 }, { X0 + Mw, Y0 }, { X0 + Mw, Y0 + Mh }, { X0, Y0 + Mh }, { X0, Y0 } }, Amber, 2.f, 3);
		P.Poly({ { X0 + Mw * 0.5f - 56.f, Y0 }, { X0 + Mw * 0.5f + 56.f, Y0 }, { X0 + Mw * 0.5f + 46.f, Y0 + 9.f }, { X0 + Mw * 0.5f - 46.f, Y0 + 9.f } }, Amber, 3);
		P.Rect(X0 + Mw * 0.5f - 34.f, Y0 + 3.f, 22.f, 2.f, Bg, 4);
		P.Rect(X0 + Mw * 0.5f + 12.f, Y0 + 3.f, 22.f, 2.f, Bg, 4);
		P.Poly({ { X0, Y0 + Mh - 36.f }, { X0 + 36.f, Y0 + Mh }, { X0, Y0 + Mh } }, Amber, 3);
		P.Text(L1, X0 + Mw - 8.f, Y0 + Mh - 42.f, 1, 19.f, White, 2, 4);
		P.Text(L2, X0 + Mw - 8.f, Y0 + Mh - 21.f, 2, 13.f, A(White, 0.85f), 2, 4);
	}

	// Pendant un dialogue, la boîte occupe le bas de l'écran : on s'arrête aux éléments du haut.
	if (bDialog) { return; }

	// ---------------------------------------------------------------- bas gauche : portrait, vie, énergie, greffes
	{
		const int32 Mut = J->GetMut();
		const float X0 = M, Pw = 48.f, Bx = X0 + Pw + 8.f, Bw = 290.f, Bh = 30.f, By = SH - 118.f;
		// Portrait provisoire : carré ambre (rouge en mutation) et initiale ; le vrai portrait est à produire.
		const FLinearColor Pc = Mut > 0 ? RedC : Amber;
		P.Rect(X0, By - 9.f, Pw, Pw, Pc);
		P.Text(TEXT("J"), X0 + Pw * 0.5f, By - 7.f, 0, 38.f, Bg, 1);
		// Vie : cadre au coin coupé, un segment par PV (+ bouclier cyan), chiffres en Kode Mono.
		const int32 MaxHp = FMath::Max(1, J->GetMaxHP()), Sh = J->GetShieldMax(), N = MaxHp + Sh;
		P.Poly({ { Bx, By }, { Bx + Bw, By }, { Bx + Bw, By + Bh - 10.f }, { Bx + Bw - 10.f, By + Bh }, { Bx, By + Bh } }, A(Bg, 0.75f));
		P.Line({ { Bx, By }, { Bx + Bw, By }, { Bx + Bw, By + Bh - 10.f }, { Bx + Bw - 10.f, By + Bh }, { Bx, By + Bh }, { Bx, By } }, Amber);
		{
			const float Sx0 = Bx + 5.f, Sy = By + 5.f, Sh2 = Bh - 10.f, Sw = (Bw - 10.f - Sh2 * 0.5f) / N;
			for (int32 I = 0; I < N; ++I)
			{
				const bool bShield = I >= MaxHp;
				const bool bOn = bShield ? (I - MaxHp) < J->GetShield() : I < J->GetHP();
				const FLinearColor C = bShield ? CyanC : (J->GetHP() <= 1 && Blink(0.5f, 0.25f) ? RedC : Amber);
				P.Slant(Sx0 + I * Sw, Sy, Sw - 3.f, Sh2, bOn ? C : A(C, 0.15f), 1);
			}
		}
		P.Text(FString::Printf(TEXT("%d/%d"), J->GetHP(), MaxHp), Bx + Bw + 10.f, By + 1.f, 0, 24.f, White);
		// Énergie : trait cyan sous la vie (blanc qui clignote sous 25 %) ; part perdue à cause de l'armure en orange ;
		// en mutation : rouge qui clignote de plus en plus vite et « MUTATION I / II / III ».
		{
			const float Ey = By + Bh + 6.f, Ew = Bw - 40.f;
			const float EMax = FMath::Max(1.f, J->GetEnergyMax()), E01 = J->GetEnergy() / EMax, Cap = J->GetEnergyCap() / EMax;
			P.Rect(Bx, Ey, Ew, 5.f, A(CyanC, 0.15f));
			for (float Hx = Ew * Cap; Hx < Ew; Hx += 5.f) { P.Rect(Bx + Hx, Ey, 2.f, 5.f, Orange, 1); }
			if (Mut > 0)
			{
				const bool bOn = FMath::Fmod(Clock * (2.f + 2.f * Mut), 1.f) < 0.6f;
				P.Rect(Bx, Ey, Ew * Cap, 5.f, bOn ? RedC : A(RedC, 0.35f), 1);
				static const TCHAR* Roman[4] = { TEXT(""), TEXT("I"), TEXT("II"), TEXT("III") };
				P.Text(FString::Printf(TEXT("MUTATION %s"), Roman[FMath::Clamp(Mut, 0, 3)]), Bx + Ew + 8.f, Ey - 6.f, 0, 15.f, RedC);
			}
			else
			{
				P.Rect(Bx, Ey, Ew * FMath::Clamp(E01, 0.f, 1.f), 5.f, E01 < 0.25f && Blink(0.3f, 0.15f) ? White : CyanC, 1);
				P.Text(FString::Printf(TEXT("%d"), FMath::RoundToInt(J->GetEnergy())), Bx + Ew + 8.f, Ey - 6.f, 0, 15.f, CyanC);
			}
		}
		// Greffes en main : petites cases (touches 1…4), la greffe active pleine ; son nom à côté, dans sa couleur.
		{
			const UEONTuningSubsystem* Tu = UEONTuningSubsystem::Get(this);
			UDataTable* Gt = Tu ? Tu->GetGreffes() : nullptr;
			const TArray<FName> Set = J->GetGreffeSet();
			const float Gy = By - 36.f;
			float Gx = Bx;
			for (int32 I = 0; I < Set.Num() && I < 4; ++I)
			{
				const bool bAct = Set[I] == J->GetGreffeId();
				P.Rect(Gx, Gy, 22.f, 22.f, bAct ? Amber : A(Bg, 0.75f));
				P.Line({ { Gx, Gy }, { Gx + 22.f, Gy }, { Gx + 22.f, Gy + 22.f }, { Gx, Gy + 22.f }, { Gx, Gy } }, Amber, 1.5f);
				P.Text(FString::FromInt(I + 1), Gx + 11.f, Gy + 1.f, 0, 16.f, bAct ? Bg : Amber, 1);
				Gx += 28.f;
			}
			if (const FEONGreffeRow* Row = Gt ? Gt->FindRow<FEONGreffeRow>(J->GetGreffeId(), TEXT("HUD"), false) : nullptr)
			{
				P.Text(TEXT("COBRA : ") + Upper(Row->DisplayName), Gx + 6.f, Gy + 1.f, 0, 16.f, Row->Col);
				const float Ch = J->GetCharge01();
				if (Ch > 0.f)
				{
					P.Rect(Gx + 6.f, Gy + 24.f, 140.f, 3.f, A(Row->Col, 0.2f));
					P.Rect(Gx + 6.f, Gy + 24.f, 140.f * FMath::Min(Ch, 1.f), 3.f, Ch >= 1.f ? White : Row->Col, 1);
				}
			}
		}
		// Ligne d'informations : ADN (niveau), scrap, Données, armure de mutation.
		{
			const float Iy = By + Bh + 18.f;
			float Ix = X0;
			const FLinearColor G2 = J->GetLevelPending() > 0 && Blink(0.6f, 0.35f) ? White : Green;
			const FString Adn = FString::Printf(TEXT("ADN NIV %d%s"), J->GetLevel(), J->GetLevelPending() > 0 ? TEXT(" +") : TEXT(""));
			P.Text(Adn, Ix, Iy, 1, 14.f, G2);
			Ix += P.Measure(Adn, 1, 14.f).X + 8.f;
			P.Rect(Ix, Iy + 7.f, 70.f, 3.f, A(Green, 0.2f));
			P.Rect(Ix, Iy + 7.f, 70.f * FMath::Clamp(J->GetADN() / J->GetADNNeed(), 0.f, 1.f), 3.f, Green, 1);
			Ix += 84.f;
			const FString Sc = FString::Printf(TEXT("SCRAP %d"), J->GetScrap());
			P.Text(Sc, Ix, Iy, 1, 14.f, Amber);
			Ix += P.Measure(Sc, 1, 14.f).X + 14.f;
			if (const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this); MS && MS->GetDonnees() > 0)
			{
				const FString Dn = FString::Printf(TEXT("DONNÉES %d"), MS->GetDonnees());
				P.Text(Dn, Ix, Iy, 1, 14.f, CyanC);
				Ix += P.Measure(Dn, 1, 14.f).X + 14.f;
			}
			if (J->GetArmorPerm() > 0) { P.Text(FString::Printf(TEXT("ARMURE %d"), J->GetArmorPerm()), Ix, Iy, 1, 14.f, Orange); }
		}
	}

	// ---------------------------------------------------------------- bas centre : charges de dash
	{
		const int32 N = FMath::Max(1, J->GetDashMax());
		const float Sw = 74.f, Sh = 10.f, Gp = 6.f, Tw = N * Sw + (N - 1) * Gp, X0 = SW * 0.5f - Tw * 0.5f, Y = SH - 92.f;
		for (int32 I = 0; I < N; ++I) { P.Slant(X0 + I * (Sw + Gp), Y, Sw, Sh, I < J->GetDashCharges() ? White : A(White, 0.25f)); }
		// Icône de dash : trois chevrons.
		for (int32 I = 0; I < 3; ++I)
		{
			const float Cx = SW * 0.5f - 9.f + I * 6.f, Cy = Y - 14.f;
			P.Line({ { Cx, Cy - 4.f }, { Cx + 4.f, Cy }, { Cx, Cy + 4.f } }, A(White, 0.5f + 0.25f * I), 2.f);
		}
		P.Line({ { SW * 0.5f - 180.f, Y + 26.f }, { SW * 0.5f + 180.f, Y + 26.f } }, A(Amber, 0.6f), 1.f);
	}

	// ---------------------------------------------------------------- bas droite : Onde, Holo, Hypercharge
	{
		const bool bBlk = J->AreSpellsBlocked();
		const float Xr = SW - M;
		auto Key = [&](const TCHAR* K, const FString& Lbl, float Y, float K01, bool bReady, bool bBlocked, float Px)
		{
			const FLinearColor C = bBlocked ? A(RedC, 0.6f) : bReady ? White : A(White, 0.45f);
			const float Tw = P.Measure(Lbl, 0, Px).X, Kw = Px + 6.f, Kx = Xr - Tw - 10.f - Kw;
			P.Rect(Kx, Y, Kw, Kw + 2.f, C);
			P.Text(K, Kx + Kw * 0.5f, Y + 1.f, 0, Px, Bg, 1);
			P.Text(Lbl, Xr, Y + 1.f, 0, Px, C, 2);
			P.Rect(Kx, Y + Kw + 6.f, Xr - Kx, 3.f, A(Amber, 0.2f));
			P.Rect(Kx, Y + Kw + 6.f, (Xr - Kx) * FMath::Clamp(K01, 0.f, 1.f), 3.f, bBlocked ? A(RedC, 0.6f) : Amber, 1);
		};
		Key(TEXT("F"), TEXT("HOLO"), SH - 190.f, 1.f - FMath::Max(0.f, J->GetHoloCooldown()) / 7.f, J->GetHoloCooldown() <= 0.f, bBlk, 16.f);
		Key(TEXT("E"), TEXT("ONDE"), SH - 150.f, 1.f - FMath::Max(0.f, J->GetSpellCooldown()) / 0.9f, J->GetSpellCooldown() <= 0.f, bBlk, 16.f);
		const float Ult = J->GetUlt();
		const bool bReady = Ult >= 100.f;
		Key(TEXT("R"), bReady && Blink(0.5f, 0.3f) ? TEXT("HYPERCHARGE PRÊTE") : TEXT("HYPERCHARGE"), SH - 106.f, Ult / 100.f, true, false, 24.f);
	}
}

void UEONHudWidget::PaintDialog(const EONHud::FPaint& P0, const FEONDlgLine& L, int32 Shown) const
{
	// Maquette du designer (25/11) : boîte bleu nuit au contour ambre, coin coupé en bas à droite, barre ambre à bec
	// sur la gauche, cartouche du nom qui déborde en haut (Kode Mono, halo), texte blanc en Oxanium, sans portrait.
	const FPaint P(P0.G, P0.L, P0.Layer + 6);
	FString Name; FColor Col;
	UEONUISubsystem::CharInfo(L.Who, Name, Col);
	const float SW = P.W(), Bw = 1046.f, Bh = 300.f, Bx = SW * 0.5f - Bw * 0.5f + 18.f, By = 1080.f - 40.f - Bh, Ch = 45.f;

	// Voile léger sur le jeu.
	P.Rect(0.f, 0.f, SW, 1080.f, A(Bg, 0.35f));

	// Boîte : fond et contour, coin coupé à 45° en bas à droite.
	P.Poly({ { Bx, By }, { Bx + Bw, By }, { Bx + Bw, By + Bh - Ch }, { Bx + Bw - Ch, By + Bh }, { Bx, By + Bh } }, A(Bg, 0.96f));
	P.Line({ { Bx, By }, { Bx + Bw, By }, { Bx + Bw, By + Bh - Ch }, { Bx + Bw - Ch, By + Bh }, { Bx, By + Bh } }, Amber, 2.f);

	// Barre ambre à gauche, son bec à mi-hauteur et deux fentes sombres.
	P.Rect(Bx - 16.f, By, 16.f, Bh, Amber);
	P.Poly({ { Bx - 16.f, By + 85.f }, { Bx - 16.f, By + 265.f }, { Bx - 36.f, By + 246.f }, { Bx - 36.f, By + 104.f } }, Amber);
	P.Rect(Bx - 18.f, By + 115.f, 3.f, 41.f, Bg, 1);
	P.Rect(Bx - 18.f, By + 182.f, 3.f, 56.f, Bg, 1);

	// Cartouche du nom : déborde au-dessus de la boîte, fond noir, contour et halo ambre, souligné en biseau.
	const FString Title = Upper(Name);
	const float Tp = 72.f, Tw = P.Measure(Title, 0, Tp).X;
	const float Nx = Bx + 30.f, Ny = By - 48.f, Nw = Tw + 68.f, Nh = 133.f;
	P.Glow(Nx, Ny, Nw, Nh, Amber);
	P.Rect(Nx, Ny, Nw, Nh, FLinearColor(0.f, 0.f, 0.f, 1.f), 1);
	P.Line({ { Nx, Ny }, { Nx + Nw, Ny }, { Nx + Nw, Ny + Nh }, { Nx, Ny + Nh }, { Nx, Ny } }, Amber, 2.f, 2);
	P.Rect(Nx + Nw - 68.f, Ny - 8.f, 68.f, 8.f, FLinearColor(0.f, 0.f, 0.f, 1.f), 1);
	P.Poly({ { Nx, Ny + Nh }, { Nx + 72.f, Ny + Nh }, { Nx + 64.f, Ny + Nh + 8.f }, { Nx, Ny + Nh + 8.f } }, Amber, 2);
	P.Text(Title, Nx + 34.f, Ny + (Nh - P.Measure(Title, 0, Tp).Y) * 0.5f, 0, Tp, Amber, 0, 3);

	// Texte : coupé à la largeur de la boîte une fois pour toutes (les mots ne sautent pas), puis révélé lettre par lettre.
	const float Fp = 30.f, Lh = 38.f, Tx = Bx + 28.f, Ty = By + 118.f, MaxW = Bw - 80.f;
	TArray<FString> Lines;
	{
		TArray<FString> Words;
		L.T.ParseIntoArray(Words, TEXT(" "), true);
		FString Cur;
		for (const FString& W : Words)
		{
			const FString Try = Cur.IsEmpty() ? W : Cur + TEXT(" ") + W;
			if (!Cur.IsEmpty() && P.Measure(Try, 2, Fp).X > MaxW) { Lines.Add(Cur); Cur = W; }
			else { Cur = Try; }
		}
		if (!Cur.IsEmpty()) { Lines.Add(Cur); }
	}
	int32 Left = Shown;
	for (int32 I = 0; I < Lines.Num() && Left > 0; ++I)
	{
		P.Text(Lines[I].Left(Left), Tx, Ty + I * Lh, 2, Fp, White, 0, 3);
		Left -= Lines[I].Len() + 1;
	}

	// Invite de suite, quand la ligne est entière.
	if (Shown >= L.T.Len() && Blink(0.8f, 0.5f))
	{
		P.Text(TEXT("ESPACE / (A) : SUITE     ÉCHAP / (B) : PASSER"), Bx + Bw - Ch - 10.f, By + Bh - 34.f, 0, 15.f, A(Amber, 0.75f), 2, 3);
	}
}

// ---------------------------------------------------------------- terminal du C.O.M (charte, page « 07 COMPOSANTS UI »)
// Cadre noir à coin coupé et trait ambre, cartouche « C.O.M | TERMINAL », onglets segmentés (l'onglet actif plein ambre),
// liste à gauche avec curseur de défilement, fiche à droite ; levier en curseur vertical, boutons plein ambre ou contour.
void UEONHudWidget::PaintCom(const EONHud::FPaint& P, const UEONUISubsystem& UI, const AEONCharacter& J, const AEONFloorManager* F) const
{
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	const float SW = P.W();
	const FLinearColor Body = A(White, 0.86f), Dim = A(White, 0.45f), Off = A(Amber, 0.28f);

	// Coupe le texte à la largeur Wd (mesure réelle) ; renvoie le Y sous la dernière ligne.
	auto Para = [&P](const TArray<FString>& Src, float X, float Y, float Wd, int32 Fo, float Px, float Lh, const FLinearColor& C)
	{
		for (const FString& S : Src)
		{
			TArray<FString> Words; S.ParseIntoArray(Words, TEXT(" "), true);
			FString Cur;
			for (const FString& Wo : Words)
			{
				const FString Try = Cur.IsEmpty() ? Wo : Cur + TEXT(" ") + Wo;
				if (!Cur.IsEmpty() && P.Measure(Try, Fo, Px).X > Wd) { P.Text(Cur, X, Y, Fo, Px, C); Y += Lh; Cur = Wo; }
				else { Cur = Try; }
			}
			P.Text(Cur, X, Y, Fo, Px, C); Y += Lh;
		}
		return Y;
	};
	// Bouton de la charte : plein ambre (actif) ou contour gris (inactif).
	auto Button = [&P](const FString& S, float X, float Y, float Wd, float H, bool bOn)
	{
		if (bOn) { P.Rect(X, Y, Wd, H, Amber, 1); }
		else { P.Line({ { X, Y }, { X + Wd, Y }, { X + Wd, Y + H }, { X, Y + H }, { X, Y } }, A(White, 0.4f), 2.f, 1); }
		P.Text(S, X + Wd * 0.5f, Y + H * 0.5f - 13.f, 0, 22.f, bOn ? Bg : A(White, 0.55f), 1, 3);
	};

	// Voile et cadre.
	P.Rect(0.f, 0.f, SW, 1080.f, A(Bg, 0.82f));
	const float X0 = 150.f, Y0 = 80.f, Pw = SW - 300.f, Ph = 920.f, Ch = 40.f;
	P.Poly({ { X0, Y0 }, { X0 + Pw, Y0 }, { X0 + Pw, Y0 + Ph - Ch }, { X0 + Pw - Ch, Y0 + Ph }, { X0, Y0 + Ph } }, A(Bg, 0.97f));
	P.Line({ { X0, Y0 }, { X0 + Pw, Y0 }, { X0 + Pw, Y0 + Ph - Ch }, { X0 + Pw - Ch, Y0 + Ph }, { X0, Y0 + Ph }, { X0, Y0 } }, Amber, 2.f);

	// Cartouche du titre : « C.O.M » plein ambre, « TERMINAL » au trait.
	const float Hx = X0 + 30.f, Hy = Y0 + 28.f, Hh = 64.f;
	const float Cw = P.Measure(TEXT("C.O.M"), 0, 34.f).X + 36.f, Tw = P.Measure(TEXT("TERMINAL"), 0, 40.f).X + 44.f;
	P.Rect(Hx, Hy, Cw, Hh, Amber);
	P.Text(TEXT("C.O.M"), Hx + Cw * 0.5f, Hy + 10.f, 0, 34.f, Bg, 1);
	P.Line({ { Hx + Cw, Hy }, { Hx + Cw + Tw, Hy }, { Hx + Cw + Tw, Hy + Hh }, { Hx + Cw, Hy + Hh } }, Amber, 2.f);
	P.Text(TEXT("TERMINAL"), Hx + Cw + 22.f, Hy + 7.f, 0, 40.f, Amber);
	// Ligne de transmission rouge (charte) : lieu, pilote, solde de Données.
	const FString Where = UI.IsDocked() ? FString(TEXT("KIBO - À QUAI")) : FString::Printf(TEXT("ÉTAGE %02d"), F ? F->GetFloorN() : 0);
	const float Ly = Hy + 6.f, Rr = X0 + Pw - 30.f;
	const FString Meta[3] = { Where, TEXT("PILOTE : JIRA"), FString::Printf(TEXT("DONNÉES %d"), MS ? MS->GetDonnees() : 0) };
	float Mx = Rr;
	for (int32 I = 2; I >= 0; --I)
	{
		const float Mw = P.Measure(Meta[I], 0, 15.f).X;
		Mx -= Mw;
		P.Text(Meta[I], Mx, Ly, 0, 15.f, I == 2 ? CyanC : RedC);
		Mx -= 30.f; P.Rect(Mx + 8.f, Ly + 6.f, 14.f, 6.f, RedC); Mx -= 20.f;
	}
	P.Line({ { Hx + Cw + Tw + 30.f, Ly + 30.f }, { Rr, Ly + 30.f } }, A(RedC, 0.7f), 0.75f);
	P.Line({ { Rr, Ly + 30.f }, { Rr, Ly + 52.f } }, A(RedC, 0.7f), 0.75f);

	// Onglets segmentés : contour ambre, l'actif plein ambre, coin coupé sur le dernier.
	static const TCHAR* Tabs[5] = { TEXT("1  AMÉLIORATIONS"), TEXT("2  LOGS"), TEXT("3  QUÊTES"), TEXT("4  DESCENTE"), TEXT("5  CASSETTES") };
	const int32 Tab = UI.GetComTab();
	const float Ty = Y0 + 124.f, Th = 50.f, Tx = X0 + 30.f, TwAll = Pw - 60.f, Cell = TwAll / 5.f, Cc = 16.f;
	for (int32 I = 0; I < 5; ++I)
	{
		const float Cx = Tx + I * Cell;
		const bool bLast = I == 4, bOn = I == Tab;
		if (bOn)
		{
			if (bLast) { P.Poly({ { Cx, Ty }, { Cx + Cell - Cc, Ty }, { Cx + Cell, Ty + Cc }, { Cx + Cell, Ty + Th }, { Cx, Ty + Th } }, Amber); }
			else { P.Rect(Cx, Ty, Cell, Th, Amber); }
		}
		P.Text(Tabs[I], Cx + Cell * 0.5f, Ty + 11.f, 0, 22.f, bOn ? Bg : Amber, 1);
		if (I > 0) { P.Line({ { Cx, Ty }, { Cx, Ty + Th } }, Amber, 2.f); }
	}
	P.Line({ { Tx, Ty }, { Tx + TwAll - Cc, Ty }, { Tx + TwAll, Ty + Cc }, { Tx + TwAll, Ty + Th }, { Tx, Ty + Th }, { Tx, Ty } }, Amber, 2.f);

	// Zone de contenu, aide en bas, filet rouge de la charte.
	const float Cy = Ty + Th + 36.f, Cb = Y0 + Ph - 80.f;
	const float Lx = X0 + 30.f, Lw = 560.f, Rx = Lx + Lw + 80.f, Rw = X0 + Pw - 40.f - Rx;
	auto Help = [&](const TCHAR* S) { P.Text(S, X0 + 30.f, Y0 + Ph - 52.f, 0, 16.f, A(Amber, 0.7f)); };
	P.Rect(X0 + 30.f, Y0 + Ph - 20.f, 320.f, 4.f, RedC);
	P.Rect(X0 + 360.f, Y0 + Ph - 20.f, 6.f, 4.f, RedC);
	const int32 Sel0 = UI.GetComSel(Tab);

	// Liste à gauche (lignes de 44 px), curseur de défilement, séparateur.
	auto List = [&](int32 N, int32 Sel, TFunctionRef<void(int32 I, float X, float Y, bool bOn)> Row)
	{
		const float Rh = 44.f;
		const int32 Vis = FMath::Max(1, FMath::FloorToInt((Cb - Cy) / Rh));
		const int32 Of = FMath::Clamp(Sel - Vis + 1, 0, FMath::Max(0, N - Vis));
		for (int32 Jn = 0; Jn < FMath::Min(Vis, N); ++Jn)
		{
			const int32 I = Jn + Of;
			const float Y = Cy + Jn * Rh;
			const bool bOn = I == Sel;
			if (bOn) { P.Rect(Lx, Y, Lw, Rh - 6.f, A(Amber, 0.16f)); P.Rect(Lx, Y, 4.f, Rh - 6.f, Amber); }
			Row(I, Lx + 20.f, Y + 6.f, bOn);
		}
		const float Sx = Lx + Lw + 24.f;
		P.Line({ { Sx, Cy }, { Sx, Cb } }, A(White, N > Vis ? 0.8f : 0.25f), 2.f);
		if (N > Vis)
		{
			const float Hy2 = Cy + (Cb - Cy - 14.f) * (float)Of / (float)FMath::Max(1, N - Vis);
			P.Rect(Sx - 16.f, Hy2, 32.f, 14.f, Amber, 2);
			P.Rect(Sx - 8.f, Hy2 + 6.f, 16.f, 2.f, Bg, 3);
		}
		P.Line({ { Lx + Lw + 52.f, Cy }, { Lx + Lw + 52.f, Cb } }, Off, 0.75f);
	};
	// Titre de fiche (Kode Mono) et sous-titre à droite.
	auto Head = [&](const FString& S, const FLinearColor& C, const FString& Sub, const FLinearColor& SubC)
	{
		P.Text(S, Rx, Cy - 4.f, 0, 34.f, C);
		if (!Sub.IsEmpty()) { P.Text(Sub, Rx + Rw, Cy + 6.f, 0, 18.f, SubC, 2); }
		P.Line({ { Rx, Cy + 46.f }, { Rx + Rw, Cy + 46.f } }, Off, 0.75f);
		return Cy + 66.f;
	};

	if (Tab == 3)
	{
		// Le levier : curseur vertical de la charte ; poignée en haut quand le gardien est neutralisé.
		const bool bReady = F && F->IsFloorDone();
		const float Mxc = X0 + Pw * 0.5f, T0 = Cy + 10.f, T1 = Cb - 170.f;
		P.Rect(Mxc - 3.f, T0, 6.f, T1 - T0, A(White, bReady ? 0.9f : 0.35f));
		const float Hy3 = bReady ? T0 + 10.f : T1 - 34.f;
		P.Rect(Mxc - 60.f, Hy3, 120.f, 26.f, bReady ? Amber : RedC, 2);
		P.Rect(Mxc - 30.f, Hy3 + 11.f, 60.f, 4.f, Bg, 3);
		if (bReady)
		{
			P.Glow(Mxc - 60.f, Hy3, 120.f, 26.f, Amber);
			Button(TEXT("LEVIER PRÊT"), Mxc - 150.f, T1 + 30.f, 300.f, 52.f, true);
		}
		else
		{
			// « VERROUILLÉ » entre crochets rouges (charte, bouton refusé).
			const float Bw = 300.f, Bx = Mxc - Bw * 0.5f, By = T1 + 30.f, Bh = 52.f, Br = 12.f;
			P.Line({ { Bx + Br, By }, { Bx, By }, { Bx, By + Bh }, { Bx + Br, By + Bh } }, RedC, 2.f);
			P.Line({ { Bx + Bw - Br, By }, { Bx + Bw, By }, { Bx + Bw, By + Bh }, { Bx + Bw - Br, By + Bh } }, RedC, 2.f);
			P.Text(TEXT("VERROUILLÉ"), Mxc, By + 13.f, 0, 22.f, RedC, 1);
			if (F)
			{
				P.Text(FString::Printf(TEXT("Gardien : %s du C.O.M  -  carte rouge %s"), *Upper(F->GuardDir()), F->HasRedCard() ? TEXT("en main") : TEXT("à trouver")),
					Mxc, By + Bh + 26.f, 1, 22.f, Body, 1);
			}
		}
		Help(TEXT("ENTRÉE / (A) : TIRER LE LEVIER     Q/D : ONGLET     C OU ÉCHAP / (B) : FERMER"));
		return;
	}

	if (Tab == 4)
	{
		// Cassettes : niveaux à gauche, fiche et bouton Installer à droite.
		const TArray<UEONUISubsystem::FCassRow>& Rows = UEONUISubsystem::CassRows();
		if (!Rows.Num()) { return; }
		const int32 Sel = FMath::Clamp(Sel0, 0, Rows.Num() - 1);
		List(Rows.Num(), Sel, [&](int32 I, float X, float Y, bool bOn)
		{
			const UEONUISubsystem::FCassRow& Rw2 = Rows[I];
			FString T2; int32 Cost2 = 0;
			const int32 St2 = UI.CassState(Rw2, T2, Cost2);
			const FString Lbl = Rw2.Lock.Len() ? Rw2.Lock : FString::Printf(TEXT("Shield NV%d%s"), Rw2.Lvl, Rw2.Br.Len() ? *(TEXT(" ") + Rw2.Br) : TEXT(""));
			const FLinearColor Cc2 = St2 == 0 ? Green : St2 == 1 ? White : St2 == 2 ? Amber : Dim;
			P.Text(St2 == 0 ? TEXT("[X]") : St2 == 1 ? TEXT("[+]") : TEXT("[ ]"), X, Y + 2.f, 0, 18.f, Cc2);
			P.Text(Lbl, X + 50.f, Y, 1, 22.f, bOn ? White : Cc2);
		});
		const UEONUISubsystem::FCassRow& Row = Rows[Sel];
		FString T; int32 Cost = 0;
		const int32 St = UI.CassState(Row, T, Cost);
		if (Row.Lock.Len())
		{
			const float Y = Head(Upper(Row.Lock), Dim, FString(), Dim);
			Para({ TEXT("Catégorie prévue par le GDD Items (Survie, Mobilité, Cobra/Attaque)."), TEXT("À définir ensemble avant de la coder.") }, Rx, Y, Rw, 2, 22.f, 32.f, Dim);
		}
		else
		{
			const int32 L = MS ? MS->CasLvl(TEXT("shield")) : 0;
			const FString Br = MS ? MS->CasBr(TEXT("shield")) : FString();
			float Y = Head(TEXT("CASSETTE SHIELD"), Amber, L ? FString::Printf(TEXT("NV%d%s"), L, Br.Len() ? *(TEXT(" ") + Br) : TEXT("")) : FString(TEXT("PAS ENCORE TROUVÉE")), L ? White : Dim);
			for (int32 Kk = 0; Kk < 3; ++Kk) { P.Slant(Rx + Kk * 64.f, Y, 56.f, 12.f, L > Kk ? Amber : Grey); }
			Y += 34.f;
			// Textes de dialogues.json (cassettes.shield).
			Y = Para({ TEXT("NV1 - Absorbe 1 coup. Revient à l'étage suivant."), TEXT("NV2 - Se recharge après 10 s sans dégâts."),
				TEXT("NV3 A - Shield nettoyant : à l'impact, explose et nettoie la salle autour de Jira."), TEXT("NV3 B - Shield miroir : à l'impact, renvoie les tirs ennemis à l'expéditeur.") },
				Rx, Y, Rw, 2, 22.f, 34.f, Body);
			if (Row.Lvl == 3) { P.Text(TEXT("Choix de branche définitif."), Rx, Y + 10.f, 1, 22.f, Orange); }
		}
		const bool bCan = St == 1 && UI.IsDocked();
		const FString Bt = St == 0 ? FString(TEXT("INSTALLÉ")) : (!UI.IsDocked() && St == 1) ? FString(TEXT("AU KIBO (À QUAI)")) : St == 1 ? TEXT("INSTALLER  ") + T : Upper(T);
		Button(Bt, Rx + Rw - 360.f, Cb - 56.f, 360.f, 52.f, bCan);
		Help(TEXT("Z/S : CHOISIR     ENTRÉE / (A) : INSTALLER     Q/D : ONGLET     C OU ÉCHAP / (B) : FERMER"));
		return;
	}

	// Onglets à liste : Améliorations (0), Logs (1), Quêtes (2).
	const TArray<TPair<FName, int32>> Items = Tab == 0 ? UI.ComItems() : TArray<TPair<FName, int32>>();
	const TArray<UEONUISubsystem::FQuest> Qs = Tab == 2 ? UI.Quests() : TArray<UEONUISubsystem::FQuest>();
	const int32 N = Tab == 0 ? Items.Num() : Tab == 1 ? UEONUISubsystem::NumLogs() : Qs.Num();
	const int32 Sel = FMath::Clamp(Sel0, 0, FMath::Max(0, N - 1));
	const int32 LogN = MS && MS->M() ? FMath::Min(MS->M()->LogN, UEONUISubsystem::NumLogs()) : 0;
	List(N, Sel, [&](int32 I, float X, float Y, bool bOn)
	{
		if (Tab == 0)
		{
			const FEONItemDef* It = EONItems::Find(Items[I].Key);
			if (!It) { return; }
			P.Slant(X, Y + 8.f, 22.f, 12.f, EONItems::Color(It->Rar));
			P.Text(It->Name + (Items[I].Value > 1 ? FString::Printf(TEXT("  x%d"), Items[I].Value) : TEXT("")), X + 40.f, Y, 1, 22.f, bOn ? White : Body);
		}
		else if (Tab == 1)
		{
			const bool bGot = I < LogN;
			FString Title; UEONUISubsystem::Log(I).Split(TEXT("."), &Title, nullptr);
			P.Text(FString::Printf(TEXT("%02d"), I + 1), X, Y + 2.f, 0, 20.f, bGot ? Amber : Off);
			P.Text(bGot ? Title.Left(40) : FString(TEXT("- - - - - - - -")), X + 50.f, Y, 1, 22.f, bGot ? (bOn ? White : Body) : Off);
		}
		else
		{
			const UEONUISubsystem::FQuest& Q = Qs[I];
			P.Text(Q.bOk ? TEXT("[X]") : TEXT("[ ]"), X, Y + 2.f, 0, 18.f, Q.bOk ? Green : Dim);
			P.Text(Q.T, X + 50.f, Y, 1, 22.f, bOn ? White : FLinearColor(Q.C));
		}
	});

	if (Tab == 0)
	{
		// Fiche de Jira, puis l'amélioration choisie.
		const FEONGreffeRow* Gd = nullptr;
		if (const UEONTuningSubsystem* Tu = UEONTuningSubsystem::Get(this)) { if (UDataTable* G = Tu->GetGreffes()) { Gd = G->FindRow<FEONGreffeRow>(J.GetGreffeId(), TEXT("HUD"), false); } }
		const int32 Hb = MS && MS->M() ? MS->M()->HpBonus : 0;
		float Y = Head(TEXT("JIRA"), Amber, FString::Printf(TEXT("NIV %d"), J.GetLevel()), White);
		Y = Para({ TEXT("Greffe active : ") + (Gd ? Gd->DisplayName : J.GetGreffeId().ToString()),
			FString::Printf(TEXT("Niveau ADN : %d   (%d / %d)"), J.GetLevel(), FMath::FloorToInt(J.GetADN()), FMath::RoundToInt(J.GetADNNeed())),
			FString::Printf(TEXT("PV : %d / %d"), J.GetHP(), J.GetMaxHP()) + (Hb ? FString::Printf(TEXT("   dont +%d permanents (insignes)"), Hb) : FString()),
			FString::Printf(TEXT("Dash : %d charge%s     Scrap : %d"), J.GetDashMax(), J.GetDashMax() > 1 ? TEXT("s") : TEXT(""), J.GetScrap()),
			FString::Printf(TEXT("Améliorations prises : %d"), J.GetItems().Num()) }, Rx, Y, Rw, 2, 22.f, 34.f, Body);
		Y += 18.f;
		P.Line({ { Rx, Y }, { Rx + Rw, Y } }, Off, 0.75f);
		Y += 24.f;
		if (!Items.IsValidIndex(Sel)) { Para({ TEXT("Aucune amélioration pour l'instant."), TEXT("Les salles Opportunité, le marchand et les niveaux d'ADN en proposent.") }, Rx, Y, Rw, 2, 22.f, 34.f, Dim); }
		else if (const FEONItemDef* It = EONItems::Find(Items[Sel].Key))
		{
			const FLinearColor Rc = EONItems::Color(It->Rar);
			P.Slant(Rx, Y + 10.f, 30.f, 16.f, Rc);
			P.Text(It->Name + (Items[Sel].Value > 1 ? FString::Printf(TEXT("  x%d"), Items[Sel].Value) : TEXT("")), Rx + 46.f, Y, 0, 28.f, Rc);
			P.Text(Upper((EONItems::RarName(It->Rar) + TEXT("  -  ") + It->Cat.ToString())), Rx + 46.f, Y + 40.f, 0, 16.f, Dim);
			Para({ It->Desc }, Rx, Y + 78.f, Rw, 2, 22.f, 34.f, Body);
		}
	}
	else if (Tab == 1)
	{
		const FString Cnt = FString::Printf(TEXT("%d / %d"), LogN, UEONUISubsystem::NumLogs());
		if (Sel < LogN)
		{
			const float Y = Head(FString::Printf(TEXT("LOG %02d"), Sel + 1), Amber, Cnt, CyanC);
			Para({ UEONUISubsystem::Log(Sel) }, Rx, Y, Rw, 2, 22.f, 34.f, Body);
		}
		else
		{
			const float Y = Head(TEXT("SIGNAL NON CAPTÉ"), RedC, Cnt, CyanC);
			Para({ TEXT("Ce log est quelque part dans les étages. Les logs se trouvent dans les salles calmes et au sol, après les combats.") }, Rx, Y, Rw, 2, 22.f, 34.f, Dim);
		}
	}
	else if (Qs.IsValidIndex(Sel))
	{
		const UEONUISubsystem::FQuest& Q = Qs[Sel];
		const float Y = Head(Upper(Q.T), Amber, Q.bOk ? TEXT("TERMINÉE") : TEXT("EN COURS"), Q.bOk ? Green : Orange);
		Para(Q.L, Rx, Y, Rw, 2, 22.f, 34.f, Body);
	}
	Help(TEXT("Q/D : ONGLET     Z/S : CHOISIR     C OU ÉCHAP / (B) : FERMER"));
}

// ---------------------------------------------------------------- bannières et invites (charte, pages 05 et 07)
// Bannière centrale en cartouche (Kode Mono), invites « touche + action » comme « R HYPERCHARGE », fiche du socle,
// transmission d'un log et encart « DÉBLOQUÉ ».
void UEONHudWidget::PaintNotices(const EONHud::FPaint& P, const AEONCharacter& J, const AEONFloorManager* F, bool bPad) const
{
	const float SW = P.W(), Cx = SW * 0.5f;
	const FLinearColor Body = A(White, 0.86f);

	// Coupe le texte à la largeur Wd (mesure réelle).
	auto Wrap = [&P](const FString& S, int32 Fo, float Px, float Wd)
	{
		TArray<FString> Out, Words; S.ParseIntoArray(Words, TEXT(" "), true);
		FString Cur;
		for (const FString& Wo : Words)
		{
			const FString Try = Cur.IsEmpty() ? Wo : Cur + TEXT(" ") + Wo;
			if (!Cur.IsEmpty() && P.Measure(Try, Fo, Px).X > Wd) { Out.Add(Cur); Cur = Wo; }
			else { Cur = Try; }
		}
		if (!Cur.IsEmpty()) { Out.Add(Cur); }
		return Out;
	};
	// Cadre de la charte : fond noir, trait coloré, coin bas droit coupé.
	auto Frame = [&P](float X, float Y, float Wd, float H, const FLinearColor& C, float Al, float Ch = 18.f)
	{
		P.Poly({ { X, Y }, { X + Wd, Y }, { X + Wd, Y + H - Ch }, { X + Wd - Ch, Y + H }, { X, Y + H } }, A(Bg, 0.92f * Al));
		P.Line({ { X, Y }, { X + Wd, Y }, { X + Wd, Y + H - Ch }, { X + Wd - Ch, Y + H }, { X, Y + H }, { X, Y } }, A(C, Al), 2.f);
	};
	// Invite : touche en carré blanc (C ou B), puis l'action en Kode Mono.
	auto Prompt = [&](const FString& Act, float Y, bool bOn)
	{
		const FString Key = bPad ? TEXT("B") : TEXT("C");
		const float Kw = 40.f, Tw = P.Measure(Act, 0, 24.f).X, X = Cx - (Kw + 16.f + Tw) * 0.5f;
		P.Poly({ { X, Y }, { X + Kw - 10.f, Y }, { X + Kw, Y + 10.f }, { X + Kw, Y + Kw }, { X, Y + Kw } }, A(White, bOn ? 1.f : 0.5f));
		P.Text(Key, X + Kw * 0.5f, Y + 6.f, 0, 24.f, Bg, 1);
		P.Text(Act, X + Kw + 16.f, Y + 5.f, 0, 24.f, bOn ? White : A(White, 0.5f));
		P.Line({ { X + Kw + 16.f, Y + Kw + 6.f }, { X + Kw + 16.f + Tw, Y + Kw + 6.f } }, A(bOn ? Amber : Grey, 0.9f), 2.f);
	};

	float TopY = 150.f;   // sous la barre du monstre

	if (F)
	{
		// Transmission d'un log : le texte s'écrit à 40 caractères par seconde.
		FString Lt; int32 Ln = 0; float El = 0.f;
		if (F->GetLogMsg(Lt, Ln, El))
		{
			const float Dur = Lt.Len() / 40.f + 5.f;
			const float Al = FMath::Clamp(FMath::Min3(1.f, El * 4.f, (Dur - El) * 2.f), 0.f, 1.f);
			const float Bw = 900.f, Bx = Cx - Bw * 0.5f;
			const TArray<FString> Wr = Wrap(Lt, 2, 22.f, Bw - 60.f);
			const float Bh = 70.f + Wr.Num() * 32.f;
			Frame(Bx, TopY, Bw, Bh, CyanC, Al);
			P.Rect(Bx, TopY, 12.f, Bh - 0.f, A(CyanC, Al));
			P.Text(FString::Printf(TEXT("TRANSMISSION  //  LOG %02d / %02d"), Ln, UEONUISubsystem::NumLogs()), Bx + 30.f, TopY + 14.f, 0, 18.f, A(CyanC, Al));
			if (Blink(0.6f, 0.3f)) { P.Rect(Bx + Bw - 40.f, TopY + 18.f, 12.f, 12.f, A(RedC, Al)); }
			int32 Left = FMath::FloorToInt(El * 40.f);
			for (int32 I = 0; I < Wr.Num() && Left > 0; ++I) { P.Text(Wr[I].Left(Left), Bx + 30.f, TopY + 50.f + I * 32.f, 2, 22.f, A(Body, Al)); Left -= Wr[I].Len() + 1; }
			TopY += Bh + 16.f;
		}

		// Bannière centrale (1,8 s) : cartouche au trait de la couleur de l'annonce, bloc plein à gauche.
		if (F->BannerT > 0.f && !F->BannerText.IsEmpty())
		{
			const float Al = FMath::Min(1.f, F->BannerT / 0.3f);
			const float In = FMath::Clamp((1.8f - F->BannerT) / 0.15f, 0.f, 1.f);
			FLinearColor C = F->BannerColor; C.A = 1.f;
			const FString S = Upper(F->BannerText);
			const float Tw = P.Measure(S, 0, 38.f).X, Bh = 76.f, Bw = (Tw + 110.f) * (0.6f + 0.4f * In), Bx = Cx - Bw * 0.5f, By = 300.f;
			Frame(Bx, By, Bw, Bh, C, Al, 22.f);
			P.Rect(Bx, By, 24.f, Bh, A(C, Al));
			for (int32 I = 0; I < 3; ++I) { P.Slant(Bx + 4.f, By + 12.f + I * 20.f, 16.f, 10.f, A(Bg, 0.8f * Al), 1); }
			P.Text(S, Cx + 12.f, By + 15.f, 0, 38.f, A(C, Al * In), 1, 3);
			P.Rect(Bx + 40.f, By + Bh + 8.f, 120.f, 3.f, A(RedC, Al));
			P.Rect(Bx + 166.f, By + Bh + 8.f, 4.f, 3.f, A(RedC, Al));
		}

		// Invites : porte intérieure fermée à portée, terminal du C.O.M.
		const UEONUISubsystem* UI = UEONUISubsystem::Get(this);
		const bool bFree = !(UI && UI->IsBlocking()) && !J.IsLevelUpOpen() && !J.IsDead();
		if (bFree)
		{
			if (const AEONTileRoom* Rm = F->GetRoom(); Rm && !(F->IsHubMode() && F->GetCurrentRole() == TEXT("DEPART"))
				&& Rm->IsNearClosedInnerDoor(Rm->WorldToRoomPx(J.GetActorLocation()), 40.f))
			{
				Prompt(TEXT("OUVRIR LA PORTE"), 760.f, true);
			}
			if (F->IsNearComTerminal() && !F->IsComTravelling()) { Prompt(TEXT("CONSULTER LE C.O.M"), 760.f, F->IsFloorDone() || F->IsDockedMode()); }

			// Fiche de l'objet du socle le plus proche (40 px) : nom, rareté, effet, prix.
			if (AEONTileRoom* Rm = F->GetRoom())
			{
				if (AEONPedestal* Pd = AEONPedestal::Nearest(GetWorld(), Rm->WorldToRoomPx(J.GetActorLocation()), 40.f))
				{
					if (const FEONItemDef* It = EONItems::Find(Pd->Item))
					{
						const FLinearColor Rc = EONItems::Color(It->Rar);
						const float Cw = 600.f, X = Cx - Cw * 0.5f, Y = 740.f;
						const TArray<FString> Wr = Wrap(It->Desc, 2, 21.f, Cw - 60.f);
						const float Ch = 104.f + Wr.Num() * 30.f + (Pd->Price > 0 ? 40.f : 0.f);
						Frame(X, Y, Cw, Ch, Amber, 1.f);
						P.Rect(X, Y, Cw, 5.f, Rc);
						P.Slant(X + 30.f, Y + 30.f, 30.f, 16.f, Rc);
						P.Text(It->Name, X + 76.f, Y + 20.f, 0, 28.f, Rc);
						P.Text(Upper(EONItems::RarName(It->Rar) + TEXT("  -  ") + It->Cat.ToString()), X + 76.f, Y + 58.f, 0, 15.f, A(White, 0.5f));
						float Ty = Y + 92.f;
						for (const FString& L : Wr) { P.Text(L, X + 30.f, Ty, 2, 21.f, Body); Ty += 30.f; }
						if (Pd->Price > 0)
						{
							const bool bOk = J.GetScrap() >= Pd->Price;
							P.Text(FString::Printf(TEXT("%d SCRAP"), Pd->Price), X + Cw - 40.f, Ty + 8.f, 0, 22.f, bOk ? Amber : RedC, 2);
						}
					}
				}
			}
		}
	}

	// Encart « DÉBLOQUÉ » (3,2 s), en haut au centre, sous le log s'il y en a un.
	if (!J.IsDead())
	{
		const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
		float Life = 0.f;
		if (const FEONUnlockDef* Ud = MS ? MS->GetToast(Life) : nullptr)
		{
			const float Al = FMath::Clamp(FMath::Min(Life * 3.f, (3.2f - Life) * 6.f), 0.f, 1.f);
			const FString Hd = Upper(TEXT("Débloqué  -  ") + Ud->Kind);
			const float Bw = FMath::Max(P.Measure(Ud->Name, 0, 30.f).X, P.Measure(Hd, 0, 16.f).X) + 140.f, Bx = Cx - Bw * 0.5f, Bh = 92.f;
			Frame(Bx, TopY, Bw, Bh, Amber, Al);
			P.Rect(Bx, TopY, 64.f, Bh, A(Amber, Al));
			P.Text(TEXT("+"), Bx + 32.f, TopY + 18.f, 0, 44.f, A(Bg, Al), 1, 3);
			P.Text(Hd, Bx + 90.f, TopY + 14.f, 0, 16.f, A(Amber, Al));
			P.Text(Ud->Name, Bx + 90.f, TopY + 40.f, 0, 30.f, A(White, Al));
		}
	}
}
