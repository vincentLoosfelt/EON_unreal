// EON — jalon 6 : ramassables (pickups du proto) : gemmes d'ADN aspirées, scrap, énergie, cœur.
// Position et vitesse en px de salle, comme le proto ; le visuel suit la salle en cours.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONPickup.generated.h"

class UStaticMeshComponent;
class AEONTileRoom;

UENUM(BlueprintType)
enum class EEONPickup : uint8
{
	Adn,
	Scrap,
	Energy,
	Heart
};

UCLASS()
class EON_API AEONPickup : public AActor
{
	GENERATED_BODY()

public:
	AEONPickup();

	/** Pose un ramassable dans la salle (px de salle) avec une vitesse de départ (px/s). */
	static AEONPickup* Spawn(UWorld* World, AEONTileRoom* Room, EEONPickup Type, const FVector2D& Px, const FVector2D& VelPx, float V = 1.f, float T0 = 0.f);
	/** dropADN(x, y, v) : paquets de 3 au plus, éjectés au hasard. */
	static void DropADN(UWorld* World, AEONTileRoom* Room, const FVector2D& Px, int32 V);
	/** Reste-t-il de l'ADN au sol ? (le choix de niveau attend qu'il soit ramassé) */
	static bool AnyAdn(UWorld* World);
	/** Changement de salle : l'ADN encore en vol est crédité, le reste disparaît. */
	static void ClearAll(UWorld* World, bool bCreditAdn);

	virtual void Tick(float DeltaSeconds) override;

	EEONPickup Type = EEONPickup::Adn;
	float Value = 1.f;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	TWeakObjectPtr<AEONTileRoom> Room;
	FVector2D Px = FVector2D::ZeroVector;
	FVector2D Vel = FVector2D::ZeroVector;
	float T = 0.f;
};
