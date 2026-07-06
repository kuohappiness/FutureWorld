#pragma once

#include "CoreMinimal.h"
#include "FWCharacterBase.h"
#include "FWAICharacter.generated.h"

class AFWWeaponBase;

UCLASS()
class FW_API AFWAICharacter : public AFWCharacterBase
{
	GENERATED_BODY()

public:
	AFWAICharacter();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|AI")
	TSubclassOf<AFWWeaponBase> DefaultWeaponClass;

private:
	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	void SpawnDefaultWeapon();
};
