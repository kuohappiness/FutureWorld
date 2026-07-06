#include "FWAICharacter.h"

#include "../AI/FWEnemyAIController.h"
#include "../Combat/FWHealthComponent.h"
#include "../State/FWStateMachineComponent.h"
#include "../Weapons/FWWeaponBase.h"
#include "../Weapons/FWMVPWeaponClasses.h"

namespace FWAICharacterStateTags
{
	const FName Idle(TEXT("State.AI.Idle"));
	const FName Dead(TEXT("State.AI.Dead"));
}

AFWAICharacter::AFWAICharacter()
{
	AIControllerClass = AFWEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	DefaultWeaponClass = AFWPistolWeapon::StaticClass();
}

void AFWAICharacter::BeginPlay()
{
	Super::BeginPlay();

	if (StateMachineComponent)
	{
		StateMachineComponent->ChangeState(FGameplayTag::RequestGameplayTag(FWAICharacterStateTags::Idle));
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AFWAICharacter::HandleDeath);
	}

	SpawnDefaultWeapon();
}

void AFWAICharacter::HandleDeath(AActor* DeadActor)
{
	if (DeadActor != this)
	{
		return;
	}

	if (StateMachineComponent)
	{
		StateMachineComponent->ChangeState(FGameplayTag::RequestGameplayTag(FWAICharacterStateTags::Dead));
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
}

void AFWAICharacter::SpawnDefaultWeapon()
{
	if (!DefaultWeaponClass || CurrentWeapon)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;

	if (AFWWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AFWWeaponBase>(DefaultWeaponClass, GetActorTransform(), SpawnParameters))
	{
		EquipWeapon(SpawnedWeapon);
	}
}
