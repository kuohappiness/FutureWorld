#pragma once

#include "CoreMinimal.h"
#include "FWGameModeBase.h"
#include "GameplayTagContainer.h"
#include "FWCompetitiveGameMode.generated.h"

class AFWPlayerState;
class AFWVehicleBase;
class UFWMatchRuleSet;

UCLASS()
class FW_API AFWCompetitiveGameMode : public AFWGameModeBase
{
	GENERATED_BODY()

public:
	AFWCompetitiveGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "FW|Match")
	void StartMatchFlow();

	UFUNCTION(BlueprintCallable, Category = "FW|Match")
	void EndMatchFlow();

	UFUNCTION(BlueprintCallable, Category = "FW|Respawn")
	void RequestRespawn(AController* RespawnController);

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicles")
	void HandleCoreVehicleDestroyed(AController* OwnerController);

	UFUNCTION(BlueprintPure, Category = "FW|Match")
	FGameplayTag GetCurrentMatchStateTag() const { return CurrentMatchStateTag; }

	UFUNCTION(BlueprintCallable, Category = "FW|Rules")
	void SetMatchRuleSet(UFWMatchRuleSet* NewMatchRuleSet);

	UFUNCTION(BlueprintPure, Category = "FW|Rules")
	UFWMatchRuleSet* GetMatchRuleSet() const { return MatchRuleSet; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Rules")
	TObjectPtr<UFWMatchRuleSet> MatchRuleSet;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Match")
	FGameplayTag CurrentMatchStateTag;

private:
	FTimerHandle RespawnTimerHandle;

	UFUNCTION()
	void HandleGameplayEvent(const struct FFWGameplayEvent& Event);

	void SetMatchStateTag(FName NewStateTagName);
	void InitializePlayerState(AController* Controller) const;
	bool CanApplyMatchResults() const;
	int32 GetConfiguredPlayerLives() const;
	float GetConfiguredRespawnDelay() const;
	bool IsRespawnEnabled() const;
	bool IsCoreVehicleRuleEnabled() const;
	int32 GetCoreVehiclePenalty() const;
	void FinishRespawn(AController* RespawnController);
	void BroadcastRespawnCompleted(AController* RespawnController) const;
};
