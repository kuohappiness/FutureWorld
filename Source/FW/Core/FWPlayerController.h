#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/PlayerController.h"
#include "../UI/FWHUDTypes.h"
#include "FWPlayerController.generated.h"

class UFWHUDWidget;
class AFWAICharacter;
class AFWVehicleBase;

UCLASS()
class FW_API AFWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFWPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintPure, Category = "FW|HUD")
	FFWHUDStateSnapshot BuildHUDStateSnapshot() const;

	UFUNCTION(BlueprintCallable, Category = "FW|HUD")
	void ForceRefreshHUD();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|HUD")
	TSubclassOf<UFWHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	TObjectPtr<UFWHUDWidget> HUDWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|HUD", meta = (ClampMin = "0.0"))
	float HUDRefreshInterval = 0.1f;

private:
	float LastHUDRefreshTime = -FLT_MAX;
	FString LastCombatDebugText = TEXT("None");
	FTimerHandle VehicleInteractionEvidenceTimerHandle;
	int32 VehicleInteractionEvidenceStep = 0;
	bool bVehicleInteractionEvidenceActive = false;
	double VehicleInteractionEvidenceStartTime = 0.0;
	FTSTicker::FDelegateHandle VehicleInteractionEvidenceTickerHandle;

	void CreateHUDWidget();
	void RefreshHUDWidget();
	AFWAICharacter* FindNearestAICharacter() const;
	AFWVehicleBase* FindNearestVehicle() const;
	void HandleVehicleExitKeyFallback();
	void StartVehicleInteractionEvidenceSequence();
	void AdvanceVehicleInteractionEvidenceSequence();
	void TickVehicleInteractionEvidenceSequence();
	void SimulateInteractKeyForEvidence();
	void DebugFireNearestAI();
	void DebugFireNearestVehicle();
};
