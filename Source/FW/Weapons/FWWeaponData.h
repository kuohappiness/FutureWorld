#pragma once

#include "CoreMinimal.h"
#include "../Assets/FWPrimaryDataAsset.h"
#include "FWWeaponData.generated.h"

UENUM(BlueprintType)
enum class EFWWeaponType : uint8
{
	Pistol,
	Rifle,
	Sniper
};

UCLASS(BlueprintType)
class FW_API UFWWeaponData : public UFWPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon")
	EFWWeaponType WeaponType = EFWWeaponType::Pistol;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "0.0"))
	float BaseDamage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "0.01"))
	float FireRate = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "1"))
	int32 MagazineSize = 12;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "0.0"))
	float ReloadTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "0.0"))
	float Range = 10000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "0.0"))
	float VehicleDamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon", meta = (ClampMin = "0.0"))
	float WeakPointMultiplier = 1.5f;
};
