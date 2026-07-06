#include "FWPlayerState.h"

#include "../Vehicles/FWVehicleBase.h"

void AFWPlayerState::SetLives(int32 NewLives)
{
	const int32 PreviousLives = Lives;
	Lives = FMath::Max(0, NewLives);
	OnLivesChanged.Broadcast(Lives, Lives - PreviousLives);
}

int32 AFWPlayerState::ApplyLifeDelta(int32 Delta)
{
	SetLives(Lives + Delta);
	return Lives;
}

void AFWPlayerState::SetCoreVehicle(AFWVehicleBase* NewCoreVehicle)
{
	CoreVehicle = NewCoreVehicle;
	if (CoreVehicle)
	{
		CoreVehicle->SetOwnerController(GetOwner<AController>());
		CoreVehicle->SetCoreVehicle(true);
	}
}
