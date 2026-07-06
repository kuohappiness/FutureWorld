#include "FWPlayerController.h"

#include "../Characters/FWAICharacter.h"
#include "../Characters/FWCharacterBase.h"
#include "../Characters/FWPlayerCharacter.h"
#include "../Combat/FWHealthComponent.h"
#include "../Core/FWCompetitiveGameMode.h"
#include "../State/FWStateMachineComponent.h"
#include "../UI/FWHUDWidget.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Weapons/FWWeaponBase.h"
#include "../Weapons/FWWeaponData.h"
#include "../World/FWInteractable.h"
#include "FWPlayerState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"

AFWPlayerController::AFWPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	HUDWidgetClass = UFWHUDWidget::StaticClass();
}

void AFWPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CreateHUDWidget();
	RefreshHUDWidget();
	StartVehicleInteractionEvidenceSequence();
}

void AFWPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickVehicleInteractionEvidenceSequence();

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if ((Now - LastHUDRefreshTime) >= HUDRefreshInterval)
	{
		RefreshHUDWidget();
		LastHUDRefreshTime = Now;
	}
}

void AFWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AFWPlayerController::HandleVehicleExitKeyFallback);
		InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AFWPlayerController::DebugFireNearestAI);
		InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AFWPlayerController::DebugFireNearestVehicle);
	}
}

