#include "FWVehicleBase.h"

#include "../Combat/FWHealthComponent.h"
#include "../Events/FWEventSubsystem.h"
#include "../State/FWStateMachineComponent.h"
#include "../Core/FWPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "FWVehicleData.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace FWVehicleStateTags
{
	const FName Idle(TEXT("State.Vehicle.Idle"));
	const FName Occupied(TEXT("State.Vehicle.Occupied"));
	const FName Destroyed(TEXT("State.Vehicle.Destroyed"));
}

namespace FWVehicleEventTags
{
	const FName VehicleDestroyed(TEXT("Event.Vehicle.Destroyed"));
	const FName CoreVehicleDestroyed(TEXT("Event.Vehicle.CoreDestroyed"));
}

namespace FWVehicleInputAssets
{
	template <typename TAsset>
	TAsset* LoadDefaultAsset(const TCHAR* AssetPath)
	{
		return LoadObject<TAsset>(nullptr, AssetPath);
	}
}

AFWVehicleBase::AFWVehicleBase()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("VehicleCollision"));
	VehicleCollision->SetBoxExtent(FVector(180.0f, 90.0f, 55.0f));
	VehicleCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VehicleCollision->SetCollisionObjectType(ECC_Pawn);
	VehicleCollision->SetCollisionResponseToAllChannels(ECR_Block);
	VehicleCollision->SetGenerateOverlapEvents(false);
	SetRootComponent(VehicleCollision);

	SceneRoot = VehicleCollision;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = VehicleCollision;
	MovementComponent->MaxSpeed = 1800.0f;
	MovementComponent->Acceleration = 800.0f;
	MovementComponent->Deceleration = 1200.0f;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(SceneRoot);
	CameraSpringArm->TargetArmLength = 600.0f;
	CameraSpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	CameraSpringArm->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CameraSpringArm);
	CameraComponent->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UFWHealthComponent>(TEXT("HealthComponent"));
	StateMachineComponent = CreateDefaultSubobject<UFWStateMachineComponent>(TEXT("StateMachineComponent"));
	HealthComponent->OnDeath.AddUniqueDynamic(this, &AFWVehicleBase::HandleHealthDeath);

	VehicleMappingContext = FWVehicleInputAssets::LoadDefaultAsset<UInputMappingContext>(TEXT("/Game/FW/Input/MappingContexts/IMC_Vehicle.IMC_Vehicle"));
	MoveAction = FWVehicleInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Move.IA_Move"));
	LookAction = FWVehicleInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Look.IA_Look"));
	InteractAction = FWVehicleInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Interact.IA_Interact"));
}

void AFWVehicleBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bDestroyed && HealthComponent && HealthComponent->IsDead())
	{
		HandleVehicleDestroyed();
	}
	ApplyDriving(DeltaSeconds);
}

void AFWVehicleBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddUniqueDynamic(this, &AFWVehicleBase::HandleHealthDeath);
	}
}

void AFWVehicleBase::BeginPlay()
{
	Super::BeginPlay();

	ApplyVehicleData();
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddUniqueDynamic(this, &AFWVehicleBase::HandleHealthDeath);
	}

	ChangeVehicleState(FWVehicleStateTags::Idle);
}

void AFWVehicleBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AddVehicleInputMappingContext();
}

void AFWVehicleBase::UnPossessed()
{
	RemoveVehicleInputMappingContext();
	Super::UnPossessed();
}

void AFWVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFWVehicleBase::Move);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &AFWVehicleBase::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFWVehicleBase::Look);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AFWVehicleBase::HandleExitInput);
	}
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AFWVehicleBase::HandleExitInput);
}

void AFWVehicleBase::Interact_Implementation(AActor* InteractingActor)
{
	AController* InteractingController = InteractingActor ? InteractingActor->GetInstigatorController() : nullptr;
	if (!InteractingController)
	{
		if (const APawn* InteractingPawn = Cast<APawn>(InteractingActor))
		{
			InteractingController = InteractingPawn->GetController();
		}
	}

	if (InteractingController)
	{
		if (OccupantController == InteractingController)
		{
			ExitVehicle();
		}
		else
		{
			EnterVehicle(InteractingController);
		}
	}
}

FText AFWVehicleBase::GetInteractionText_Implementation() const
{
	return OccupantController ? FText::FromString(TEXT("Exit Vehicle")) : FText::FromString(TEXT("Enter Vehicle"));
}

bool AFWVehicleBase::CanEnterVehicle(AController* EnteringController) const
{
	return EnteringController && !bDestroyed && !OccupantController;
}

bool AFWVehicleBase::EnterVehicle(AController* EnteringController)
{
	if (!CanEnterVehicle(EnteringController))
	{
		return false;
	}

	OccupantController = EnteringController;
	PreviousPawn = EnteringController->GetPawn();
	if (!OwnerController)
	{
		OwnerController = EnteringController;
	}
	if (AFWPlayerState* FWPlayerState = EnteringController->GetPlayerState<AFWPlayerState>())
	{
		if (!FWPlayerState->CoreVehicle)
		{
			FWPlayerState->SetCoreVehicle(this);
		}
	}

	EnteringController->Possess(this);
	ChangeVehicleState(FWVehicleStateTags::Occupied);
	return true;
}

