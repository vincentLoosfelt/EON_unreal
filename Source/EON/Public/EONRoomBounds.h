// EON — emprise d'une salle (murs compris). La caméra ne sort pas de la salle
// où se trouve Jira (camClamp du proto).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONRoomBounds.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class EON_API AEONRoomBounds : public AActor
{
	GENERATED_BODY()

public:
	AEONRoomBounds();

	/** Taille de la salle en tuiles (murs compris). 32×20 = salle standard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle")
	FIntPoint SizeTiles = FIntPoint(32, 20);

	UFUNCTION(BlueprintPure, Category = "EON|Salle")
	FBox2D GetRect2D() const;

	/** Salle qui contient ce point (au sol), ou nullptr. */
	UFUNCTION(BlueprintPure, Category = "EON|Salle", meta = (WorldContext = "WorldContext"))
	static AEONRoomBounds* FindRoomAt(const UObject* WorldContext, FVector Location);

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/** Boîte relevée de 300 cm (salles J1 posées à la main). Faux pour les salles générées : le pivot reste au sol. */
	bool bLiftBox = true;
	UPROPERTY(VisibleAnywhere, Category = "EON|Salle")
	TObjectPtr<UBoxComponent> Box;
};
