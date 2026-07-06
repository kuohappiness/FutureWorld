#include "FWCharacterBase.h"

#include "../Combat/FWHealthComponent.h"
#include "../State/FWStateMachineComponent.h"
#include "../Weapons/FWWeaponBase.h"

AFWCharacterBase::AFWCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthComponent = CreateDefaultSubobject<UFWHealthComponent>(TEXT("HealthComponent"));
	StateMachineComponent = CreateDefaultSubobject<UFWStateMachineComponent>(TEXT("StateMachineComponent"));
}

bool AFWCharacterBase::EquipWeapon(AFWWeaponBase* NewWeapon)
{
	if (!NewWeapon)
	{
		return false;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->Unequip();
	}

	CurrentWeapon = NewWeapon;
	CurrentWeapon->Equip(this);
	return true;
}