bool AFWVehicleBase::ExitVehicle()
{
	if (!OccupantController || bDestroyed)
	{
		return false;
	}

	RestorePreviousPawn();
	ChangeVehicleState(FWVehicleStateTags::Idle);
	return true;
}

void AFWVehicleBase::HandleVehicleDestroyed()
{
	if (bDestroyed)
	{
		return;
	}

	bDestroyed = true;
	RestorePreviousPawn();
	ChangeVehicleState(FWVehicleStateTags::Destroyed);
	BroadcastVehicleDestroyedEvent();
	OnVehicleDestroyed.Broadcast(this);
}

void AFWVehicleBase::SetOwnerController(AController* NewOwnerController)
{
	OwnerController = NewOwnerController;
}

void AFWVehicleBase::SetCoreVehicle(bool bNewCoreVehicle)
{
	bCoreVehicle = bNewCoreVehicle;
}

void AFWVehicleBase::HandleHealthDeath(AActor* DeadActor)
{
	if (DeadActor == this)
	{
		HandleVehicleDestroyed();
	}
}

void AFWVehicleBase::ApplyVehicleData()
{
	if (VehicleData && HealthComponent)
	{
		HealthComponent->MaxHealth = VehicleData->MaxHealth;
		HealthComponent->ResetHealth();
	}

	if (VehicleData && MovementComponent)
	{
		MovementComponent->MaxSpeed = VehicleData->MaxSpeed;
		MovementComponent->Acceleration = VehicleData->Acceleration;
		MovementComponent->Deceleration = FMath::Max(VehicleData->Acceleration * 1.5f, 1000.0f);
	}
}

void AFWVehicleBase::ChangeVehicleState(FName StateTagName)
{
	if (StateMachineComponent)
	{
		StateMachineComponent->ChangeState(FGameplayTag::RequestGameplayTag(StateTagName));
	}
}

void AFWVehicleBase::BroadcastVehicleDestroyedEvent() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>();
	if (!EventSubsystem)
	{
		return;
	}

	FFWGameplayEvent Event;
	Event.EventTag = FGameplayTag::RequestGameplayTag(bCoreVehicle ? FWVehicleEventTags::CoreVehicleDestroyed : FWVehicleEventTags::VehicleDestroyed);
	Event.InstigatorActor = OwnerController ? OwnerController->GetPawn() : nullptr;
	Event.Target = const_cast<AFWVehicleBase*>(this);
	Event.Magnitude = HealthComponent ? HealthComponent->CurrentHealth : 0.0f;
	Event.WorldLocation = GetActorLocation();
	EventSubsystem->BroadcastGameplayEvent(Event);
}

void AFWVehicleBase::RestorePreviousPawn()
{
	if (OccupantController && PreviousPawn && PreviousPawn != this)
	{
		OccupantController->Possess(PreviousPawn);
	}

	OccupantController = nullptr;
	PreviousPawn = nullptr;
	ThrottleInput = 0.0f;
	SteeringInput = 0.0f;
	CurrentForwardSpeed = 0.0f;
}

void AFWVehicleBase::AddVehicleInputMappingContext()
{
	if (!VehicleMappingContext)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController() ? GetController() : OccupantController.Get()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->RemoveMappingContext(VehicleMappingContext);
				InputSubsystem->AddMappingContext(VehicleMappingContext, 1);
			}
		}
	}
}

void AFWVehicleBase::RemoveVehicleInputMappingContext()
{
	if (!VehicleMappingContext)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController() ? GetController() : OccupantController.Get()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->RemoveMappingContext(VehicleMappingContext);
			}
		}
	}
}

void AFWVehicleBase::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	SteeringInput = FMath::Clamp(MovementVector.X, -1.0f, 1.0f);
	ThrottleInput = FMath::Clamp(MovementVector.Y, -1.0f, 1.0f);
}

void AFWVehicleBase::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AFWVehicleBase::HandleExitInput()
{
	ExitVehicle();
}

void AFWVehicleBase::ApplyDriving(float DeltaSeconds)
{
	if (!OccupantController || bDestroyed || !MovementComponent)
	{
		return;
	}

	const float MaxSpeed = VehicleData ? VehicleData->MaxSpeed : MovementComponent->MaxSpeed;
	const float Acceleration = VehicleData ? VehicleData->Acceleration : MovementComponent->Acceleration;
	const float Handling = VehicleData ? VehicleData->Handling : 1.0f;
	const float TargetSpeed = ThrottleInput * MaxSpeed;
	CurrentForwardSpeed = FMath::FInterpTo(CurrentForwardSpeed, TargetSpeed, DeltaSeconds, FMath::Max(Acceleration / FMath::Max(MaxSpeed, 1.0f), 1.0f));

	const float SpeedAlpha = FMath::Clamp(FMath::Abs(CurrentForwardSpeed) / FMath::Max(MaxSpeed, 1.0f), 0.15f, 1.0f);
	const float YawDelta = SteeringInput * Handling * 95.0f * SpeedAlpha * DeltaSeconds;
	AddActorWorldRotation(FRotator(0.0f, YawDelta, 0.0f), false, nullptr, ETeleportType::None);

	const FVector MoveDelta = GetActorForwardVector() * CurrentForwardSpeed * DeltaSeconds;
	AddActorWorldOffset(MoveDelta, true);
}
