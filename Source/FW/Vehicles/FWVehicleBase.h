#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "../World/FWInteractable.h"
#include "FWVehicleBase.generated.h"

class UCameraComponent;
class UBoxComponent;
class UFloatingPawnMovement;
class UFWHealthComponent;
class UFWStateMachineComponent;
class UFWVehicleData;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWVehicleDestroyedSignature, class AFWVehicleBase*, Vehicle);

UCLASS(Blueprintable)
class FW_API AFWVehicleBase : public APawn, public IFWInteractable
{
	GENERATED_BODY()

public:
	AFWVehicleBase();

	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractionText_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	bool CanEnterVehicle(AController* EnteringController) const;

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	bool EnterVehicle(AController* EnteringController);

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	bool ExitVehicle();

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	void HandleVehicleDestroyed();

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	bool IsDestroyed() const { return bDestroyed; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	AController* GetOccupantController() const { return OccupantController; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	AController* GetOwnerController() const { return OwnerController; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	APawn* GetPreviousPawn() const { return PreviousPawn; }

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	void SetOwnerController(AController* NewOwnerController);

	UFUNCTION(BlueprintCallable, Category = "FW|Vehicle")
	void SetCoreVehicle(bool bNewCoreVehicle);

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	bool IsCoreVehicle() const { return bCoreVehicle; }

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void HandleExitInput();

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	float GetCurrentForwardSpeed() const { return CurrentForwardSpeed; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	float GetThrottleInput() const { return ThrottleInput; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	float GetSteeringInput() const { return SteeringInput; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	UBoxComponent* GetVehicleCollision() const { return VehicleCollision; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	UCameraComponent* GetVehicleCameraComponent() const { return CameraComponent; }

	UFUNCTION(BlueprintPure, Category = "FW|Vehicle")
	UFloatingPawnMovement* GetVehicleMovementComponent() const { return MovementComponent; }

	UPROPERTY(BlueprintAssignable, Category = "FW|Vehicle")
	FFWVehicleDestroyedSignature OnVehicleDestroyed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<UBoxComponent> VehicleCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Camera")
	TObjectPtr<USpringArmComponent> CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<UFWHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<UFWStateMachineComponent> StateMachineComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Vehicle")
	TObjectPtr<UFWVehicleData> VehicleData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputMappingContext> VehicleMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Vehicle")
	TObjectPtr<AController> OccupantController;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Vehicle")
	TObjectPtr<AController> OwnerController;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Vehicle")
	TObjectPtr<APawn> PreviousPawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Vehicle")
	bool bCoreVehicle = false;

private:
	bool bDestroyed = false;
	float CurrentForwardSpeed = 0.0f;
	float ThrottleInput = 0.0f;
	float SteeringInput = 0.0f;

	UFUNCTION()
	void HandleHealthDeath(AActor* DeadActor);

	void ApplyVehicleData();
	void ChangeVehicleState(FName StateTagName);
	void BroadcastVehicleDestroyedEvent() const;
	void RestorePreviousPawn();
	void AddVehicleInputMappingContext();
	void RemoveVehicleInputMappingContext();
	void ApplyDriving(float DeltaSeconds);
};
