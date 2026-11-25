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
	virtual void DrawHUD() override;
private:
	void Rect(float X, float Y, float W, float H, const FLinearColor& C);
	void Txt(const FString& S, float X, float Y, const FLinearColor& C, float Scale = 1.f, bool bCenter = false);
	/** Crée le widget du HUD de la charte (UEONHudWidget) à la première image. */
	void EnsureHudWidget();
	UPROPERTY(Transient) TObjectPtr<class UEONHudWidget> HudWidget;
	void DrawDevMenu();
	// Jalon 8 : méta et écrans figés (coordonnées du proto 480 × 270, origine O).
	void Box(float X, float Y, float W, float H, const FLinearColor& Border, const FLinearColor& Fill);
	static TArray<FString> Wrap(const FString& S, int32 N);
	/** comLines : lignes coupées à N caractères, 8 px proto d'interligne. Renvoie le nombre de lignes. */
	int32 Lines(const TArray<FString>& L, float X, float Y, const FLinearColor& C, int32 N);
	void DrawToast();
	void DrawCom();
	void DrawTitle();
	float Ox = 0.f, Oy = 0.f;
	/** Échelle : 4 px écran par px du proto à 1920 × 1080. */
	float U = 4.f;
	float Time = 0.f;
	/** JIR-V23 : rendu provisoire de Jira détouré, posé dans la bande du cut-in « Protocole thoracique ». */
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> CutInTex;
};
