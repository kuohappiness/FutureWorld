#pragma once

#include "CoreMinimal.h"
#include "FWVehicleBase.h"
#include "FWMVPVehicleClasses.generated.h"

UCLASS(Abstract)
class FW_API AFWMVPVehicleBase : public AFWVehicleBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	FName DefaultVehicleDataPath;

	void SetDefaultVehicleDataPath(FName AssetPath);
};

UCLASS()
class FW_API AFWCarVehicle : public AFWMVPVehicleBase
{
	GENERATED_BODY()

public:
	AFWCarVehicle();
};

UCLASS()
class FW_API AFWMotorcycleVehicle : public AFWMVPVehicleBase
{
	GENERATED_BODY()

public:
	AFWMotorcycleVehicle();
};
