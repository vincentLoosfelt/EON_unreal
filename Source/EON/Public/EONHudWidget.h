// EON — HUD de jeu selon la charte du studio (EON_CHARTE_V0 / CHARTE_BETA) et la maquette de Vincent (25/11).
// Widget UMG dessiné en C++ (NativePaint) : coins coupés à 45°, traits de 2 px, ambre #E9A100 sur #01060C,
// Kode Mono (HUD, chiffres) et Oxanium (textes). Polices lues depuis Content/EON/UI/FontsTTF (OFL).
//   haut gauche  : annonces « +1 NOUVELLE GREFFE / CASSETTE / DISQUETTE »
//   haut centre  : barre du monstre (gardien, sinon dernier ennemi touché)
//   haut droite  : mini-carte (charte, page « 09 CARTE »), étage et salle
//   bas gauche   : portrait, vie en segments, énergie, greffes, ADN / scrap / Données
//   bas centre   : charges de dash
//   bas droite   : Onde, Holo, Hypercharge (ultime)
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void PaintHud(const EONHud::FPaint& P, const AEONCharacter* J, const AEONFloorManager* F, bool bDialog) const;
	/** Boîte de dialogue de la maquette (25/11) : cartouche du nom en Kode Mono, texte en Oxanium lettre par lettre. */
	void PaintDialog(const EONHud::FPaint& P, const FEONDlgLine& L, int32 Shown) const;
	/** Terminal du C.O.M selon la charte (page « 07 COMPOSANTS UI ») : onglets, liste, fiche, levier, cassettes. */
	void PaintCom(const EONHud::FPaint& P, const class UEONUISubsystem& UI, const AEONCharacter& J, const AEONFloorManager* F) const;

	struct FToast { FString Txt; FLinearColor Col; float T = 0.f; };
	TArray<FToast> Toasts;
	int32 SeenItems = -1;

	TWeakObjectPtr<AEONEnemy> Foe;
	float FoeT = 0.f;
	float Clock = 0.f;
};
