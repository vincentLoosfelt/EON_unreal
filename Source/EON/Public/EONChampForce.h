// EON — réactions du dôme du C.O.M en champ de force (Vincent, 15/11 : « montrer qu'il y a une réaction, que ça
// protège par un champ de force orangé lumineux »). Posé au centre de la sphère du dôme (L_HUB).
// Chaque balle qui traverse la sphère y lance une onde (4 ondes à la fois au plus) ; une impulsion au hasard toutes
// les 3 à 6 s garde le champ vivant ; la position de Jira allume les hexagones proches.
// Tout passe par MPC_EON_Champ (lu par M_EON_ChampForce) : Now, Jira, Impact0..3 = (x, y, z, instant).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONChampForce.generated.h"

class UMaterialParameterCollection;

UCLASS()
class EON_API AEONChampForce : public AActor
{
	GENERATED_BODY()

public:
	AEONChampForce();
	virtual void Tick(float DeltaSeconds) override;

	/** Rayon de la sphère du dôme (cm), centre = position de l'acteur. */
	UPROPERTY(EditAnywhere, Category = "Champ") float Radius = 1046.f;
	/** Sous cette hauteur (monde), pas de dôme : pas d'onde. */
	UPROPERTY(EditAnywhere, Category = "Champ") float MinZ = 480.f;
	UPROPERTY(EditAnywhere, Category = "Champ") float AmbientMin = 3.f;
	UPROPERTY(EditAnywhere, Category = "Champ") float AmbientMax = 6.f;
	UPROPERTY(EditAnywhere, Category = "Champ") TObjectPtr<UMaterialParameterCollection> Collection;

	/** Lance une onde au point du dôme le plus proche de P. */
	void Impact(const FVector& P);

private:
	TMap<TWeakObjectPtr<AActor>, bool> Inside;
	int32 NextSlot = 0;
	float NextAmbient = 2.f;
};
