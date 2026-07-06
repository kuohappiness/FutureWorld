#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Events/FWGameplayEvent.h"
#include "FWDebugSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FW_API FFWDebugEventRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FW|Debug")
	FGameplayTag EventTag;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Debug")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Debug")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Debug")
	float Magnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Debug")
	FVector WorldLocation = FVector::ZeroVector;
};

UCLASS()
class FW_API UFWDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "FW|Debug")
	void SetDebugEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "FW|Debug")
	bool IsDebugEnabled() const { return bDebugEnabled; }

	UFUNCTION(BlueprintCallable, Category = "FW|Debug")
	void PrintActorDebugState(AActor* Actor, float Duration = 2.0f) const;

	UFUNCTION(BlueprintPure, Category = "FW|Debug")
	const TArray<FFWDebugEventRecord>& GetRecentEvents() const { return RecentEvents; }

	UFUNCTION(BlueprintCallable, Category = "FW|Debug")
	void ClearRecentEvents();

	UFUNCTION(BlueprintCallable, Category = "FW|Debug")
	void RecordGameplayEvent(const FFWGameplayEvent& Event);

private:
	bool bDebugEnabled = false;

	UPROPERTY()
	TArray<FFWDebugEventRecord> RecentEvents;

	UPROPERTY()
	int32 MaxRecentEvents = 64;

	UFUNCTION()
	void HandleGameplayEvent(const FFWGameplayEvent& Event);
};
