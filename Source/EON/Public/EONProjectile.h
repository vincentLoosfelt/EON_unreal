// EON — projectile du joueur (greffes Standard, Rafale, Shotgun : fireBullet() du proto).
// Il part du bout du bras, meurt au premier mur ou au premier ennemi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UMaterialInstanceDynamic;

UCLASS()
class EON_API AEONProjectile : public AActor
{
	GENERATED_BODY()
public:
	/** Statistiques de tir (eon.auto) : balles tirées, balles au but, balles arrêtées par le décor. */
	static int32 StatShots;
	static int32 StatHits;
	static int32 StatWalls;

public:
	AEONProjectile();

	/** À appeler avant FinishSpawning. Vitesse en cm/s, durée de vie en s. */
	void Setup(const FVector& InVelocityCm, float InLifeSeconds, float InDamage, float InRadiusPx, const FLinearColor& InColor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON")
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON")
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON")
	TObjectPtr<UProjectileMovementComponent> Movement;

	float Damage = 1.f;
	/** Perks Balles perforantes (traverse N ennemis) et Ricochet (N rebonds sur les murs). */
	void SetPierceBounce(int32 InPierce, int32 InBounce) { PierceLeft = InPierce; BounceLeft = InBounce; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnStop(const FHitResult& Hit);
	UFUNCTION()
	void OnBounce(const FHitResult& Hit, const FVector& ImpactVelocity);
	int32 PierceLeft = 0;
	int32 BounceLeft = 0;
	float SpeedCm = 0.f;

	FVector ShotDir = FVector::ForwardVector;
	FLinearColor Color = FLinearColor::White;
	float LifeSeconds = 1.f;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Mid;
};
