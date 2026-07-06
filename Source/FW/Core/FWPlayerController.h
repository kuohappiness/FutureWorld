#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/PlayerController.h"
#include "../UI/FWHUDTypes.h"
#include "FWPlayerController.generated.h"

class UFWHUDWidget;
class AFWAICharacter;
class AFWVehicleBase;
struct FFWGameplayEvent;

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
	int32 VehicleInteractionEvidenceStep = 0;
	bool bVehicleInteractionEvidenceActive = false;
	double VehicleInteractionEvidenceStartTime = 0.0;
	FTSTicker::FDelegateHandle VehicleInteractionEvidenceTickerHandle;
	FString VehicleInteractionEvidenceFolder;
	FString VehicleInteractionEvidencePrefix = TEXT("FW_MVP_PIE_VehicleInteraction");
	FString VehicleInteractionEvidenceTarget = TEXT("Car");
	int32 VehicleDriveEvidenceStep = 0;
	bool bVehicleDriveEvidenceActive = false;
	double VehicleDriveEvidenceStartTime = 0.0;
	FTSTicker::FDelegateHandle VehicleDriveEvidenceTickerHandle;
	FString VehicleDriveEvidenceFolder;
	FString VehicleDriveEvidencePrefix = TEXT("FW_MVP_PIE_VehicleDrive");
	FString VehicleDriveEvidenceTarget = TEXT("Car");
	FVector VehicleDriveEvidenceStartLocation = FVector::ZeroVector;
	FRotator VehicleDriveEvidenceStartRotation = FRotator::ZeroRotator;
	int32 VehicleDestructionEvidenceStep = 0;
	bool bVehicleDestructionEvidenceActive = false;
	double VehicleDestructionEvidenceStartTime = 0.0;
	FTSTicker::FDelegateHandle VehicleDestructionEvidenceTickerHandle;
	FString VehicleDestructionEvidenceFolder;
	FString VehicleDestructionEvidencePrefix = TEXT("FW_MVP_PIE_VehicleDestruction");
	FString VehicleDestructionEvidenceTarget = TEXT("Car");
	TObjectPtr<AFWVehicleBase> VehicleDestructionEvidenceTargetActor;
	int32 VehicleDestructionEvidenceDelegateCount = 0;
	int32 VehicleDestructionEvidenceGameplayEventCount = 0;

	void CreateHUDWidget();
	void RefreshHUDWidget();
	AFWAICharacter* FindNearestAICharacter() const;
	AFWVehicleBase* FindNearestVehicle() const;
	void HandleVehicleExitKeyFallback();
	void StartVehicleInteractionEvidenceSequence();
	void AdvanceVehicleInteractionEvidenceSequence();
	void TickVehicleInteractionEvidenceSequence();
	void TriggerVehicleInteractionForEvidence();
	void CaptureVehicleInteractionEvidenceScreenshot(const FString& StepSuffix) const;
	void WriteVehicleInteractionEvidenceState(const FString& StepSuffix) const;
	AFWVehicleBase* FindVehicleInteractionEvidenceTarget() const;
	void PositionPlayerForVehicleInteractionEvidence();
	void StartVehicleDriveEvidenceSequence();
	void AdvanceVehicleDriveEvidenceSequence();
	void TickVehicleDriveEvidenceSequence();
	AFWVehicleBase* FindVehicleDriveEvidenceTarget() const;
	void WriteVehicleDriveEvidenceState(const FString& StepSuffix, const FString& ActionLabel, const FHitResult* CollisionHit = nullptr) const;
	void PositionPlayerForVehicleDriveEvidence();
	void PumpVehicleDriveEvidence(float TotalSeconds, float StepSeconds = 0.05f) const;
	void StartVehicleDestructionEvidenceSequence();
	void AdvanceVehicleDestructionEvidenceSequence();
	void TickVehicleDestructionEvidenceSequence();
	AFWVehicleBase* FindVehicleDestructionEvidenceTarget() const;
	void WriteVehicleDestructionEvidenceState(const FString& StepSuffix, const FString& ActionLabel) const;

	UFUNCTION()
	void HandleVehicleDestructionEvidenceDestroyed(AFWVehicleBase* Vehicle);

	UFUNCTION()
	void HandleVehicleDestructionEvidenceGameplayEvent(const FFWGameplayEvent& Event);

	void DebugFireNearestAI();
	void DebugFireNearestVehicle();
};
