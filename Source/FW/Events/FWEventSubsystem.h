#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FWGameplayEvent.h"
#include "FWEventSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWGameplayEventSignature, const FFWGameplayEvent&, Event);

UCLASS()
class FW_API UFWEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "FW|Events")
	FFWGameplayEventSignature OnGameplayEvent;

	UFUNCTION(BlueprintCallable, Category = "FW|Events")
	void BroadcastGameplayEvent(const FFWGameplayEvent& Event);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Debug")
	bool bPrintEventsToScreen = false;
};
