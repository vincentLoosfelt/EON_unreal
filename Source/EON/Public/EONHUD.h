// EON — HUD en jeu (06, section 1), version jalons 2–3 dessinée au Canvas : PV, énergie, dash, greffe,
// mini-carte (section 2), rôle de la salle et étage, bannière centrale, flash rouge, écran « Signal perdu ».
// Le HUD complet (mutation, ADN, sorts, ultime) viendra au jalon 4, en UMG.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EONHUD.generated.h"

UCLASS()
class EON_API AEONHUD : public AHUD
{
	GENERATED_BODY()
public:
	AEONHUD();
	virtual void DrawHUD() override;
	/** HUD masqué (vitrine, cinématique) : les chiffres de dégâts, eux, restent (ils font partie du combat). */
	virtual void PostRender() override;
private:
	void Rect(float X, float Y, float W, float H, const FLinearColor& C);
	void Txt(const FString& S, float X, float Y, const FLinearColor& C, float Scale = 1.f, bool bCenter = false);
	/** Crée le widget du HUD de la charte (UEONHudWidget) à la première image. */
	void EnsureHudWidget();
	UPROPERTY(Transient) TObjectPtr<class UEONHudWidget> HudWidget;
	void DrawDevMenu();
	// Jalon 8 : méta et écrans figés (coordonnées du proto 480 × 270, origine O).
	void Box(float X, float Y, float W, float H, const FLinearColor& Border, const FLinearColor& Fill);
	void DrawToast();
	void DrawTitle();
	/** Chiffres de dégâts manga (Vincent, 25/11) : police Bangers, contour noir épais, qui jaillissent en grossissant. */
	void DrawNums();
	/** Gros coup : traits de vitesse radiaux (procéduraux, 12 i/s) centrés sur le point du coup, puis onomatopée P7. */
	void DrawGrosCoup();
	/** Échelle : 4 px écran par px du proto à 1920 × 1080. */
	float U = 4.f;
	float Time = 0.f;
	/** JIR-V23 : rendu provisoire de Jira détouré, posé dans la bande du cut-in « Protocole thoracique ». */
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> CutInTex;
	/** Face Bangers (OFL, /Game/EON/UI/Fonts/FF_Bangers) et police « runtime » construite dessus au premier dessin. */
	UPROPERTY() TObjectPtr<class UFontFace> NumFace;
	UPROPERTY(Transient) TObjectPtr<class UFont> NumFont;
	/** Planche P7 des onomatopées (3 × 2 : BAM ZAP BOOM / KRAK DOOM VRRM). */
	UPROPERTY() TObjectPtr<class UTexture2D> OnoTex;
};
