#include "EONGameMode.h"
#include "EONCharacter.h"
#include "EONPlayerController.h"
#include "EONHUD.h"
#include "EONFloorManager.h"
#include "EONCombatDirector.h"
#include "EONTrainingDummy.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AEONGameMode::AEONGameMode()
{
	DefaultPawnClass = AEONCharacter::StaticClass();
	PlayerControllerClass = AEONPlayerController::StaticClass();
	HUDClass = AEONHUD::StaticClass();
}

void AEONGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	AEONCombatDirector::Get(World);   // vagues, salles Combat, mort de Jira (jalon 3)
	if (!bSpawnTrainingDummies || !World) { return; }
	// Carte d'étage : pas de cibles d'entraînement, ce sont les vagues qui peuplent les salles.
	for (TActorIterator<AEONFloorManager> It(World); It; ++It) { return; }

	// Si le niveau contient déjà des cibles posées à la main, on n'y touche pas.
	for (TActorIterator<AEONTrainingDummy> It(World); It; ++It) { return; }

	// Salle de test 32 × 20 m centrée sur l'origine, Jira au centre. Quatre cibles devant et autour,
	// dans l'esprit des quatre « dummy » du réveil du Kibo (buildTraining) : trois proches, une loin.
	static const FVector2D Spots[] = {
		FVector2D(500.f, -300.f), FVector2D(900.f, -500.f), FVector2D(1300.f, 0.f), FVector2D(-900.f, 400.f)
	};
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FVector2D& S : Spots)
	{
		World->SpawnActor<AEONTrainingDummy>(AEONTrainingDummy::StaticClass(), FVector(S.X, S.Y, 0.f), FRotator::ZeroRotator, Params);
	}
}
