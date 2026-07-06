#include "FWPlayerCharacter.h"

#include "../State/FWStateMachineComponent.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../World/FWInteractable.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "../Combat/FWHealthComponent.h"
#include "../Weapons/FWWeaponBase.h"
#include "../Weapons/FWWeaponData.h"
#include "../Weapons/FWMVPWeaponClasses.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

namespace FWPlayerStateTags
{
	const FName OnFoot(TEXT("State.Character.OnFoot"));
	const FName Dead(TEXT("State.Character.Dead"));
}

namespace FWPlayerInputAssets
{
	template <typename TAsset>
	TAsset* LoadDefaultAsset(const TCHAR* AssetPath)
	{
		return LoadObject<TAsset>(nullptr, AssetPath);
	}
}

AFWPlayerCharacter::AFWPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonSpringArm"));
	ThirdPersonSpringArm->SetupAttachment(GetRootComponent());
	ThirdPersonSpringArm->TargetArmLength = 350.0f;
	ThirdPersonSpringArm->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(ThirdPersonSpringArm);
	CameraComponent->bUsePawnControlRotation = false;

	OnFootMappingContext = FWPlayerInputAssets::LoadDefaultAsset<UInputMappingContext>(TEXT("/Game/FW/Input/MappingContexts/IMC_OnFoot.IMC_OnFoot"));
	MoveAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Move.IA_Move"));
	LookAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Look.IA_Look"));
	JumpAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Jump.IA_Jump"));
	RunAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Run.IA_Run"));
	CrouchAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Crouch.IA_Crouch"));
	CrawlAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Crawl.IA_Crawl"));
	FireAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Fire.IA_Fire"));
	ReloadAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Reload.IA_Reload"));
	InteractAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_Interact.IA_Interact"));
	ToggleCameraAction = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_ToggleCamera.IA_ToggleCamera"));
	WeaponSlot1Action = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_WeaponSlot1.IA_WeaponSlot1"));
	WeaponSlot2Action = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_WeaponSlot2.IA_WeaponSlot2"));
	WeaponSlot3Action = FWPlayerInputAssets::LoadDefaultAsset<UInputAction>(TEXT("/Game/FW/Input/Actions/IA_WeaponSlot3.IA_WeaponSlot3"));

	PistolWeaponClass = AFWPistolWeapon::StaticClass();
	RifleWeaponClass = AFWRifleWeapon::StaticClass();
	SniperWeaponClass = AFWSniperWeapon::StaticClass();
}

void AFWPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (StateMachineComponent)
	{
		StateMachineComponent->ChangeState(FGameplayTag::RequestGameplayTag(FWPlayerStateTags::OnFoot));
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AFWPlayerCharacter::HandleDeath);
	}

	AddOnFootInputMappingContext();
	ApplyCameraMode();
	SpawnDefaultWeaponLoadout();
}

void AFWPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateInteractionTarget();
}

void AFWPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddOnFootInputMappingContext();
}

void AFWPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFWPlayerCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFWPlayerCharacter::Look);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::StartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFWPlayerCharacter::StopJump);
	}
	if (RunAction)
	{
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::StartRun);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Completed, this, &AFWPlayerCharacter::StopRun);
	}
	if (CrouchAction)
	{
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::ToggleCrouch);
	}
	if (CrawlAction)
	{
		EnhancedInput->BindAction(CrawlAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::ToggleCrawl);
	}
	if (FireAction)
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::StartFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AFWPlayerCharacter::StopFire);
	}
	if (ReloadAction)
	{
		EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::ReloadWeapon);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::Interact);
	}
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AFWPlayerCharacter::Interact);
	if (ToggleCameraAction)
	{
		EnhancedInput->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &AFWPlayerCharacter::ToggleCamera);
	}
	if (WeaponSlot1Action)
	{
		EnhancedInput->BindAction(WeaponSlot1Action, ETriggerEvent::Started, this, &AFWPlayerCharacter::EquipWeaponSlot1);
	}
	if (WeaponSlot2Action)
	{
		EnhancedInput->BindAction(WeaponSlot2Action, ETriggerEvent::Started, this, &AFWPlayerCharacter::EquipWeaponSlot2);
	}
	if (WeaponSlot3Action)
	{
		EnhancedInput->BindAction(WeaponSlot3Action, ETriggerEvent::Started, this, &AFWPlayerCharacter::EquipWeaponSlot3);
	}
}

void AFWPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!CanMove())
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller && !MovementVector.IsNearlyZero())
	{
		MovementInputDebugText = FString::Printf(TEXT("Move X=%.2f Y=%.2f"), MovementVector.X, MovementVector.Y);
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementVector.Y);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementVector.X);
	}
}

void AFWPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AFWPlayerCharacter::StartJump()
{
	if (CanMove())
	{
		MovementInputDebugText = TEXT("JumpStart");
		Jump();
	}
}

void AFWPlayerCharacter::StopJump()
{
	MovementInputDebugText = TEXT("JumpStop");
	StopJumping();
}

void AFWPlayerCharacter::StartRun()
{
	if (CanMove() && !bIsCrawling)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		MovementInputDebugText = FString::Printf(TEXT("RunStart Max=%.0f"), GetCharacterMovement()->MaxWalkSpeed);
	}
}

void AFWPlayerCharacter::StopRun()
{
	if (!bIsCrawling)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		MovementInputDebugText = FString::Printf(TEXT("RunStop Max=%.0f"), GetCharacterMovement()->MaxWalkSpeed);
	}
}

void AFWPlayerCharacter::ToggleCrouch()
{
	if (!CanMove())
	{
		return;
	}

	bIsCrawling = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (bIsCrouched)
	{
		UnCrouch();
		MovementInputDebugText = TEXT("CrouchOff");
	}
	else
	{
		Crouch();
		MovementInputDebugText = TEXT("CrouchOn");
	}
}

void AFWPlayerCharacter::ToggleCrawl()
{
	if (!CanMove())
	{
		return;
	}

	bIsCrawling = !bIsCrawling;
	GetCharacterMovement()->MaxWalkSpeed = bIsCrawling ? CrawlSpeed : WalkSpeed;

	if (bIsCrawling && !bIsCrouched)
	{
		Crouch();
	}
	MovementInputDebugText = bIsCrawling ? TEXT("CrawlOn") : TEXT("CrawlOff");
}

void AFWPlayerCharacter::StartFire()
{
	if (CanMove() && CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AFWPlayerCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AFWPlayerCharacter::ReloadWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Reload();
	}
}

void AFWPlayerCharacter::Interact()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if ((Now - LastInteractInputTime) < 0.1f)
	{
		return;
	}
	LastInteractInputTime = Now;

	AActor* InteractionTarget = CurrentInteractableActor;
	UpdateInteractionTarget();

	if (CurrentInteractableActor)
	{
		InteractionTarget = CurrentInteractableActor;
	}

	if (InteractionTarget && InteractionTarget->GetClass()->ImplementsInterface(UFWInteractable::StaticClass()))
	{
		if (IFWInteractable* NativeInteractable = Cast<IFWInteractable>(InteractionTarget))
		{
			NativeInteractable->Interact_Implementation(this);
		}
		else
		{
			IFWInteractable::Execute_Interact(InteractionTarget, this);
		}
	}
}

void AFWPlayerCharacter::ToggleCamera()
{
	bFirstPersonCamera = !bFirstPersonCamera;
	ApplyCameraMode();
}

void AFWPlayerCharacter::EquipWeaponSlot1()
{
	EquipWeaponSlot(0);
}

void AFWPlayerCharacter::EquipWeaponSlot2()
{
	EquipWeaponSlot(1);
}

