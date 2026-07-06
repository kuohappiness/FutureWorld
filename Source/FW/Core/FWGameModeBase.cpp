#include "FWGameModeBase.h"

#include "../Characters/FWPlayerCharacter.h"
#include "FWPlayerController.h"

AFWGameModeBase::AFWGameModeBase()
{
	DefaultPawnClass = AFWPlayerCharacter::StaticClass();
	PlayerControllerClass = AFWPlayerController::StaticClass();
}
