// EON — membre coupé qui tombe (Vincent, 25/11 : « découper les parties qui ne sont plus attachées et les détacher du
// corps pour les faire tomber en rigid body », méthode du plan de coupe).
// Copie figée de la pose de l'ennemi (UPoseableMeshComponent), dont le matériau ne garde que le membre (Morceau = k),
// portée par une boîte physique (rigid body) qui ne heurte que le décor statique. Vit Life s puis s'enfonce dans le sol.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONMembre.generated.h"

class UBoxComponent;
class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

/** Membre coupable, en espace du maillage avant skinning (cm). */
struct FEONMembreDef
{
	const TCHAR* Bone;   // os qui porte le membre (place la boîte sur la pose animée)
	FVector J;           // articulation : plan de coupe
	FVector Axe;         // axe du membre (unitaire), vers le bout
	float L;             // longueur
	float R;             // rayon (zone cachée sur le corps, gardée sur le morceau)
	float Ts;            // profondeur de la section testée par les trous
	float Rs;            // rayon de la section testée
	FVector N = FVector::ZeroVector;   // normale du plan de coupe (inclinable jusqu'à 60° de l'axe) ; nulle -> l'axe
	uint8 Rang = 1;      // ordre de coupe aux critiques : 0 tête (explose), 1 bras, 2 jambe (il tombe et meurt)
};

UCLASS(NotBlueprintable, Transient)
class EON_API AEONMembre : public AActor
{
	GENERATED_BODY()

public:
	AEONMembre();
	virtual void Tick(float Dt) override;

	/** Src : maillage de l'ennemi ; SrcMids : ses matériaux à trous (copiés) ; K : index du membre (Morceau = K + 1). */
	void Init(USkeletalMeshComponent* Src, const TArray<UMaterialInstanceDynamic*>& SrcMids, int32 K, const FEONMembreDef& Def,
		const FVector& ShotDir, const FLinearColor& Sang);

	/** Point Ref (espace avant skinning) amené sur la pose animée de Src, en monde, via l'os Bone. */
	static FVector RefToWorld(const USkeletalMeshComponent* Src, FName Bone, const FVector& Ref);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Box;
	UPROPERTY(Transient) TObjectPtr<UPoseableMeshComponent> Pose;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Mids;

	float Life = 6.f;
	float Age = 0.f;
	FLinearColor Sang = FLinearColor::Red;
	float RayonCm = 10.f;
	bool bTache = false;
};
