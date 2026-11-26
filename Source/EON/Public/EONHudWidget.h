// EON — HUD de jeu selon la charte du studio (EON_CHARTE_V0 / CHARTE_BETA) et la maquette de Vincent (25/11).
// Widget UMG dessiné en C++ (NativePaint) : coins coupés à 45°, traits de 2 px, ambre #E9A100 sur #01060C,
// Kode Mono (HUD, chiffres) et Oxanium (textes). Polices lues depuis Content/EON/UI/FontsTTF (OFL).
//   haut gauche  : annonces « +1 NOUVELLE GREFFE / CASSETTE / DISQUETTE »
//   haut centre  : barre du monstre (gardien, sinon dernier ennemi touché)
//   haut droite  : mini-carte (charte, page « 09 CARTE »), étage et salle
//   bas gauche   : portrait, vie en segments, énergie, greffes, ADN / scrap / Données
//   bas centre   : charges de dash
//   bas droite   : Onde, Holo, Hypercharge (ultime)
// Options du joueur appliquées ici : taille et opacité du HUD, contraste élevé, clignotements réduits,
// mini-carte tournante, compteur d'images, taille du texte des dialogues, style des boutons de manette.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "EONHudWidget.generated.h"

class AEONEnemy;
class AEONCharacter;
class AEONFloorManager;
struct FEONDlgLine;
namespace EONHud { struct FPaint; }

UCLASS()
class EON_API UEONHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void PaintHud(const EONHud::FPaint& P, const AEONCharacter* J, const AEONFloorManager* F, bool bDialog) const;
	/** Boîte de dialogue de la maquette (25/11) : cartouche du nom en Kode Mono, texte en Oxanium lettre par lettre. */
	void PaintDialog(const EONHud::FPaint& P, const FEONDlgLine& L, int32 Shown) const;
	/** Terminal du C.O.M selon la charte (page « 07 COMPOSANTS UI ») : onglets, liste, fiche, levier, cassettes. */
	void PaintCom(const EONHud::FPaint& P, const class UEONUISubsystem& UI, const AEONCharacter& J, const AEONFloorManager* F) const;
	/** Bannière centrale, invites (touche + action), fiche du socle, transmission d'un log, encart « DÉBLOQUÉ ». */
	void PaintNotices(const EONHud::FPaint& P, const AEONCharacter& J, const AEONFloorManager* F, bool bPad) const;
	/** Écran titre : logotype de la charte, sauvegarde du PC, Continuer / Nouvelle partie. */
	void PaintTitle(const EONHud::FPaint& P, const class UEONUISubsystem& UI) const;
	/** Mort : « SIGNAL PERDU » entre crochets rouges, bilan de la run, R / A : nouvelle run, déblocages de la run. */
	void PaintDead(const EONHud::FPaint& P, const AEONCharacter& Jr, const AEONFloorManager* F) const;
	/** Montée de niveau : cartouche « NIVEAU N | ADN ASSIMILÉ » et trois cartes. */
	void PaintLevelUp(const EONHud::FPaint& P, const AEONCharacter& J) const;
	/** Carton-titre du gardien, faille (oxygène, console, brèche), consigne du Kibo, QTE et cut-in de l'ultime. */
	void PaintScene(const EONHud::FPaint& P, const AEONCharacter& J, const AEONFloorManager* F, bool bPad) const;
	/** Menu de réglages F6 : panneau à droite, sous la mini-carte. */
	void PaintDevMenu(const EONHud::FPaint& P, const class AEONPlayerController& PC) const;
	/** Menu pause (Redémarrer, Options, Quitter) ; les options passent par PaintOptions. */
	void PaintPause(const EONHud::FPaint& P, const class UEONUISubsystem& UI, bool bPad) const;
	/** Options : huit onglets, liste (curseurs, choix, touches clavier et manette), aide à droite (EONOptLayout). */
	void PaintOptions(const EONHud::FPaint& P, const class UEONUISubsystem& UI, bool bPad) const;
	/** Compteur d'images (option), sous la mini-carte. */
	void PaintFps(const EONHud::FPaint& P) const;

	struct FToast { FString Txt; FLinearColor Col; float T = 0.f; };
	TArray<FToast> Toasts;
	int32 SeenItems = -1;

	TWeakObjectPtr<AEONEnemy> Foe;
	float FoeT = 0.f;
	float Clock = 0.f;

	/** Images par seconde, moyennées sur une demi-seconde. */
	float FpsAcc = 0.f;
	int32 FpsFrames = 0;
	float Fps = 0.f;
	/** Visée de Jira à l'écran (radians, lissée) : flèche de la mini-carte, rotation de la carte tournante. */
	float MapAng = -UE_HALF_PI;
	bool bMapAng = false;

	UPROPERTY(Transient) TObjectPtr<class UTexture2D> LogoTex;
	FSlateBrush LogoBrush;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> CutInTex;
	FSlateBrush CutInBrush;
};
