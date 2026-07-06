#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FWEnemyAIController.generated.h"

UCLASS()
class FW_API AFWEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFWEnemyAIController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|AI", meta = (ClampMin = "0.0"))
	float DetectionRadius = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|AI", meta = (ClampMin = "0.0"))
	float AttackRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|AI", meta = (ClampMin = "0.01"))
	float AttackInterval = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Accuracy = 0.65f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|AI")
	TObjectPtr<AActor> CurrentTarget;

private:
	float LastAttackTime = -FLT_MAX;

	AActor* FindBestTarget() const;
	void UpdateAIState(FName StateTagName) const;
	void RunCombatBehavior(float DeltaSeconds);
	bool IsControlledPawnDead() const;
};
