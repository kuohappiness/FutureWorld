#include "FWCompetitiveGameMode.h"

#include "../Events/FWEventSubsystem.h"
#include "../Rules/FWMatchRuleSet.h"
#include "../State/FWStateMachineComponent.h"
#include "../Vehicles/FWVehicleBase.h"
#include "FWPlayerState.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace FWMatchStateTags
{
	const FName WaitingForStart(TEXT("Match.WaitingForStart"));
	const FName Spawning(TEXT("Match.Spawning"));
	const FName InProgress(TEXT("Match.InProgress"));
	const FName Ending(TEXT("Match.Ending"));
	const FName Ended(TEXT("Match.Ended"));
}

namespace FWRuleEventTags
{
	const FName CoreVehicleDestroyed(TEXT("Event.Vehicle.CoreDestroyed"));
	const FName RespawnCompleted(TEXT("Event.Respawn.Completed"));
}

AFWCompetitiveGameMode::AFWCompetitiveGameMode()
{
	PlayerStateClass = AFWPlayerState::StaticClass();
}

void AFWCompetitiveGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!CurrentMatchStateTag.IsValid())
	{
		SetMatchStateTag(FWMatchStateTags::WaitingForStart);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>())
		{
			EventSubsystem->OnGameplayEvent.AddDynamic(this, &AFWCompetitiveGameMode::HandleGameplayEvent);
		}
	}
}

void AFWCompetitiveGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	InitializePlayerState(NewPlayer);
}

void AFWCompetitiveGameMode::StartMatchFlow()
{
	SetMatchStateTag(FWMatchStateTags::InProgress);
}

void AFWCompetitiveGameMode::EndMatchFlow()
{
	SetMatchStateTag(FWMatchStateTags::Ending);
	SetMatchStateTag(FWMatchStateTags::Ended);
}

void AFWCompetitiveGameMode::SetMatchRuleSet(UFWMatchRuleSet* NewMatchRuleSet)
{
	if (CurrentMatchStateTag == FGameplayTag::RequestGameplayTag(FWMatchStateTags::InProgress))
	{
		return;
	}

	MatchRuleSet = NewMatchRuleSet;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		InitializePlayerState(It->Get());
	}
}

void AFWCompetitiveGameMode::RequestRespawn(AController* RespawnController)
{
	if (!RespawnController || !IsRespawnEnabled() || CurrentMatchStateTag == FGameplayTag::RequestGameplayTag(FWMatchStateTags::Ended))
	{
		return;
	}

	SetMatchStateTag(FWMatchStateTags::Spawning);

	const float Delay = GetConfiguredRespawnDelay();
	if (Delay <= 0.0f)
	{
		FinishRespawn(RespawnController);
	}
	else
	{
		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindUObject(this, &AFWCompetitiveGameMode::FinishRespawn, RespawnController);
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, Delay, false);
	}
}

void AFWCompetitiveGameMode::HandleCoreVehicleDestroyed(AController* OwnerController)
{
	if (!OwnerController || !IsCoreVehicleRuleEnabled() || !CanApplyMatchResults())
	{
		return;
	}

	AFWPlayerState* FWPlayerState = OwnerController->GetPlayerState<AFWPlayerState>();
	if (!FWPlayerState)
	{
		return;
	}

	const int32 Penalty = GetCoreVehiclePenalty();
	FWPlayerState->ApplyLifeDelta(-Penalty);

	if (FWPlayerState->Lives <= 0)
	{
		EndMatchFlow();
	}
	else
	{
		RequestRespawn(OwnerController);
	}
}

void AFWCompetitiveGameMode::HandleGameplayEvent(const FFWGameplayEvent& Event)
{
	if (Event.EventTag != FGameplayTag::RequestGameplayTag(FWRuleEventTags::CoreVehicleDestroyed))
	{
		return;
	}

	const AFWVehicleBase* DestroyedVehicle = Cast<AFWVehicleBase>(Event.Target);
	HandleCoreVehicleDestroyed(DestroyedVehicle ? DestroyedVehicle->GetOwnerController() : nullptr);
}

void AFWCompetitiveGameMode::SetMatchStateTag(FName NewStateTagName)
{
	CurrentMatchStateTag = FGameplayTag::RequestGameplayTag(NewStateTagName);
}

void AFWCompetitiveGameMode::InitializePlayerState(AController* Controller) const
{
	if (!Controller)
	{
		return;
	}

	if (AFWPlayerState* FWPlayerState = Controller->GetPlayerState<AFWPlayerState>())
	{
		FWPlayerState->SetLives(GetConfiguredPlayerLives());
	}
}

bool AFWCompetitiveGameMode::CanApplyMatchResults() const
{
	return CurrentMatchStateTag == FGameplayTag::RequestGameplayTag(FWMatchStateTags::InProgress);
}

int32 AFWCompetitiveGameMode::GetConfiguredPlayerLives() const
{
	return MatchRuleSet ? FMath::Max(1, MatchRuleSet->PlayerLives) : 3;
}

float AFWCompetitiveGameMode::GetConfiguredRespawnDelay() const
{
	return MatchRuleSet ? FMath::Max(0.0f, MatchRuleSet->RespawnDelay) : 3.0f;
}

bool AFWCompetitiveGameMode::IsRespawnEnabled() const
{
	return MatchRuleSet ? MatchRuleSet->bRespawnEnabled : true;
}

bool AFWCompetitiveGameMode::IsCoreVehicleRuleEnabled() const
{
	return MatchRuleSet ? MatchRuleSet->bCoreVehicleEnabled : true;
}

int32 AFWCompetitiveGameMode::GetCoreVehiclePenalty() const
{
	return MatchRuleSet ? FMath::Max(0, MatchRuleSet->CoreVehiclePenalty) : 1;
}

void AFWCompetitiveGameMode::FinishRespawn(AController* RespawnController)
{
	if (!RespawnController || CurrentMatchStateTag == FGameplayTag::RequestGameplayTag(FWMatchStateTags::Ended))
	{
		return;
	}

	RestartPlayer(RespawnController);
	BroadcastRespawnCompleted(RespawnController);
	SetMatchStateTag(FWMatchStateTags::InProgress);
}

void AFWCompetitiveGameMode::BroadcastRespawnCompleted(AController* RespawnController) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>();
	if (!EventSubsystem)
	{
		return;
	}

	FFWGameplayEvent Event;
	Event.EventTag = FGameplayTag::RequestGameplayTag(FWRuleEventTags::RespawnCompleted);
	Event.InstigatorActor = RespawnController ? RespawnController->GetPawn() : nullptr;
	Event.Target = RespawnController ? RespawnController->GetPawn() : nullptr;
	Event.Magnitude = 1.0f;
	Event.WorldLocation = RespawnController && RespawnController->GetPawn() ? RespawnController->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	EventSubsystem->BroadcastGameplayEvent(Event);
}
