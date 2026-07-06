#pragma once

#include "CoreMinimal.h"
#include "../Assets/FWPrimaryDataAsset.h"
#include "FWVehicleData.generated.h"

UENUM(BlueprintType)
enum class EFWVehicleType : uint8
{
	Car,
	Motorcycle
};

UCLASS(BlueprintType)
class FW_API UFWVehicleData : public UFWPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle")
	EFWVehicleType VehicleType = EFWVehicleType::Car;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle", meta = (ClampMin = "1.0"))
	float MaxHealth = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle", meta = (ClampMin = "0.0"))
	float Armor = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle", meta = (ClampMin = "0.0"))
	float MaxSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle", meta = (ClampMin = "0.0"))
	float Acceleration = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle", meta = (ClampMin = "0.0"))
	float Handling = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle", meta = (ClampMin = "1"))
	int32 SeatCount = 1;
};
