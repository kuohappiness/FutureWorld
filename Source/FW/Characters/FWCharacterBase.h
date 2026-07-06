#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FWCharacterBase.generated.h"

class UFWHealthComponent;
class UFWStateMachineComponent;
class AFWWeaponBase;

UCLASS(Abstract)
class FW_API AFWCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AFWCharacterBase();

	UFUNCTION(BlueprintPure, Category = "FW|Components")
	UFWHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "FW|Components")
	UFWStateMachineComponent* GetStateMachineComponent() const { return StateMachineComponent; }

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	bool EquipWeapon(AFWWeaponBase* NewWeapon);

	UFUNCTION(BlueprintPure, Category = "FW|Weapon")
	AFWWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<UFWHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<UFWStateMachineComponent> StateMachineComponent;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Weapon")
	TObjectPtr<AFWWeaponBase> CurrentWeapon;
};
