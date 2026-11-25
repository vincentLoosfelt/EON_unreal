#include "EONHUD.h"
#include "EONHudWidget.h"
#include "EONBoss.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONUI.h"
#include "Misc/App.h"
#include "EONPlayerController.h"
#include "EONTileRoom.h"
#include "EngineUtils.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "Engine/Canvas.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "EONCombatDirector.h"
#include "EONFx.h"
#include "UObject/ConstructorHelpers.h"
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

AEONHUD::AEONHUD()
{
	static ConstructorHelpers::FObjectFinder<UFontFace> Face(TEXT("/Game/EON/UI/Fonts/FF_Bangers.FF_Bangers"));
	NumFace = Face.Object;
	static ConstructorHelpers::FObjectFinder<UTexture2D> Ono(TEXT("/Game/EON/VFX/Planches/T_EON_VFX_P7_Onomatopees.T_EON_VFX_P7_Onomatopees"));
	OnoTex = Ono.Object;
}

void AEONHUD::DrawGrosCoup()
{
	float Age = 0.f;
	const EONFx::FGrosCoup* G = EONFx::GrosCoupActif(&Age);
	if (!G || !Canvas) { return; }
	const float SX = Canvas->ClipX, SY = Canvas->ClipY;
	U = SY / 270.f;
	const FVector P3 = Project(G->At, false);
	const FVector2D C = P3.Z > 0.f ? FVector2D(P3.X, P3.Y) : FVector2D(SX * 0.5f, SY * 0.5f);
	const float A = FMath::Clamp((EONFx::GrosCoupDuree - Age) / 0.12f, 0.f, 1.f);
	// --- Traits de vitesse (référence de Vincent, 25/11) : coins noirs effilés qui pointent vers le coup, laissant
	// une zone claire ovale autour de lui ; nouveau tirage 12 fois par seconde (focus / vitesse façon manga).
	{
		const int32 Img = FMath::FloorToInt(Age * 12.f);
		FRandomStream Rs(G->Graine + Img * 7919);
		const float Loin = FVector2D(SX, SY).Size() * 1.1f;
		const float Rx = SX * 0.24f, Ry = SY * 0.27f;
		TArray<FCanvasUVTri> Tris;
		const int32 N = 110;
		for (int32 I = 0; I < N; ++I)
		{
			const float Th = (I + Rs.FRandRange(-0.45f, 0.45f)) * UE_TWO_PI / N;
			const FVector2D D(FMath::Cos(Th), FMath::Sin(Th));
			// rayon de la zone claire sur l'ovale, plus ou moins entamé par chaque trait
			const float Re = 1.f / FMath::Sqrt(FMath::Square(D.X / Rx) + FMath::Square(D.Y / Ry));
			const float Rin = Re * Rs.FRandRange(0.95f, 1.75f);
			const float W = SY * Rs.FRandRange(0.002f, 0.011f) * (Rs.FRand() < 0.15f ? 2.2f : 1.f);
			const FVector2D Pn(-D.Y, D.X);
			FCanvasUVTri T;
			T.V0_Pos = C + D * Rin;
			T.V1_Pos = C + D * Loin + Pn * W * (Loin / SY);
			T.V2_Pos = C + D * Loin - Pn * W * (Loin / SY);
			T.V0_Color = T.V1_Color = T.V2_Color = FLinearColor(0.f, 0.f, 0.f, A);
			Tris.Add(T);
		}
		FCanvasTriangleItem Ti(Tris, GWhiteTexture);
		Ti.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Ti);
	}
	// --- Onomatopée (planche P7) au-dessus du coup : jaillit (×1,5 puis ×1,15 puis taille) à 12 i/s, penchée.
	if (OnoTex && OnoTex->GetResource())
	{
		const int32 Img = FMath::FloorToInt(Age * 12.f);
		const float Pop = Img == 0 ? 1.5f : Img == 1 ? 1.15f : 1.f;
		const float Wd = 62.f * U * Pop;
		const int32 Cx = G->Mot % 3, Cy = G->Mot / 3;
		const FVector2D Pos = C + FVector2D(10.f * U, -38.f * U);
		FCanvasTileItem T(Pos - FVector2D(Wd, Wd) * 0.5f, OnoTex->GetResource(), FVector2D(Wd, Wd),
			FVector2D(Cx / 3.f, Cy / 2.f), FVector2D((Cx + 1) / 3.f, (Cy + 1) / 2.f), FLinearColor(1.f, 1.f, 1.f, A));
		T.Rotation = FRotator(0.f, G->Rot, 0.f);
		T.PivotPoint = FVector2D(0.5f, 0.5f);
		T.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(T);
	}
}

