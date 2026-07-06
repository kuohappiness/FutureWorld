#pragma once

#include "CoreMinimal.h"
#include "FWCharacterBase.h"
#include "InputActionValue.h"
#include "FWPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;

UCLASS()
class FW_API AFWPlayerCharacter : public AFWCharacterBase
{
	GENERATED_BODY()

public:
	AFWPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "FW|Camera")
	bool IsFirstPersonCamera() const { return bFirstPersonCamera; }

	UFUNCTION(BlueprintPure, Category = "FW|Interaction")
	AActor* GetCurrentInteractableActor() const { return CurrentInteractableActor; }

	UFUNCTION(BlueprintPure, Category = "FW|Movement")
	bool IsCrawling() const { return bIsCrawling; }

	UFUNCTION(BlueprintPure, Category = "FW|Movement")
	const FString& GetMovementInputDebugText() const { return MovementInputDebugText; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Camera")
	TObjectPtr<USpringArmComponent> ThirdPersonSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputMappingContext> OnFootMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> CrawlAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> ToggleCameraAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> WeaponSlot1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> WeaponSlot2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> WeaponSlot3Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon")
	TSubclassOf<AFWWeaponBase> PistolWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon")
	TSubclassOf<AFWWeaponBase> RifleWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon")
	TSubclassOf<AFWWeaponBase> SniperWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Movement", meta = (ClampMin = "0.0"))
	float RunSpeed = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Movement", meta = (ClampMin = "0.0"))
	float CrawlSpeed = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Interaction", meta = (ClampMin = "0.0"))
	float InteractionTraceDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Interaction", meta = (ClampMin = "0.0"))
	float InteractionFallbackRadius = 1200.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Interaction")
	TObjectPtr<AActor> CurrentInteractableActor;

	float LastInteractInputTime = -FLT_MAX;

public:
	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void Move(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void Look(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void StartJump();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void StopJump();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void StopRun();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void ToggleCrouch();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void ToggleCrawl();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void ReloadWeapon();

	UFUNCTION(BlueprintCallable, Category = "FW|Input")
	void Interact();

	UFUNCTION(BlueprintCallable, Category = "FW|Camera")
	void ToggleCamera();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void EquipWeaponSlot1();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void EquipWeaponSlot2();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void EquipWeaponSlot3();

	UFUNCTION(BlueprintCallable, Category = "FW|Interaction")
	void UpdateInteractionTarget();

protected:
	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

private:
	bool bFirstPersonCamera = false;
	bool bIsCrawling = false;
	FString MovementInputDebugText = TEXT("None");
	UPROPERTY()
	TArray<TObjectPtr<AFWWeaponBase>> WeaponSlots;

	bool CanMove() const;
	void AddOnFootInputMappingContext();
	void ApplyCameraMode();
	void SpawnDefaultWeaponLoadout();
	void EquipWeaponSlot(int32 SlotIndex);
};
