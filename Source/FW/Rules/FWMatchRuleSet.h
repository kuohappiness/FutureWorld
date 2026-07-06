#pragma once

#include "CoreMinimal.h"
#include "../Assets/FWPrimaryDataAsset.h"
#include "FWMatchRuleSet.generated.h"

UCLASS(BlueprintType)
class FW_API UFWMatchRuleSet : public UFWPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules", meta = (ClampMin = "0"))
	int32 AIEnemyCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules", meta = (ClampMin = "1"))
	int32 PlayerLives = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules")
	bool bRespawnEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules", meta = (ClampMin = "0.0"))
	float RespawnDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules")
	bool bCoreVehicleEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules", meta = (ClampMin = "0"))
	int32 CoreVehiclePenalty = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules")
	bool bVehicleStealingEnabled = false;
};