FFWHUDStateSnapshot AFWPlayerController::BuildHUDStateSnapshot() const
{
	FFWHUDStateSnapshot Snapshot;

	const AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(GetPawn());
	const AFWCharacterBase* FWCharacter = Cast<AFWCharacterBase>(GetPawn());
	if (!FWCharacter && VehiclePawn)
	{
		FWCharacter = Cast<AFWCharacterBase>(VehiclePawn->GetPreviousPawn());
	}

	if (FWCharacter)
	{
		Snapshot.PlayerLocation = FWCharacter->GetActorLocation();
		if (const UCharacterMovementComponent* Movement = FWCharacter->GetCharacterMovement())
		{
			Snapshot.PlayerSpeed = FWCharacter->GetVelocity().Size2D();
			Snapshot.PlayerMaxWalkSpeed = Movement->MaxWalkSpeed;
			Snapshot.bPlayerFalling = Movement->IsFalling();
			Snapshot.bPlayerCrouched = Movement->IsCrouching();
		}
		if (const AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(FWCharacter))
		{
			Snapshot.bPlayerCrawling = PlayerCharacter->IsCrawling();
			Snapshot.PlayerMovementInputDebug = PlayerCharacter->GetMovementInputDebugText();
			Snapshot.bFirstPersonCamera = PlayerCharacter->IsFirstPersonCamera();
			if (AActor* InteractableActor = PlayerCharacter->GetCurrentInteractableActor())
			{
				FString InteractionText = InteractableActor->GetName();
				if (InteractableActor->GetClass()->ImplementsInterface(UFWInteractable::StaticClass()))
				{
					InteractionText = IFWInteractable::Execute_GetInteractionText(InteractableActor).ToString();
				}
				Snapshot.InteractionDebugText = InteractionText;
			}
		}
		if (const UFWHealthComponent* Health = FWCharacter->GetHealthComponent())
		{
			Snapshot.PlayerHealth = Health->CurrentHealth;
			Snapshot.PlayerMaxHealth = Health->MaxHealth;
		}
		if (const UFWStateMachineComponent* StateMachine = FWCharacter->GetStateMachineComponent())
		{
			Snapshot.PlayerStateTag = StateMachine->CurrentState;
		}
		if (const AFWWeaponBase* Weapon = FWCharacter->GetCurrentWeapon())
		{
			Snapshot.CurrentAmmo = Weapon->GetCurrentAmmo();
			if (const UFWWeaponData* WeaponData = Weapon->GetWeaponData())
			{
				Snapshot.WeaponContentId = WeaponData->ContentId;
			}
		}
	}

	Snapshot.LastCombatDebugText = LastCombatDebugText;
	if (GetWorld())
	{
		const FVector ReferenceLocation = FWCharacter ? FWCharacter->GetActorLocation() : FVector::ZeroVector;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (TActorIterator<AFWAICharacter> It(GetWorld()); It; ++It)
		{
			AFWAICharacter* AICharacter = *It;
			if (!AICharacter)
			{
				continue;
			}

			++Snapshot.AICount;
			const UFWHealthComponent* AIHealth = AICharacter->GetHealthComponent();
			const bool bAlive = !AIHealth || !AIHealth->IsDead();
			if (bAlive)
			{
				++Snapshot.AliveAICount;
			}
			else
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared2D(ReferenceLocation, AICharacter->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				if (AIHealth)
				{
					Snapshot.NearestAIHealth = AIHealth->CurrentHealth;
					Snapshot.NearestAIMaxHealth = AIHealth->MaxHealth;
				}
				if (const UFWStateMachineComponent* AIStateMachine = AICharacter->GetStateMachineComponent())
				{
					Snapshot.NearestAIStateTag = AIStateMachine->CurrentState;
				}
			}
		}

		BestDistanceSquared = TNumericLimits<float>::Max();
		for (TActorIterator<AFWVehicleBase> It(GetWorld()); It; ++It)
		{
			AFWVehicleBase* Vehicle = *It;
			if (!Vehicle)
			{
				continue;
			}

			++Snapshot.VehicleCount;
			if (Vehicle->IsDestroyed())
			{
				continue;
			}

			++Snapshot.ActiveVehicleCount;
			const float DistanceSquared = FVector::DistSquared2D(ReferenceLocation, Vehicle->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				if (const UFWHealthComponent* VehicleHealth = Vehicle->FindComponentByClass<UFWHealthComponent>())
				{
					Snapshot.NearestVehicleHealth = VehicleHealth->CurrentHealth;
					Snapshot.NearestVehicleMaxHealth = VehicleHealth->MaxHealth;
				}
				if (const UFWStateMachineComponent* VehicleStateMachine = Vehicle->FindComponentByClass<UFWStateMachineComponent>())
				{
					Snapshot.NearestVehicleStateTag = VehicleStateMachine->CurrentState;
				}
			}
		}
	}

	const AFWPlayerState* FWPlayerState = GetPlayerState<AFWPlayerState>();
	if (FWPlayerState)
	{
		Snapshot.Lives = FWPlayerState->Lives;
		if (const AFWVehicleBase* CoreVehicle = FWPlayerState->CoreVehicle)
		{
			if (const UFWHealthComponent* CoreHealth = CoreVehicle->FindComponentByClass<UFWHealthComponent>())
			{
				Snapshot.CoreVehicleHealth = CoreHealth->CurrentHealth;
				Snapshot.CoreVehicleMaxHealth = CoreHealth->MaxHealth;
			}
		}
	}

	if (VehiclePawn)
	{
		if (const UFWHealthComponent* VehicleHealth = VehiclePawn->FindComponentByClass<UFWHealthComponent>())
		{
			Snapshot.VehicleHealth = VehicleHealth->CurrentHealth;
			Snapshot.VehicleMaxHealth = VehicleHealth->MaxHealth;
		}
	}

	if (const AFWCompetitiveGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFWCompetitiveGameMode>() : nullptr)
	{
		Snapshot.MatchStateTag = GameMode->GetCurrentMatchStateTag();
	}

	return Snapshot;
}

void AFWPlayerController::ForceRefreshHUD()
{
	RefreshHUDWidget();
}

