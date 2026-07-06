#include "FWMVPVehicleClasses.h"

#include "FWVehicleData.h"

void AFWMVPVehicleBase::BeginPlay()
{
	if (!VehicleData && !DefaultVehicleDataPath.IsNone())
	{
		VehicleData = LoadObject<UFWVehicleData>(nullptr, *DefaultVehicleDataPath.ToString());
	}

	Super::BeginPlay();
}

void AFWMVPVehicleBase::SetDefaultVehicleDataPath(FName AssetPath)
{
	DefaultVehicleDataPath = AssetPath;
	if (!DefaultVehicleDataPath.IsNone())
	{
		VehicleData = LoadObject<UFWVehicleData>(nullptr, *DefaultVehicleDataPath.ToString());
	}
}

AFWCarVehicle::AFWCarVehicle()
{
	SetDefaultVehicleDataPath(TEXT("/Game/FW/Vehicles/Data/DA_Vehicle_Car_Balanced.DA_Vehicle_Car_Balanced"));
}

AFWMotorcycleVehicle::AFWMotorcycleVehicle()
{
	SetDefaultVehicleDataPath(TEXT("/Game/FW/Vehicles/Data/DA_Vehicle_Motorcycle_Fast.DA_Vehicle_Motorcycle_Fast"));
}
