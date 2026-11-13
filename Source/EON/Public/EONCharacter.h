// EON — Jira (socle du jalon 0). Le déplacement ici est PROVISOIRE : il sert à
// tester la caméra et les entrées. Le vrai contrôleur (dash, course, tir) est le jalon 1.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EONCharacter.generated.h"

class UEONSpriteComponent;

UCLASS(Blueprintable)
class EON_API AEONCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEONCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	/** Direction voulue, repère écran : X = droite, Y = haut. Longueur 0..1. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetMoveInput(FVector2D ScreenInput) { RawMove = ScreenInput.GetClampedToMaxSize(1.f); }

	/** Angle de visée au sol (radians, convention du proto : 0 = +X, + = vers +Y). */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetAimAngle(float Radians) { AimAngle = Radians; }

	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	float GetAimAngle() const { return AimAngle; }

	/** Hauteur du bras (départ des tirs) au-dessus du sol, en world. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	FVector GetArmLocation() const;

	/** Chute dans le vide en cours (jalon 2). */
	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	bool IsFallingIntoVoid() const { return bFallingIntoVoid; }

	/** Placeholder pixel (jira_a0). Masqué par le Blueprint quand le vrai mesh est branché. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Jira")
	TObjectPtr<UEONSpriteComponent> PlaceholderSprite;

	/** Vitesse de marche du proto, px/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira")
	float WalkSpeedPx = 82.f;

	/** Rayon de collision du proto, px. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira")
	float CollisionRadiusPx = 4.f;

	/** Taille de Jira dans le proto, px. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira")
	float HeightPx = 29.f;

protected:
	virtual void BeginPlay() override;

private:
	FVector2D RawMove = FVector2D::ZeroVector;
	FVector2D SmoothMove = FVector2D::ZeroVector;
	float AimAngle = 0.f;
	bool bFallingIntoVoid = false;
};
