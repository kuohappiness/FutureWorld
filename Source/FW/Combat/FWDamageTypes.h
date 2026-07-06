#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FWDamageTypes.generated.h"

USTRUCT(BlueprintType)
struct FW_API FFWDamageRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	TObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	FName HitZoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	FGameplayTag DamageTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	FHitResult HitResult;
};

USTRUCT(BlueprintType)
struct FW_API FFWDamageResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	TObjectPtr<AActor> DamagedActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	float AppliedDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	FName HitZoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	float RemainingHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FW|Damage")
	bool bKilled = false;
};
