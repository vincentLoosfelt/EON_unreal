// EON — jalon 6 : socle d'objet (pedestals du proto). Gratuit : on marche dessus, les autres socles de la salle disparaissent.
// Payant (marchand) : on paie en scrap et on peut racheter les autres tant qu'il en reste.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONPedestal.generated.h"

class UStaticMeshComponent;
class AEONTileRoom;

UCLASS()
class EON_API AEONPedestal : public AActor
{
	GENERATED_BODY()

public:
	AEONPedestal();

	static AEONPedestal* Spawn(UWorld* World, AEONTileRoom* Room, FName Item, const FVector2D& Px, int32 Price, int32 Slot);
	static void ClearAll(UWorld* World);
	/** Socle le plus proche de Jira à moins de RangePx (fiche affichée par le HUD). */
	static AEONPedestal* Nearest(UWorld* World, const FVector2D& JiraPx, float RangePx);

	virtual void Tick(float DeltaSeconds) override;

	FName Item;
	int32 Price = 0;
	/** Indice dans la liste des socles de la case (pour la mémoire de la salle). */
	int32 Slot = 0;
	FVector2D Px = FVector2D::ZeroVector;

	/** Diffusé quand l'objet est pris : (socle, payant). */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTaken, AEONPedestal*, bool);
	static FOnTaken OnTaken;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Base;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gem;
	TWeakObjectPtr<AEONTileRoom> Room;
	float T = 0.f;
	float Cool = 0.f;
};
