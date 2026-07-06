#include "FWHealthComponent.h"

UFWHealthComponent::UFWHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFWHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetHealth();
}

float UFWHealthComponent::ApplyDamage(float DamageAmount)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return CurrentHealth;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - PreviousHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast(GetOwner());
	}

	return CurrentHealth;
}

FFWDamageResult UFWHealthComponent::ApplyDamageRequest(const FFWDamageRequest& DamageRequest)
{
	FFWDamageResult DamageResult;
	DamageResult.DamagedActor = GetOwner();

	if (bIsDead)
	{
		DamageResult.RemainingHealth = CurrentHealth;
		DamageResult.bKilled = false;
		return DamageResult;
	}

	const float AppliedDamage = FMath::Max(0.0f, DamageRequest.BaseDamage * DamageRequest.DamageMultiplier);
	const float PreviousHealth = CurrentHealth;
	const bool bWasAlive = !bIsDead;
	ApplyDamage(AppliedDamage);

	DamageResult.AppliedDamage = PreviousHealth - CurrentHealth;
	DamageResult.HitZoneName = DamageRequest.HitZoneName;
	DamageResult.DamageMultiplier = DamageRequest.DamageMultiplier;
	DamageResult.RemainingHealth = CurrentHealth;
	DamageResult.bKilled = bWasAlive && bIsDead;
	return DamageResult;
}

void UFWHealthComponent::ResetHealth()
{
	bIsDead = false;
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, 0.0f);
}
