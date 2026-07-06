#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FWPlayerState.generated.h"

class AFWVehicleBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWLivesChangedSignature, int32, Lives, int32, Delta);

UCLASS()
class FW_API AFWPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "FW|Match")
	FFWLivesChangedSignature OnLivesChanged;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Match")
	int32 Lives = 3;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Team")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Vehicle")
	TObjectPtr<AFWVehicleBase> CoreVehicle;

	UFUNCTION(BlueprintCallable, Category = "FW|Match")
	void SetLives(int32 NewLives);

	UFUNCTION(BlueprintCallable, Category = "FW|Match")
	int32 ApplyLifeDelta(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	void SetCoreVehicle(AFWVehicleBase* NewCoreVehicle);
};
