#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FWDamageTypes.h"
#include "GameFramework/Actor.h"
#include "FWHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWHealthChangedSignature, float, CurrentHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWDeathSignature, AActor*, DeadActor);

UCLASS(ClassGroup = (FW), meta = (BlueprintSpawnableComponent))
class FW_API UFWHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFWHealthComponent();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(BlueprintAssignable, Category = "FW|Health")
	FFWHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "FW|Health")
	FFWDeathSignature OnDeath;

	UFUNCTION(BlueprintCallable, Category = "FW|Health")
	float ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "FW|Health")
	FFWDamageResult ApplyDamageRequest(const FFWDamageRequest& DamageRequest);

	UFUNCTION(BlueprintCallable, Category = "FW|Health")
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category = "FW|Health")
	bool IsDead() const { return bIsDead; }

private:
	bool bIsDead = false;
};
