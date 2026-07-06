#pragma once

#include "CoreMinimal.h"
#include "FWWeaponBase.h"
#include "FWMVPWeaponClasses.generated.h"

UCLASS(Abstract)
class FW_API AFWMVPWeaponBase : public AFWWeaponBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	FName DefaultWeaponDataPath;

	void SetDefaultWeaponDataPath(FName AssetPath);
};

UCLASS()
class FW_API AFWPistolWeapon : public AFWMVPWeaponBase
{
	GENERATED_BODY()

public:
	AFWPistolWeapon();
};

UCLASS()
class FW_API AFWRifleWeapon : public AFWMVPWeaponBase
{
	GENERATED_BODY()

public:
	AFWRifleWeapon();
};

UCLASS()
class FW_API AFWSniperWeapon : public AFWMVPWeaponBase
{
	GENERATED_BODY()

public:
	AFWSniperWeapon();
};
