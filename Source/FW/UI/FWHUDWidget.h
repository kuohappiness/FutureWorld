#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWHUDTypes.h"
#include "FWHUDWidget.generated.h"

class STextBlock;
class SVerticalBox;

UCLASS(Blueprintable)
class FW_API UFWHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FW|HUD")
	void SetHUDState(const FFWHUDStateSnapshot& NewHUDState);

	UFUNCTION(BlueprintPure, Category = "FW|HUD")
	const FFWHUDStateSnapshot& GetHUDState() const { return HUDState; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FFWHUDStateSnapshot HUDState;

	UFUNCTION(BlueprintImplementableEvent, Category = "FW|HUD")
	void OnHUDStateChanged();

private:
	TSharedPtr<STextBlock> HealthText;
	TSharedPtr<STextBlock> LivesText;
	TSharedPtr<STextBlock> WeaponText;
	TSharedPtr<STextBlock> VehicleText;
	TSharedPtr<STextBlock> CoreVehicleText;
	TSharedPtr<STextBlock> StateText;
	TSharedPtr<STextBlock> MovementText;
	TSharedPtr<STextBlock> AIText;

	void UpdateHUDText();
	static FText FormatHealthLine(const TCHAR* Label, float CurrentValue, float MaxValue);
};
