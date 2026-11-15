// EON — porte « Sas » d'Unity (ColoredDoor, type Sas + RoomEnemyDoorController) : un cadre fixe et deux vantaux qui
// glissent chacun de 1 m dans le mur, à vitesse constante (MoveTowards, 2 m/s). Ouverte par défaut ; fermée tant
// qu'un ennemi vivant se trouve dans la salle (rayon autour de la porte), rouverte quand la salle est vidée.
// Les meshes viennent de eon_porte_decoupe.py (porte.fbx découpé en cadre / vantail gauche / vantail droit).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONSlidingDoor.generated.h"

class UStaticMeshComponent;

UCLASS()
class EON_API AEONSlidingDoor : public AActor
{
	GENERATED_BODY()

public:
	AEONSlidingDoor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, Category = "Porte") TObjectPtr<UStaticMeshComponent> Cadre;
	UPROPERTY(VisibleAnywhere, Category = "Porte") TObjectPtr<UStaticMeshComponent> VantailG;
	UPROPERTY(VisibleAnywhere, Category = "Porte") TObjectPtr<UStaticMeshComponent> VantailD;

	/** Course de chaque vantail le long de son X local (Unity : openDistance 1 m). */
	UPROPERTY(EditAnywhere, Category = "Porte") float OpenDistance = 100.f;
	/** Vitesse constante en cm/s (Unity : openSpeed 2 m/s). */
	UPROPERTY(EditAnywhere, Category = "Porte") float OpenSpeed = 200.f;
	/** Un ennemi vivant à moins de ce rayon ferme la porte (Unity : déclencheur de salle 30 × 30 m). */
	UPROPERTY(EditAnywhere, Category = "Porte") float RoomRadius = 1500.f;
	/** Intervalle entre deux vérifications des ennemis (Unity : checkInterval 0,5 s). */
	UPROPERTY(EditAnywhere, Category = "Porte") float CheckInterval = 0.5f;

	bool IsOpen() const { return Opening >= 1.f; }
	/** Forçage par le jeu : -1 = comportement normal, 0 = fermée (C.O.M à quai), 1 = ouverte. */
	int32 ForceState = -1;

private:
	/** 0 = fermée, 1 = ouverte (course parcourue / OpenDistance). */
	float Opening = 1.f;
	bool bWantOpen = true;
	float NextCheck = 0.f;
	void ApplyOpening();
	bool EnemyInRoom() const;
};
