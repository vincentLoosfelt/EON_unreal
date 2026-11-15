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
	void DrawMinimap(float S);
	/** Échelle : 4 px écran par px du proto à 1920 × 1080. */
	float U = 4.f;
	float Time = 0.f;
};