void AEONHUD::DrawNums()
{
	const AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld());
	if (!D || D->GetNums().Num() == 0) { return; }
	if (!NumFont && NumFace)
	{
		NumFont = NewObject<UFont>(this);
		NumFont->FontCacheType = EFontCacheType::Runtime;
		NumFont->LegacyFontSize = 48;
		FTypefaceEntry& E = NumFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		E.Name = TEXT("Regular");
		E.Font = FFontData(NumFace.Get());
	}
	UFont* F = NumFont ? NumFont.Get() : (GEngine ? GEngine->GetLargeFont() : nullptr);
	if (!F || !Canvas) { return; }
	U = Canvas->ClipY / 270.f;
	for (const AEONCombatDirector::FNum& N : D->GetNums())
	{
		const FVector P = Project(N.Loc, false);
		if (P.Z <= 0.f) { continue; }
		// Jaillit : 1,7 fois plus gros à l'apparition, revient à sa taille en 0,12 s ; s'efface dans le dernier tiers.
		const float Age = N.Life0 - N.Life;
		const float Pop = 1.f + 0.7f * FMath::Exp(-Age * 25.f);
		const float Sc = U / 4.f * 0.75f * N.Taille * Pop;
		const float A = FMath::Clamp(N.Life / FMath::Max(0.01f, N.Life0 * 0.33f), 0.f, 1.f);
		FLinearColor Fill = FLinearColor(N.Col); Fill.A = A;
		const FLinearColor Noir(0.f, 0.f, 0.f, A);
		FCanvasTextItem T(FVector2D(P.X, P.Y), FText::FromString(N.Text), F, Noir);
		T.bCentreX = true; T.bCentreY = true;
		T.Scale = FVector2D(Sc, Sc);
		T.BlendMode = SE_BLEND_Translucent;
		// Contour noir épais : le chiffre en noir tout autour (16 directions), puis le chiffre en couleur par-dessus.
		const float O = 3.2f * Sc;
		for (int32 I = 0; I < 16; ++I)
		{
			const float Ang = I * UE_TWO_PI / 16.f;
			T.Position = FVector2D(P.X + FMath::Cos(Ang) * O, P.Y + FMath::Sin(Ang) * O);
			Canvas->DrawItem(T);
		}
		// ombre portée dure en bas à droite, façon cel
		T.Position = FVector2D(P.X + O * 1.6f, P.Y + O * 1.6f);
		Canvas->DrawItem(T);
		T.Position = FVector2D(P.X, P.Y);
		T.SetColor(Fill);
		Canvas->DrawItem(T);
	}
}

void AEONHUD::PostRender()
{
	EONFx::AvancerHorlogeEcran(FApp::GetDeltaTime());
	Super::PostRender();
	if (!bShowHUD && Canvas) { DrawGrosCoup(); DrawNums(); }
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

	// --- Gros coup (traits de vitesse, onomatopée) puis chiffres de dégâts (manga)
	DrawGrosCoup();
	DrawNums();

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
		// Repère au-dessus du PNJ d'event : « ? » de la gamine, « ?! » puis « ! » d'Ulysse.
		{
			FVector Ew; FString Et; FLinearColor Ec; FVector2D Es;
			if (FC->GetEvtHud(Ew, Et, Ec) && GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(Ew, Es))
			{
				Txt(Et, Es.X, Es.Y - 4 * U + FMath::RoundToFloat(FMath::Sin(Time * 5.f)) * U, Ec, U / 2.5f, true);
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

	// Bannière, invites, fiche du socle, log, encart « DÉBLOQUÉ » : UEONHudWidget::PaintNotices (charte).
	// Gardien, faille, Kibo, QTE, cut-in : UEONHudWidget::PaintScene (charte).
	// Dialogue : UEONHudWidget::PaintDialog (maquette du 25/11).
	DrawDevMenu();
}

// ---------------------------------------------------------------- menu de réglages (F6)

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
