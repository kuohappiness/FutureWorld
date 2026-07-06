#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FWInteractable.generated.h"

UINTERFACE(BlueprintType)
class FW_API UFWInteractable : public UInterface
{
	GENERATED_BODY()
};

class FW_API IFWInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FW|Interaction")
	void Interact(AActor* InteractingActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FW|Interaction")
	FText GetInteractionText() const;
};
