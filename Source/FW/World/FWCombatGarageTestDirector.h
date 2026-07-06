#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FWCombatGarageSpawnPoint.h"
#include "FWCombatGarageTestDirector.generated.h"

class AFWAICharacter;
class AFWVehicleBase;
class AFWWeaponBase;

UCLASS(Blueprintable)
class FW_API AFWCombatGarageTestDirector : public AActor
{
	GENERATED_BODY()

public:
	AFWCombatGarageTestDirector();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "FW|Combat Garage")
	void SetupCombatGarage();

	UFUNCTION(BlueprintCallable, Category = "FW|Combat Garage")
	bool ValidateCombatGarageSetup(FString& OutValidationMessage) const;

	UFUNCTION(BlueprintCallable, Category = "FW|Combat Garage")
	void ConfigureMVPDefaults();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage")
	TSubclassOf<AFWAICharacter> AICharacterClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "FW|Combat Garage")
	TSubclassOf<AFWVehicleBase> CarVehicleClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "FW|Combat Garage")
	TSubclassOf<AFWVehicleBase> MotorcycleVehicleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage")
	TSubclassOf<AFWWeaponBase> TestWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage", meta = (ClampMin = "0"))
	int32 DesiredAICount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage", meta = (ClampMin = "0.0"))
	float MinimumInitialAIDistanceFromPlayer = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage")
	bool bSetupOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage")
	bool bStartMatchAfterSetup = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage", meta = (ClampMin = "0.0"))
	float BeginPlaySetupDelay = 0.0f;

private:
	bool bHasSetup = false;
	FTimerHandle SetupTimerHandle;

	TArray<AFWCombatGarageSpawnPoint*> GetSpawnPointsByType(EFWCombatGarageSpawnType SpawnType) const;
	APlayerController* GetFirstPlayerController() const;
	void MovePlayerToSpawn() const;
	void SpawnAI();
	void SpawnVehicles();
	void SpawnTestWeapons();
	void RegisterCoreVehicleForPlayer(AFWVehicleBase* Vehicle) const;
	void StartMatchFlow() const;
	void BroadcastGarageReady() const;
};
