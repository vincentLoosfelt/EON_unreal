// EON — lampe posée dans une salle (asset dynamique de l'éditeur de salles, Blueprints BPA_* de RoomAssets/Dynamiques/Lumieres).
// Règles des lampes du proto (room.lights, collectLights) :
//   néon mural : rayon 52 px, intensité 0,8 ; lampe au sol : 24 px, 0,5 ; couleur = néon de la palette de la salle ;
//   clignote  : éteinte (intensité × 0,1) quand floor(temps × 2,2 + phase) % 3 == 0, phase = colonne de tuile × 0,37 ;
//   vacille   : intensité × 0,12 quand sin(temps × 37 + x) × sin(temps × 23 + y) > 0,55 (x, y en px) ;
//   Irruption : les lampes vacillent puis s'éteignent (elles suivent la compensation d'exposition de l'étage).
// Les maillages du Blueprint qui portent le tag « neon » s'allument et s'éteignent avec la lampe (paramètre Emission).
// Réglages par lampe posée : tag « eonlum=mode;#rrggbb;rayon_px;intensite » du ChildActorComponent (écrit à l'import).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONLampe.generated.h"

class UPointLightComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EEONLampeMode : uint8
{
	Fixe     UMETA(DisplayName = "Fixe"),
	Clignote UMETA(DisplayName = "Clignote"),
	Vacille  UMETA(DisplayName = "Vacille"),
};

UCLASS(Blueprintable)
class EON_API AEONLampe : public AActor
{
	GENERATED_BODY()

public:
	AEONLampe();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe") EEONLampeMode Mode = EEONLampeMode::Fixe;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe") FLinearColor Couleur = FLinearColor(FColor(0xff, 0x3d, 0x8b));
	/** Portée en px du proto (16 px = 1 tuile = 1 m). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe", meta = (ClampMin = "1")) float RayonPx = 52.f;
	/** Intensité du proto (0 à 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe", meta = (ClampMin = "0")) float Intensite = 0.8f;
	/** Intensité Unreal pour une intensité proto de 1 (la lampe de Jira : 60000 pour 0,95). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe") float IntensiteUnreal = 63000.f;
	/** Émission des maillages « neon » allumés. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe") float EmissionNeon = 40.f;
	/** Phase du clignotement ; négative = automatique (colonne de tuile × 0,37, comme le proto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Lampe") float Phase = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Lampe") TObjectPtr<USceneComponent> Racine;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Lampe") TObjectPtr<UPointLightComponent> Lumiere;

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:
	void LireReglages();
	void Appliquer(float Facteur);

	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Neons;
	FVector2D PosPx = FVector2D::ZeroVector;
	float Dernier = -1.f;
};
