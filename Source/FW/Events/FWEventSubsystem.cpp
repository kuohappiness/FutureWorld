#include "FWEventSubsystem.h"

#include "Engine/Engine.h"

void UFWEventSubsystem::BroadcastGameplayEvent(const FFWGameplayEvent& Event)
{
	OnGameplayEvent.Broadcast(Event);

	if (bPrintEventsToScreen && GEngine)
	{
		const FString EventName = Event.EventTag.IsValid() ? Event.EventTag.ToString() : TEXT("None");
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			FString::Printf(TEXT("FW Event: %s"), *EventName));
	}
}