void AFWPlayerCharacter::EquipWeaponSlot3()
{
	EquipWeaponSlot(2);
}

void AFWPlayerCharacter::UpdateInteractionTarget()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		CurrentInteractableActor = nullptr;
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * InteractionTraceDistance);
	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FWInteractionTrace), false, this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams) &&
		Hit.GetActor() &&
		Hit.GetActor()->GetClass()->ImplementsInterface(UFWInteractable::StaticClass()))
	{
		CurrentInteractableActor = Hit.GetActor();
	}
	else
	{
		CurrentInteractableActor = nullptr;
		const float FallbackDistanceSquared = FMath::Square(InteractionFallbackRadius);
		float BestDistanceSquared = FallbackDistanceSquared;
		for (TActorIterator<AFWVehicleBase> It(GetWorld()); It; ++It)
		{
			AFWVehicleBase* Vehicle = *It;
			if (!Vehicle || Vehicle->IsDestroyed() || !Vehicle->GetClass()->ImplementsInterface(UFWInteractable::StaticClass()))
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				CurrentInteractableActor = Vehicle;
			}
		}
	}
}

void AFWPlayerCharacter::HandleDeath(AActor* DeadActor)
{
	if (DeadActor != this || !StateMachineComponent)
	{
		return;
	}

	StateMachineComponent->ChangeState(FGameplayTag::RequestGameplayTag(FWPlayerStateTags::Dead));
	GetCharacterMovement()->StopMovementImmediately();
}

bool AFWPlayerCharacter::CanMove() const
{
	if (HealthComponent && HealthComponent->IsDead())
	{
		return false;
	}

	if (!StateMachineComponent)
	{
		return true;
	}

	const FGameplayTag DeadState = FGameplayTag::RequestGameplayTag(FWPlayerStateTags::Dead);
	return !DeadState.IsValid() || StateMachineComponent->CurrentState != DeadState;
}

void AFWPlayerCharacter::AddOnFootInputMappingContext()
{
	if (!OnFootMappingContext)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->RemoveMappingContext(OnFootMappingContext);
				InputSubsystem->AddMappingContext(OnFootMappingContext, 0);
			}
		}
	}
}

void AFWPlayerCharacter::ApplyCameraMode()
{
	if (!ThirdPersonSpringArm || !CameraComponent)
	{
		return;
	}

	if (bFirstPersonCamera)
	{
		ThirdPersonSpringArm->TargetArmLength = 0.0f;
		CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	}
	else
	{
		ThirdPersonSpringArm->TargetArmLength = 350.0f;
		CameraComponent->SetRelativeLocation(FVector::ZeroVector);
	}
}

void AFWPlayerCharacter::SpawnDefaultWeaponLoadout()
{
	if (WeaponSlots.Num() > 0 || !GetWorld())
	{
		return;
	}

	const TSubclassOf<AFWWeaponBase> WeaponClasses[] =
	{
		PistolWeaponClass,
		RifleWeaponClass,
		SniperWeaponClass
	};

	for (TSubclassOf<AFWWeaponBase> WeaponClass : WeaponClasses)
	{
		if (!WeaponClass)
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.Instigator = this;

		AFWWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AFWWeaponBase>(WeaponClass, GetActorTransform(), SpawnParameters);
		if (!SpawnedWeapon)
		{
			continue;
		}

		SpawnedWeapon->Equip(this);
		SpawnedWeapon->Unequip();
		WeaponSlots.Add(SpawnedWeapon);
	}

	EquipWeaponSlot(0);
}

void AFWPlayerCharacter::EquipWeaponSlot(int32 SlotIndex)
{
	if (!CanMove() || SlotIndex < 0)
	{
		return;
	}

	if (!WeaponSlots.IsValidIndex(SlotIndex) || !WeaponSlots[SlotIndex])
	{
		return;
	}

	EquipWeapon(WeaponSlots[SlotIndex]);
}
