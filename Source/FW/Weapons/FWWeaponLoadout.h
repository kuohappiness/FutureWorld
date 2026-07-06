#pragma once

#include "CoreMinimal.h"
#include "../Assets/FWPrimaryDataAsset.h"
#include "FWWeaponLoadout.generated.h"

class UFWWeaponData;

UCLASS(BlueprintType)
class FW_API UFWWeaponLoadout : public UFWPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Loadout")
	TObjectPtr<UFWWeaponData> Pistol;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Loadout")
	TObjectPtr<UFWWeaponData> Rifle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Loadout")
	TObjectPtr<UFWWeaponData> Sniper;

	UFUNCTION(BlueprintPure, Category = "FW|Loadout")
	TArray<FName> GetSelectedContentIds() const;
};
