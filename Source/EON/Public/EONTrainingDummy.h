// EON — cible d'entraînement increvable (type 'dummy' du proto : hp 9999, r 7 px).
// Elle vacille au tir, clignote en blanc, affiche les dégâts qui s'envolent
// et le total encaissé sur 1,2 s (« DPS »). Placeholder : un cube sur un poteau (MANQUANTS.md §4).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONTrainingDummy.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class EON_API AEONTrainingDummy : public AActor
{
	GENERATED_BODY()

public:
	AEONTrainingDummy();

	/** hitEnemy() du proto, branche 'dummy'. Dir = direction du coup au sol (normalisée). */
	UFUNCTION(BlueprintCallable, Category = "EON|Cible")
	void ApplyShot(float Damage, FVector Dir, float KnockMul, FVector ImpactPoint);

	/** Point visé par l'aide à la visée (centre de la cible). */
	FVector GetAimPoint() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FNum
	{
		FVector Loc;
		float VelZ;
		float Life;
		FString Text;
		FColor Col;
	};

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Capsule;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Post;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMid;

	TArray<FNum> Nums;
	float WobCm = 0.f;
	float FlashT = 0.f;
	float DmgAcc = 0.f;
	float DmgT = 0.f;
};
