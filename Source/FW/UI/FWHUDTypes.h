#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FWHUDTypes.generated.h"

USTRUCT(BlueprintType)
struct FW_API FFWHUDStateSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float PlayerHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float PlayerMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	int32 Lives = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FName WeaponContentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float VehicleHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float VehicleMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float CoreVehicleHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float CoreVehicleMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FGameplayTag PlayerStateTag;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FGameplayTag MatchStateTag;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float PlayerSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float PlayerMaxWalkSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	bool bPlayerFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	bool bPlayerCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	bool bPlayerCrawling = false;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FString PlayerMovementInputDebug;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	bool bFirstPersonCamera = false;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FString InteractionDebugText;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	int32 AICount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	int32 AliveAICount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float NearestAIHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float NearestAIMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FGameplayTag NearestAIStateTag;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FString LastCombatDebugText;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	int32 VehicleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	int32 ActiveVehicleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float NearestVehicleHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	float NearestVehicleMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|HUD")
	FGameplayTag NearestVehicleStateTag;
};