void AFWPlayerController::CreateHUDWidget()
{
	if (!HUDWidgetClass || HUDWidget || !IsLocalPlayerController())
	{
		return;
	}

	HUDWidget = CreateWidget<UFWHUDWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void AFWPlayerController::RefreshHUDWidget()
{
	if (HUDWidget)
	{
		HUDWidget->SetHUDState(BuildHUDStateSnapshot());
	}
}

AFWAICharacter* AFWPlayerController::FindNearestAICharacter() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	const APawn* ControlledPawn = GetPawn();
	const FVector ReferenceLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	AFWAICharacter* BestAI = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AFWAICharacter> It(GetWorld()); It; ++It)
	{
		AFWAICharacter* AICharacter = *It;
		if (!AICharacter)
		{
			continue;
		}

		if (const UFWHealthComponent* AIHealth = AICharacter->GetHealthComponent())
		{
			if (AIHealth->IsDead())
			{
				continue;
			}
		}

		const float DistanceSquared = FVector::DistSquared2D(ReferenceLocation, AICharacter->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestAI = AICharacter;
		}
	}

	return BestAI;
}

AFWVehicleBase* AFWPlayerController::FindNearestVehicle() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	const APawn* ControlledPawn = GetPawn();
	const FVector ReferenceLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	AFWVehicleBase* BestVehicle = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AFWVehicleBase> It(GetWorld()); It; ++It)
	{
		AFWVehicleBase* Vehicle = *It;
		if (!Vehicle || Vehicle->IsDestroyed())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(ReferenceLocation, Vehicle->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestVehicle = Vehicle;
		}
	}

	return BestVehicle;
}

void AFWPlayerController::HandleVehicleExitKeyFallback()
{
	if (AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(GetPawn()))
	{
		VehiclePawn->ExitVehicle();
		ForceRefreshHUD();
	}
}

void AFWPlayerController::StartVehicleInteractionEvidenceSequence()
{
	if (!GetWorld() || !IsLocalPlayerController())
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("FWPIEVehicleInteractionEvidence")))
	{
		return;
	}

	VehicleInteractionEvidenceStep = 0;
	bVehicleInteractionEvidenceActive = true;
	VehicleInteractionEvidenceStartTime = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Display, TEXT("FW PIE-09 vehicle interaction evidence sequence started. WorldType=%d"), static_cast<int32>(GetWorld()->WorldType));
	VehicleInteractionEvidenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
	{
		TickVehicleInteractionEvidenceSequence();
		return bVehicleInteractionEvidenceActive;
	}));
}

void AFWPlayerController::AdvanceVehicleInteractionEvidenceSequence()
{
	if (!GetWorld())
	{
		return;
	}

	++VehicleInteractionEvidenceStep;
	if (VehicleInteractionEvidenceStep == 1)
	{
		if (AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(GetPawn()))
		{
			PlayerCharacter->UpdateInteractionTarget();
			LastCombatDebugText = TEXT("PIE-09 ready for simulated E enter");
			UE_LOG(LogTemp, Display, TEXT("FW PIE-09 evidence step 1 ready. Interactable=%s"), *GetNameSafe(PlayerCharacter->GetCurrentInteractableActor()));
			ForceRefreshHUD();
		}
	}
	else if (VehicleInteractionEvidenceStep == 4)
	{
		SimulateInteractKeyForEvidence();
		LastCombatDebugText = TEXT("PIE-09 simulated E enter");
		UE_LOG(LogTemp, Display, TEXT("FW PIE-09 evidence step 4 simulated E enter. Pawn=%s"), *GetNameSafe(GetPawn()));
		ForceRefreshHUD();
	}
	else if (VehicleInteractionEvidenceStep == 7)
	{
		SimulateInteractKeyForEvidence();
		LastCombatDebugText = TEXT("PIE-09 simulated E exit");
		UE_LOG(LogTemp, Display, TEXT("FW PIE-09 evidence step 7 simulated E exit. Pawn=%s"), *GetNameSafe(GetPawn()));
		ForceRefreshHUD();
		GetWorldTimerManager().ClearTimer(VehicleInteractionEvidenceTimerHandle);
	}
}

