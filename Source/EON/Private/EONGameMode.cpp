#include "EONGameMode.h"
#include "EONCharacter.h"
#include "EONPlayerController.h"

AEONGameMode::AEONGameMode()
{
	DefaultPawnClass = AEONCharacter::StaticClass();
	PlayerControllerClass = AEONPlayerController::StaticClass();
}
