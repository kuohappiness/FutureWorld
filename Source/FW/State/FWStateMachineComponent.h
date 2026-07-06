#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FWStateMachineComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWStateChangedSignature, FGameplayTag, PreviousState, FGameplayTag, NewState);

UCLASS(ClassGroup = (FW), meta = (BlueprintSpawnableComponent))
class FW_API UFWStateMachineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFWStateMachineComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|State")
	FGameplayTag CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "FW|State")
	FGameplayTag PreviousState;

	UPROPERTY(BlueprintAssignable, Category = "FW|State")
	FFWStateChangedSignature OnStateChanged;

	UFUNCTION(BlueprintCallable, Category = "FW|State")
	bool ChangeState(FGameplayTag NewState);
};