void AFWPlayerController::TickVehicleInteractionEvidenceSequence()
{
	if (!bVehicleInteractionEvidenceActive)
	{
		return;
	}

	const double ElapsedSeconds = FPlatformTime::Seconds() - VehicleInteractionEvidenceStartTime;
	if (VehicleInteractionEvidenceStep == 0 && ElapsedSeconds >= 2.0)
	{
		VehicleInteractionEvidenceStep = 1;
		AdvanceVehicleInteractionEvidenceSequence();
	}
	else if (VehicleInteractionEvidenceStep == 1 && ElapsedSeconds >= 5.0)
	{
		VehicleInteractionEvidenceStep = 3;
		AdvanceVehicleInteractionEvidenceSequence();
	}
	else if (VehicleInteractionEvidenceStep == 4 && ElapsedSeconds >= 8.0)
	{
		VehicleInteractionEvidenceStep = 6;
		AdvanceVehicleInteractionEvidenceSequence();
		bVehicleInteractionEvidenceActive = false;
		FTSTicker::GetCoreTicker().RemoveTicker(VehicleInteractionEvidenceTickerHandle);
	}
}

void AFWPlayerController::SimulateInteractKeyForEvidence()
{
	const FInputDeviceId InputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	FInputKeyEventArgs PressedArgs = FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Pressed, 1.0f, -1, InputDevice);
	InputKey(PressedArgs);
	FInputKeyEventArgs ReleasedArgs = FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Released, 0.0f, -1, InputDevice);
	InputKey(ReleasedArgs);
}

void AFWPlayerController::DebugFireNearestAI()
{
	if (!GetWorld() || GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}

	AFWCharacterBase* PlayerCharacter = Cast<AFWCharacterBase>(GetPawn());
	AFWWeaponBase* Weapon = PlayerCharacter ? PlayerCharacter->GetCurrentWeapon() : nullptr;
	AFWAICharacter* TargetAI = FindNearestAICharacter();
	if (!PlayerCharacter || !Weapon || !TargetAI)
	{
		LastCombatDebugText = TEXT("F9 failed: missing player, weapon, or AI");
		ForceRefreshHUD();
		return;
	}

	const UFWHealthComponent* TargetHealth = TargetAI->GetHealthComponent();
	const float BeforeHealth = TargetHealth ? TargetHealth->CurrentHealth : 0.0f;
	const bool bFired = Weapon->FireAtActor(TargetAI);
	const float AfterHealth = TargetHealth ? TargetHealth->CurrentHealth : 0.0f;
	LastCombatDebugText = FString::Printf(
		TEXT("F9 nearest AI fired=%s %.0f->%.0f ammo=%d"),
		bFired ? TEXT("Yes") : TEXT("No"),
		BeforeHealth,
		AfterHealth,
		Weapon->GetCurrentAmmo());
	ForceRefreshHUD();
}

void AFWPlayerController::DebugFireNearestVehicle()
{
	if (!GetWorld() || GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}

	AFWCharacterBase* PlayerCharacter = Cast<AFWCharacterBase>(GetPawn());
	AFWWeaponBase* Weapon = PlayerCharacter ? PlayerCharacter->GetCurrentWeapon() : nullptr;
	AFWVehicleBase* TargetVehicle = FindNearestVehicle();
	if (!PlayerCharacter || !Weapon || !TargetVehicle)
	{
		LastCombatDebugText = TEXT("F10 failed: missing player, weapon, or vehicle");
		ForceRefreshHUD();
		return;
	}

	const UFWHealthComponent* TargetHealth = TargetVehicle->FindComponentByClass<UFWHealthComponent>();
	const float BeforeHealth = TargetHealth ? TargetHealth->CurrentHealth : 0.0f;
	const bool bFired = Weapon->FireAtActor(TargetVehicle);
	const float AfterHealth = TargetHealth ? TargetHealth->CurrentHealth : 0.0f;
	LastCombatDebugText = FString::Printf(
		TEXT("F10 nearest vehicle fired=%s %.0f->%.0f ammo=%d"),
		bFired ? TEXT("Yes") : TEXT("No"),
		BeforeHealth,
		AfterHealth,
		Weapon->GetCurrentAmmo());
	ForceRefreshHUD();
}
