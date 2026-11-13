// EON — caméra plongeante fixe, perspective longue focale.
// Reproduit le suivi du proto (updateCam / render) : anticipation vers la visée,
// lissage min(1, dt×7), bornes de salle (camClamp), kick amorti min(1, dt×14),
// tremblement 8×trauma² px, trauma qui décroît de 1,8/s.
#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "EONPlayerCameraManager.generated.h"

UCLASS()
class EON_API AEONPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	/** Kick au tir : décale la caméra (direction au sol, intensité en px proto). */
	UFUNCTION(BlueprintCallable, Category = "EON|Caméra")
	void AddKick(FVector Direction, float Px);

	/** shake(v) du proto : trauma += v × réglage Shake, plafonné à 1. */
	UFUNCTION(BlueprintCallable, Category = "EON|Caméra")
	void AddTrauma(float Amount);

	/** Recentre sans lissage (changement de salle, réveil). */
	UFUNCTION(BlueprintCallable, Category = "EON|Caméra")
	void SnapToTarget() { bSnap = true; }

	/** Largeur de sol visible au centre (cm), pour vérifier les 30 m. */
	UFUNCTION(BlueprintPure, Category = "EON|Caméra")
	float GetViewWidthCm() const { return LastViewWidthCm; }

protected:
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

private:
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Kick = FVector2D::ZeroVector;
	float Trauma = 0.f;
	bool bSnap = true;
	float LastViewWidthCm = 0.f;
};
