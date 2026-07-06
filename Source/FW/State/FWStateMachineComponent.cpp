#include "FWStateMachineComponent.h"

UFWStateMachineComponent::UFWStateMachineComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFWStateMachineComponent::ChangeState(FGameplayTag NewState)
{
	if (!NewState.IsValid() || CurrentState == NewState)
	{
		return false;
	}

	PreviousState = CurrentState;
	CurrentState = NewState;
	OnStateChanged.Broadcast(PreviousState, CurrentState);
	return true;
}
