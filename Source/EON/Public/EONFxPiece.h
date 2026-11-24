// EON — effets en maillage (Vincent, 24/11 : coder les effets « ABS » du PDF des effets spéciaux).
// Un AEONFxPiece est un petit acteur jetable qui vit Life secondes et s'efface : plans lumineux à plat (M_EON_Glow,
// toutes les formes : flash de bouche, annonces au sol, flaques), lumière ponctuelle qui décroît, fantôme de Jira
// (copie de sa pose) ou douille qui tombe et rebondit. Les fonctions d'appel sont dans EONFx.h.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONFxPiece.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UPoseableMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(NotBlueprintable, Transient)
class EON_API AEONFxPiece : public AActor
{
	GENERATED_BODY()

public:
	AEONFxPiece();
	virtual void Tick(float Dt) override;

	/** Plan lumineux à plat (plan moteur 100 × 100 cm), centré en Loc (relatif), tourné de Yaw, de taille en cm. */
	UStaticMeshComponent* AddPlane(UMaterialInstanceDynamic* Mid, const FVector& Loc, float Yaw, float SizeXCm, float SizeYCm);
	UPointLightComponent* AddLight(const FLinearColor& Col, float Intensity, float RadiusCm);
	UPoseableMeshComponent* AddGhost(class USkeletalMeshComponent* Src, UMaterialInstanceDynamic* Mid);

	float Life = 0.2f;
	float Age = 0.f;
	/** Opacité de départ ; l'opacité décroît linéairement jusqu'à 0 sur Life (sauf bNoFade). */
	float Opacity0 = 1.f;
	bool bNoFade = false;
	/** Annonces : Remplissage de 0 à 1 sur Life (cercle, cône, ligne qui se remplissent). */
	bool bFill = false;

	/** Douille : vitesse en cm/s, sol en Z monde. Reste ensuite RestLife s, s'efface la dernière seconde. */
	bool bShell = false;
	FVector ShellVel = FVector::ZeroVector;
	float FloorZ = 0.f;
	bool bRest = false;
	float RestLife = 4.f;

	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Mids;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
	float Light0 = 0.f;

	/** Douilles en jeu (le proto en garde 160 au plus ; ici 80). */
	static TArray<TWeakObjectPtr<AEONFxPiece>> Shells;

private:
	void TickShell(float Dt);
};
