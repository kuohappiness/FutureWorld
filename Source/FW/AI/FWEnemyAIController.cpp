#include "FWEnemyAIController.h"

#include "../Characters/FWCharacterBase.h"
#include "../Combat/FWHealthComponent.h"
#include "../Events/FWEventSubsystem.h"
#include "../State/FWStateMachineComponent.h"
#include "../Weapons/FWWeaponBase.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

namespace FWAIControllerStateTags
{
	const FName Idle(TEXT("State.AI.Idle"));
	const FName Patrol(TEXT("State.AI.Patrol"));
	const FName Alert(TEXT("State.AI.Alert"));
	const FName AttackOnFoot(TEXT("State.AI.AttackOnFoot"));
	const FName Dead(TEXT("State.AI.Dead"));
}

namespace FWAIEventTags
{
	const FName StateChanged(TEXT("Event.AI.StateChanged"));
}

AFWEnemyAIController::AFWEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFWEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RunCombatBehavior(DeltaSeconds);
}

void AFWEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	UpdateAIState(FWAIControllerStateTags::Idle);
}

AActor* AFWEnemyAIController::FindBestTarget() const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(DetectionRadius);

	for (TActorIterator<AFWCharacterBase> It(GetWorld()); It; ++It)
	{
		AFWCharacterBase* Candidate = *It;
		if (!Candidate || Candidate == ControlledPawn)
		{
			continue;
		}

		if (const UFWHealthComponent* CandidateHealth = Candidate->GetHealthComponent())
		{
			if (CandidateHealth->IsDead())
			{
				continue;
			}
		}

		if (Candidate->GetController() && Candidate->GetController()->IsPlayerController())
		{
			const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestTarget = Candidate;
			}
		}
	}

	return BestTarget;
}

void AFWEnemyAIController::UpdateAIState(FName StateTagName) const
{
	const AFWCharacterBase* FWCharacter = Cast<AFWCharacterBase>(GetPawn());
	if (!FWCharacter)
	{
		return;
	}

	if (UFWStateMachineComponent* StateMachine = FWCharacter->GetStateMachineComponent())
	{
		const FGameplayTag NewState = FGameplayTag::RequestGameplayTag(StateTagName);
		if (StateMachine->ChangeState(NewState))
		{
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>())
				{
					FFWGameplayEvent Event;
					Event.EventTag = FGameplayTag::RequestGameplayTag(FWAIEventTags::StateChanged);
					Event.InstigatorActor = GetPawn();
					Event.Target = GetPawn();
					Event.Magnitude = 0.0f;
					Event.WorldLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
					EventSubsystem->BroadcastGameplayEvent(Event);
				}
			}
		}
	}
}

void AFWEnemyAIController::RunCombatBehavior(float DeltaSeconds)
{
	if (IsControlledPawnDead())
	{
		UpdateAIState(FWAIControllerStateTags::Dead);
		StopMovement();
		return;
	}

	AFWCharacterBase* FWCharacter = Cast<AFWCharacterBase>(GetPawn());
	if (!FWCharacter)
	{
		return;
	}

	CurrentTarget = FindBestTarget();
	if (!CurrentTarget)
	{
		UpdateAIState(FWAIControllerStateTags::Patrol);
		StopMovement();
		return;
	}

	const float DistanceToTarget = FVector::Dist(FWCharacter->GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistanceToTarget > AttackRange)
	{
		UpdateAIState(FWAIControllerStateTags::Alert);
		MoveToActor(CurrentTarget, AttackRange * 0.75f);
		return;
	}

	StopMovement();
	UpdateAIState(FWAIControllerStateTags::AttackOnFoot);
	FWCharacter->SetActorRotation((CurrentTarget->GetActorLocation() - FWCharacter->GetActorLocation()).Rotation());

	if (AFWWeaponBase* CurrentWeapon = FWCharacter->GetCurrentWeapon())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if ((Now - LastAttackTime) >= AttackInterval && FMath::FRand() <= Accuracy)
		{
			CurrentWeapon->FireAtActor(CurrentTarget);
			LastAttackTime = Now;
		}
	}
}

bool AFWEnemyAIController::IsControlledPawnDead() const
{
	const AFWCharacterBase* FWCharacter = Cast<AFWCharacterBase>(GetPawn());
	if (!FWCharacter)
	{
		return false;
	}

	const UFWHealthComponent* HealthComponent = FWCharacter->GetHealthComponent();
	return HealthComponent && HealthComponent->IsDead();
}
