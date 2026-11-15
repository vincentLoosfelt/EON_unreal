// EON — mode de jeu. Les Blueprints BP_EONGameMode / BP_Jira / BP_EONPlayerController
// héritent de ces classes : c'est là qu'on branche meshes, anims et effets.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EONGameMode.generated.h"

UCLASS(Blueprintable)
class EON_API AEONGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEONGameMode();

	/** Jalon 1 : pose 4 cibles d'entraînement si la carte n'en contient aucune (L_TestRoom). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Test")
	bool bSpawnTrainingDummies = true;

protected:
	virtual void BeginPlay() override;
};
