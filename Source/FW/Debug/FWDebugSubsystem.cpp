#include "FWDebugSubsystem.h"

#include "../Combat/FWHealthComponent.h"
#include "../Combat/FWHitZoneComponent.h"
#include "../Events/FWEventSubsystem.h"
#include "Engine/Engine.h"
#include "../State/FWStateMachineComponent.h"

void UFWDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UFWEventSubsystem>();
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	if (UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>())
	{
		EventSubsystem->OnGameplayEvent.AddDynamic(this, &UFWDebugSubsystem::HandleGameplayEvent);
	}
}

void UFWDebugSubsystem::SetDebugEnabled(bool bEnabled)
{
	bDebugEnabled = bEnabled;
}

void UFWDebugSubsystem::PrintActorDebugState(AActor* Actor, float Duration) const
{
	if (!bDebugEnabled || !Actor || !GEngine)
	{
		return;
	}

	const UFWStateMachineComponent* StateMachine = Actor->FindComponentByClass<UFWStateMachineComponent>();
	const UFWHealthComponent* Health = Actor->FindComponentByClass<UFWHealthComponent>();
	const UFWHitZoneComponent* HitZone = Actor->FindComponentByClass<UFWHitZoneComponent>();

	const FString StateText = StateMachine ? StateMachine->CurrentState.ToString() : TEXT("NoState");
	const FString HealthText = Health ? FString::Printf(TEXT("%.0f/%.0f"), Health->CurrentHealth, Health->MaxHealth) : TEXT("NoHealth");
	const FString HitZoneText = HitZone ? FString::Printf(TEXT("%s x%.2f"), *HitZone->HitZoneName.ToString(), HitZone->DamageMultiplier) : TEXT("NoHitZone");

	GEngine->AddOnScreenDebugMessage(
		-1,
		Duration,
		FColor::Green,
		FString::Printf(TEXT("%s | State=%s | Health=%s | HitZone=%s"), *Actor->GetName(), *StateText, *HealthText, *HitZoneText));
}

void UFWDebugSubsystem::ClearRecentEvents()
{
	RecentEvents.Reset();
}

void UFWDebugSubsystem::RecordGameplayEvent(const FFWGameplayEvent& Event)
{
	FFWDebugEventRecord Record;
	Record.EventTag = Event.EventTag;
	Record.InstigatorActor = Event.InstigatorActor;
	Record.Target = Event.Target;
	Record.Magnitude = Event.Magnitude;
	Record.WorldLocation = Event.WorldLocation;

	RecentEvents.Add(Record);
	while (RecentEvents.Num() > MaxRecentEvents)
	{
		RecentEvents.RemoveAt(0, 1, EAllowShrinking::No);
	}

	if (bDebugEnabled && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			FString::Printf(TEXT("FW Event: %s %.2f"), *Event.EventTag.ToString(), Event.Magnitude));
	}
}

void UFWDebugSubsystem::HandleGameplayEvent(const FFWGameplayEvent& Event)
{
	RecordGameplayEvent(Event);
}
