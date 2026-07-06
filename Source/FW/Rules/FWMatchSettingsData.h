#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FWMatchSettingsData.generated.h"

class UFWMatchRuleSet;

UCLASS(BlueprintType)
class FW_API UFWMatchSettingsData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Settings", meta = (ClampMin = "0"))
	int32 AIEnemyCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Difficulty = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Settings", meta = (ClampMin = "1"))
	int32 PlayerLives = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Settings")
	bool bRespawnEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Settings")
	bool bCoreVehicleEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Settings", meta = (ClampMin = "0"))
	int32 CoreVehiclePenalty = 1;

	UFUNCTION(BlueprintCallable, Category = "FW|Settings")
	void CopyFromRuleSet(const UFWMatchRuleSet* RuleSet);

	UFUNCTION(BlueprintCallable, Category = "FW|Settings")
	void ApplyToRuleSet(UFWMatchRuleSet* RuleSet) const;

	UFUNCTION(BlueprintCallable, Category = "FW|Settings")
	UFWMatchRuleSet* CreateRuntimeRuleSet(UObject* Outer) const;
};
