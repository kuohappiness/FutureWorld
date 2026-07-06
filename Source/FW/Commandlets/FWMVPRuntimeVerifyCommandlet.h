#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FWMVPRuntimeVerifyCommandlet.generated.h"

UCLASS()
class FW_API UFWMVPRuntimeVerifyCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFWMVPRuntimeVerifyCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	int32 IssueCount = 0;

	void Check(bool bCondition, const FString& Message);
	bool VerifyRuntimeWorld();
	bool VerifyPlayerInputSemantics(class UWorld* World);
	bool VerifyAICombatRuntime(class UWorld* World);
	bool VerifyWeaponDamage(class UWorld* World);
	bool VerifyVehicleEnterExitAndDestruction(class UWorld* World);
	bool VerifyVehicleDrivingRuntime(class UWorld* World);
	bool VerifyCombatGarageDirectorRuntime(class UWorld* World);
	bool VerifyCombatGarageMapRuntime(class UWorld* World);
	bool VerifyHUDSnapshotRuntime(class UWorld* World);
	bool VerifyRulesRespawnRuntime(class UWorld* World);
	bool VerifyDebugRuntime();
};
