#include "FWPlayerController.h"

#include "../AI/FWEnemyAIController.h"
#include "../Characters/FWAICharacter.h"
#include "../Characters/FWCharacterBase.h"
#include "../Characters/FWPlayerCharacter.h"
#include "../Combat/FWHitZoneComponent.h"
#include "../Combat/FWHealthComponent.h"
#include "../Core/FWCompetitiveGameMode.h"
#include "../Debug/FWDebugSubsystem.h"
#include "../Events/FWEventSubsystem.h"
#include "../Events/FWGameplayEvent.h"
#include "../State/FWStateMachineComponent.h"
#include "../UI/FWHUDWidget.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Weapons/FWWeaponBase.h"
#include "../Weapons/FWWeaponData.h"
#include "../World/FWInteractable.h"
#include "FWPlayerState.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "HighResScreenshot.h"
#include "InputCoreTypes.h"
#include "InputActionValue.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

namespace
{
	FString FWJsonEscape(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Escaped.ReplaceInline(TEXT("\r"), TEXT("\\r"));
		Escaped.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		Escaped.ReplaceInline(TEXT("\t"), TEXT("\\t"));
		return Escaped;
	}
}

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
	StartVehicleDriveEvidenceSequence();
	StartVehicleDestructionEvidenceSequence();
	StartAIEvidenceSequence();
	StartDebugEvidenceSequence();
}

void AFWPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickVehicleInteractionEvidenceSequence();
	TickVehicleDriveEvidenceSequence();
	TickVehicleDestructionEvidenceSequence();
	TickAIEvidenceSequence();
	TickDebugEvidenceSequence();

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
		if (VehiclePawn->GetClass()->ImplementsInterface(UFWInteractable::StaticClass()))
		{
			Snapshot.InteractionDebugText = IFWInteractable::Execute_GetInteractionText(VehiclePawn).ToString();
		}
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
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleInteractionEvidenceFolder="), VehicleInteractionEvidenceFolder);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleInteractionEvidencePrefix="), VehicleInteractionEvidencePrefix);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleInteractionTarget="), VehicleInteractionEvidenceTarget);
	if (!VehicleInteractionEvidenceFolder.IsEmpty())
	{
		const FString EvidencePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(VehicleInteractionEvidenceFolder), FString::Printf(TEXT("%s_state-evidence.jsonl"), *VehicleInteractionEvidencePrefix));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidencePath), true);
		const FString Header = FString::Printf(TEXT("{\"event\":\"PIE vehicle interaction evidence start\",\"prefix\":\"%s\",\"target\":\"%s\",\"worldType\":%d}\n"), *FWJsonEscape(VehicleInteractionEvidencePrefix), *FWJsonEscape(VehicleInteractionEvidenceTarget), static_cast<int32>(GetWorld()->WorldType));
		FFileHelper::SaveStringToFile(Header, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
	UE_LOG(LogTemp, Display, TEXT("FW vehicle interaction evidence sequence started. Target=%s WorldType=%d"), *VehicleInteractionEvidenceTarget, static_cast<int32>(GetWorld()->WorldType));
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
		PositionPlayerForVehicleInteractionEvidence();
		if (AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(GetPawn()))
		{
			PlayerCharacter->UpdateInteractionTarget();
			LastCombatDebugText = FString::Printf(TEXT("Vehicle interaction ready for %s enter"), *VehicleInteractionEvidenceTarget);
			UE_LOG(LogTemp, Display, TEXT("FW vehicle interaction evidence step 1 ready. Target=%s Interactable=%s"), *VehicleInteractionEvidenceTarget, *GetNameSafe(PlayerCharacter->GetCurrentInteractableActor()));
			ForceRefreshHUD();
			WriteVehicleInteractionEvidenceState(TEXT("00-enter-prompt"));
			CaptureVehicleInteractionEvidenceScreenshot(TEXT("00-enter-prompt"));
		}
	}
	else if (VehicleInteractionEvidenceStep == 2)
	{
		TriggerVehicleInteractionForEvidence();
		LastCombatDebugText = FString::Printf(TEXT("Vehicle interaction simulated %s enter"), *VehicleInteractionEvidenceTarget);
		UE_LOG(LogTemp, Display, TEXT("FW vehicle interaction evidence step 2 simulated E enter. Target=%s Pawn=%s"), *VehicleInteractionEvidenceTarget, *GetNameSafe(GetPawn()));
		ForceRefreshHUD();
		WriteVehicleInteractionEvidenceState(TEXT("01-entered-car"));
		CaptureVehicleInteractionEvidenceScreenshot(TEXT("01-entered-car"));
	}
	else if (VehicleInteractionEvidenceStep == 3)
	{
		TriggerVehicleInteractionForEvidence();
		LastCombatDebugText = FString::Printf(TEXT("Vehicle interaction simulated %s exit"), *VehicleInteractionEvidenceTarget);
		UE_LOG(LogTemp, Display, TEXT("FW vehicle interaction evidence step 3 simulated E exit. Target=%s Pawn=%s"), *VehicleInteractionEvidenceTarget, *GetNameSafe(GetPawn()));
		ForceRefreshHUD();
		WriteVehicleInteractionEvidenceState(TEXT("02-exited-car"));
		CaptureVehicleInteractionEvidenceScreenshot(TEXT("02-exited-car"));
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
		AdvanceVehicleInteractionEvidenceSequence();
	}
	else if (VehicleInteractionEvidenceStep == 1 && ElapsedSeconds >= 5.0)
	{
		AdvanceVehicleInteractionEvidenceSequence();
	}
	else if (VehicleInteractionEvidenceStep == 2 && ElapsedSeconds >= 8.0)
	{
		AdvanceVehicleInteractionEvidenceSequence();
		bVehicleInteractionEvidenceActive = false;
		FTSTicker::GetCoreTicker().RemoveTicker(VehicleInteractionEvidenceTickerHandle);
	}
}

void AFWPlayerController::TriggerVehicleInteractionForEvidence()
{
	if (AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->UpdateInteractionTarget();
		PlayerCharacter->Interact();
		return;
	}

	if (AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(GetPawn()))
	{
		VehiclePawn->ExitVehicle();
	}
}

void AFWPlayerController::CaptureVehicleInteractionEvidenceScreenshot(const FString& StepSuffix) const
{
	if (VehicleInteractionEvidenceFolder.IsEmpty())
	{
		return;
	}

	const FString AbsoluteFolder = FPaths::ConvertRelativePathToFull(VehicleInteractionEvidenceFolder);
	IFileManager::Get().MakeDirectory(*AbsoluteFolder, true);
	const FString ScreenshotPath = FPaths::Combine(AbsoluteFolder, FString::Printf(TEXT("%s_%s.png"), *VehicleInteractionEvidencePrefix, *StepSuffix));
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		GetHighResScreenshotConfig().SetFilename(ScreenshotPath);
		GEngine->GameViewport->Viewport->TakeHighResScreenShot();
		UE_LOG(LogTemp, Display, TEXT("FW PIE-09 requested high-res viewport screenshot: %s"), *ScreenshotPath);
		return;
	}

	FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
	UE_LOG(LogTemp, Display, TEXT("FW PIE-09 requested fallback screenshot: %s"), *ScreenshotPath);
}

void AFWPlayerController::WriteVehicleInteractionEvidenceState(const FString& StepSuffix) const
{
	if (VehicleInteractionEvidenceFolder.IsEmpty())
	{
		return;
	}

	const FString AbsoluteFolder = FPaths::ConvertRelativePathToFull(VehicleInteractionEvidenceFolder);
	IFileManager::Get().MakeDirectory(*AbsoluteFolder, true);
	const FString EvidencePath = FPaths::Combine(AbsoluteFolder, FString::Printf(TEXT("%s_state-evidence.jsonl"), *VehicleInteractionEvidencePrefix));
	const FFWHUDStateSnapshot Snapshot = BuildHUDStateSnapshot();
	const APawn* CurrentPawn = GetPawn();
	const AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(CurrentPawn);
	const AFWVehicleBase* TargetVehicle = FindVehicleInteractionEvidenceTarget();
	const FString Line = FString::Printf(
		TEXT("{\"event\":\"PIE vehicle interaction evidence step\",\"target\":\"%s\",\"step\":\"%s\",\"stepIndex\":%d,\"pawn\":\"%s\",\"pawnClass\":\"%s\",\"targetVehicle\":\"%s\",\"targetVehicleClass\":\"%s\",\"isVehiclePawn\":%s,\"interaction\":\"%s\",\"playerHealth\":%.2f,\"playerMaxHealth\":%.2f,\"vehicleHealth\":%.2f,\"vehicleMaxHealth\":%.2f,\"nearestVehicleHealth\":%.2f,\"nearestVehicleMaxHealth\":%.2f,\"lastCombat\":\"%s\"}\n"),
		*FWJsonEscape(VehicleInteractionEvidenceTarget),
		*FWJsonEscape(StepSuffix),
		VehicleInteractionEvidenceStep,
		*FWJsonEscape(GetNameSafe(CurrentPawn)),
		CurrentPawn ? *FWJsonEscape(CurrentPawn->GetClass()->GetName()) : TEXT(""),
		*FWJsonEscape(GetNameSafe(TargetVehicle)),
		TargetVehicle ? *FWJsonEscape(TargetVehicle->GetClass()->GetName()) : TEXT(""),
		VehiclePawn ? TEXT("true") : TEXT("false"),
		*FWJsonEscape(Snapshot.InteractionDebugText),
		Snapshot.PlayerHealth,
		Snapshot.PlayerMaxHealth,
		Snapshot.VehicleHealth,
		Snapshot.VehicleMaxHealth,
		Snapshot.NearestVehicleHealth,
		Snapshot.NearestVehicleMaxHealth,
		*FWJsonEscape(Snapshot.LastCombatDebugText));

	FFileHelper::SaveStringToFile(Line, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogTemp, Display, TEXT("FW vehicle interaction wrote state evidence: %s target=%s step=%s"), *EvidencePath, *VehicleInteractionEvidenceTarget, *StepSuffix);
}

AFWVehicleBase* AFWPlayerController::FindVehicleInteractionEvidenceTarget() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AFWVehicleBase* BestVehicle = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const APawn* ControlledPawn = GetPawn();
	const FVector ReferenceLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	for (TActorIterator<AFWVehicleBase> It(GetWorld()); It; ++It)
	{
		AFWVehicleBase* Vehicle = *It;
		if (!Vehicle || Vehicle->IsDestroyed())
		{
			continue;
		}

		const FString VehicleClassName = Vehicle->GetClass()->GetName();
		const bool bMatchesTarget = VehicleInteractionEvidenceTarget.Equals(TEXT("Motorcycle"), ESearchCase::IgnoreCase)
			? VehicleClassName.Contains(TEXT("Motorcycle"), ESearchCase::IgnoreCase)
			: VehicleClassName.Contains(TEXT("Car"), ESearchCase::IgnoreCase);
		if (!bMatchesTarget)
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

void AFWPlayerController::PositionPlayerForVehicleInteractionEvidence()
{
	AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(GetPawn());
	AFWVehicleBase* TargetVehicle = FindVehicleInteractionEvidenceTarget();
	if (!PlayerCharacter || !TargetVehicle)
	{
		UE_LOG(LogTemp, Warning, TEXT("FW vehicle interaction evidence could not position player. Player=%s Target=%s"), *GetNameSafe(PlayerCharacter), *GetNameSafe(TargetVehicle));
		return;
	}

	const FVector VehicleLocation = TargetVehicle->GetActorLocation();
	const FVector Offset = -TargetVehicle->GetActorForwardVector().GetSafeNormal2D() * 250.0f;
	const FVector TargetLocation = VehicleLocation + Offset + FVector(0.0f, 0.0f, 30.0f);
	const FRotator TargetRotation = (VehicleLocation - TargetLocation).Rotation();
	PlayerCharacter->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetControlRotation(FRotator(-5.0f, TargetRotation.Yaw, 0.0f));
	PlayerCharacter->UpdateInteractionTarget();
	UE_LOG(LogTemp, Display, TEXT("FW vehicle interaction evidence positioned player for %s. Target=%s Interactable=%s"), *VehicleInteractionEvidenceTarget, *GetNameSafe(TargetVehicle), *GetNameSafe(PlayerCharacter->GetCurrentInteractableActor()));
}

void AFWPlayerController::StartVehicleDriveEvidenceSequence()
{
	if (!GetWorld() || !IsLocalPlayerController())
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("FWPIEVehicleDriveEvidence")))
	{
		return;
	}

	VehicleDriveEvidenceStep = 0;
	bVehicleDriveEvidenceActive = true;
	VehicleDriveEvidenceStartTime = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleDriveEvidenceFolder="), VehicleDriveEvidenceFolder);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleDriveEvidencePrefix="), VehicleDriveEvidencePrefix);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleDriveTarget="), VehicleDriveEvidenceTarget);
	if (!VehicleDriveEvidenceFolder.IsEmpty())
	{
		const FString EvidencePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(VehicleDriveEvidenceFolder), FString::Printf(TEXT("%s_state-evidence.jsonl"), *VehicleDriveEvidencePrefix));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidencePath), true);
		const FString Header = FString::Printf(TEXT("{\"event\":\"PIE vehicle drive evidence start\",\"prefix\":\"%s\",\"target\":\"%s\",\"worldType\":%d}\n"), *FWJsonEscape(VehicleDriveEvidencePrefix), *FWJsonEscape(VehicleDriveEvidenceTarget), static_cast<int32>(GetWorld()->WorldType));
		FFileHelper::SaveStringToFile(Header, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	UE_LOG(LogTemp, Display, TEXT("FW vehicle drive evidence sequence started. Target=%s WorldType=%d"), *VehicleDriveEvidenceTarget, static_cast<int32>(GetWorld()->WorldType));
	VehicleDriveEvidenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
	{
		TickVehicleDriveEvidenceSequence();
		return bVehicleDriveEvidenceActive;
	}));
}

void AFWPlayerController::AdvanceVehicleDriveEvidenceSequence()
{
	if (!GetWorld())
	{
		return;
	}

	++VehicleDriveEvidenceStep;
	AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(GetPawn());
	if (VehicleDriveEvidenceStep == 1)
	{
		PositionPlayerForVehicleDriveEvidence();
		if (AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(GetPawn()))
		{
			PlayerCharacter->UpdateInteractionTarget();
			PlayerCharacter->Interact();
			VehiclePawn = Cast<AFWVehicleBase>(GetPawn());
		}
		VehicleDriveEvidenceStartLocation = VehiclePawn ? VehiclePawn->GetActorLocation() : FVector::ZeroVector;
		VehicleDriveEvidenceStartRotation = VehiclePawn ? VehiclePawn->GetActorRotation() : FRotator::ZeroRotator;
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence entered %s"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("00-entered"), TEXT("entered"));
	}
	else if (VehicleDriveEvidenceStep == 2 && VehiclePawn)
	{
		VehiclePawn->Move(FInputActionValue(FVector2D(0.0f, 1.0f)));
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s throttle forward"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("01-throttle-forward-start"), TEXT("throttle-forward-start"));
	}
	else if (VehicleDriveEvidenceStep == 3 && VehiclePawn)
	{
		PumpVehicleDriveEvidence(1.0f);
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s throttle forward result"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("02-throttle-forward-result"), TEXT("throttle-forward-result"));
	}
	else if (VehicleDriveEvidenceStep == 4 && VehiclePawn)
	{
		VehiclePawn->Move(FInputActionValue(FVector2D(0.0f, -1.0f)));
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s reverse brake"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("03-reverse-start"), TEXT("reverse-start"));
	}
	else if (VehicleDriveEvidenceStep == 5 && VehiclePawn)
	{
		PumpVehicleDriveEvidence(1.0f);
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s reverse result"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("04-reverse-result"), TEXT("reverse-result"));
	}
	else if (VehicleDriveEvidenceStep == 6 && VehiclePawn)
	{
		VehiclePawn->Move(FInputActionValue(FVector2D(1.0f, 1.0f)));
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s steering"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("05-steering-start"), TEXT("steering-start"));
	}
	else if (VehicleDriveEvidenceStep == 7 && VehiclePawn)
	{
		PumpVehicleDriveEvidence(1.0f);
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s steering result"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("06-steering-result"), TEXT("steering-result"));
	}
	else if (VehicleDriveEvidenceStep == 8 && VehiclePawn)
	{
		VehiclePawn->Look(FInputActionValue(FVector2D(18.0f, -8.0f)));
		const FRotator CurrentViewRotation = GetControlRotation();
		SetControlRotation(FRotator(CurrentViewRotation.Pitch - 8.0f, CurrentViewRotation.Yaw + 18.0f, 0.0f));
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s camera look"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("07-camera-look"), TEXT("camera-look"));
	}
	else if (VehicleDriveEvidenceStep == 9 && VehiclePawn)
	{
		VehiclePawn->Move(FInputActionValue(FVector2D::ZeroVector));
		VehiclePawn->SetActorLocationAndRotation(FVector(297.5f, -500.0f, VehiclePawn->GetActorLocation().Z), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);

		FHitResult Hit;
		const FVector ProbeDelta = VehiclePawn->GetActorForwardVector() * 5000.0f;
		VehiclePawn->AddActorWorldOffset(ProbeDelta, true, &Hit, ETeleportType::None);
		LastCombatDebugText = FString::Printf(TEXT("Vehicle drive evidence %s collision probe"), *VehicleDriveEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDriveEvidenceState(TEXT("08-collision-probe"), TEXT("collision-probe"), &Hit);
	}
}

void AFWPlayerController::TickVehicleDriveEvidenceSequence()
{
	if (!bVehicleDriveEvidenceActive)
	{
		return;
	}

	const double ElapsedSeconds = FPlatformTime::Seconds() - VehicleDriveEvidenceStartTime;
	constexpr double StepTimes[] = { 2.0, 4.0, 6.0, 7.5, 9.5, 11.0, 13.0, 14.0, 15.5 };
	if (VehicleDriveEvidenceStep < UE_ARRAY_COUNT(StepTimes) && ElapsedSeconds >= StepTimes[VehicleDriveEvidenceStep])
	{
		AdvanceVehicleDriveEvidenceSequence();
		if (VehicleDriveEvidenceStep >= UE_ARRAY_COUNT(StepTimes))
		{
			bVehicleDriveEvidenceActive = false;
			FTSTicker::GetCoreTicker().RemoveTicker(VehicleDriveEvidenceTickerHandle);
		}
	}
}

AFWVehicleBase* AFWPlayerController::FindVehicleDriveEvidenceTarget() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AFWVehicleBase* BestVehicle = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const APawn* ControlledPawn = GetPawn();
	const FVector ReferenceLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	for (TActorIterator<AFWVehicleBase> It(GetWorld()); It; ++It)
	{
		AFWVehicleBase* Vehicle = *It;
		if (!Vehicle || Vehicle->IsDestroyed())
		{
			continue;
		}

		const FString VehicleClassName = Vehicle->GetClass()->GetName();
		const bool bMatchesTarget = VehicleDriveEvidenceTarget.Equals(TEXT("Motorcycle"), ESearchCase::IgnoreCase)
			? VehicleClassName.Contains(TEXT("Motorcycle"), ESearchCase::IgnoreCase)
			: VehicleClassName.Contains(TEXT("Car"), ESearchCase::IgnoreCase);
		if (!bMatchesTarget)
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

void AFWPlayerController::WriteVehicleDriveEvidenceState(const FString& StepSuffix, const FString& ActionLabel, const FHitResult* CollisionHit) const
{
	if (VehicleDriveEvidenceFolder.IsEmpty())
	{
		return;
	}

	const AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(GetPawn());
	const UBoxComponent* Collision = VehiclePawn ? VehiclePawn->GetVehicleCollision() : nullptr;
	const FVector CurrentLocation = VehiclePawn ? VehiclePawn->GetActorLocation() : FVector::ZeroVector;
	const FRotator CurrentRotation = VehiclePawn ? VehiclePawn->GetActorRotation() : FRotator::ZeroRotator;
	const FRotator CurrentControlRotation = GetControlRotation();
	const FString AbsoluteFolder = FPaths::ConvertRelativePathToFull(VehicleDriveEvidenceFolder);
	IFileManager::Get().MakeDirectory(*AbsoluteFolder, true);
	const FString EvidencePath = FPaths::Combine(AbsoluteFolder, FString::Printf(TEXT("%s_state-evidence.jsonl"), *VehicleDriveEvidencePrefix));
	const FString Line = FString::Printf(
		TEXT("{\"event\":\"PIE vehicle drive evidence step\",\"target\":\"%s\",\"step\":\"%s\",\"stepIndex\":%d,\"action\":\"%s\",\"pawn\":\"%s\",\"pawnClass\":\"%s\",\"locationX\":%.2f,\"locationY\":%.2f,\"distanceFromStart2D\":%.2f,\"yaw\":%.2f,\"yawDeltaFromStart\":%.2f,\"controlPitch\":%.2f,\"controlYaw\":%.2f,\"forwardSpeed\":%.2f,\"throttleInput\":%.2f,\"steeringInput\":%.2f,\"collisionEnabled\":\"%s\",\"collisionObject\":\"%s\",\"blocksWorldStatic\":%s,\"blocksPawn\":%s,\"collisionProbeBlockingHit\":%s,\"collisionProbeActor\":\"%s\",\"lastCombat\":\"%s\"}\n"),
		*FWJsonEscape(VehicleDriveEvidenceTarget),
		*FWJsonEscape(StepSuffix),
		VehicleDriveEvidenceStep,
		*FWJsonEscape(ActionLabel),
		*FWJsonEscape(GetNameSafe(VehiclePawn)),
		VehiclePawn ? *FWJsonEscape(VehiclePawn->GetClass()->GetName()) : TEXT(""),
		CurrentLocation.X,
		CurrentLocation.Y,
		FVector::Dist2D(CurrentLocation, VehicleDriveEvidenceStartLocation),
		CurrentRotation.Yaw,
		FMath::FindDeltaAngleDegrees(VehicleDriveEvidenceStartRotation.Yaw, CurrentRotation.Yaw),
		CurrentControlRotation.Pitch,
		CurrentControlRotation.Yaw,
		VehiclePawn ? VehiclePawn->GetCurrentForwardSpeed() : 0.0f,
		VehiclePawn ? VehiclePawn->GetThrottleInput() : 0.0f,
		VehiclePawn ? VehiclePawn->GetSteeringInput() : 0.0f,
		Collision ? *FWJsonEscape(UEnum::GetValueAsString(Collision->GetCollisionEnabled())) : TEXT(""),
		Collision ? *FWJsonEscape(UEnum::GetValueAsString(Collision->GetCollisionObjectType())) : TEXT(""),
		Collision && Collision->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block ? TEXT("true") : TEXT("false"),
		Collision && Collision->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block ? TEXT("true") : TEXT("false"),
		CollisionHit && CollisionHit->bBlockingHit ? TEXT("true") : TEXT("false"),
		CollisionHit && CollisionHit->bBlockingHit ? *FWJsonEscape(GetNameSafe(CollisionHit->GetActor())) : TEXT(""),
		*FWJsonEscape(LastCombatDebugText));

	FFileHelper::SaveStringToFile(Line, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogTemp, Display, TEXT("FW vehicle drive wrote state evidence: %s target=%s step=%s"), *EvidencePath, *VehicleDriveEvidenceTarget, *StepSuffix);
}

void AFWPlayerController::PositionPlayerForVehicleDriveEvidence()
{
	AFWPlayerCharacter* PlayerCharacter = Cast<AFWPlayerCharacter>(GetPawn());
	AFWVehicleBase* TargetVehicle = FindVehicleDriveEvidenceTarget();
	if (!PlayerCharacter || !TargetVehicle)
	{
		UE_LOG(LogTemp, Warning, TEXT("FW vehicle drive evidence could not position player. Player=%s Target=%s"), *GetNameSafe(PlayerCharacter), *GetNameSafe(TargetVehicle));
		return;
	}

	const FVector VehicleLocation = TargetVehicle->GetActorLocation();
	TargetVehicle->SetActorLocationAndRotation(FVector(0.0f, -2500.0f, VehicleLocation.Z + 20.0f), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	TargetVehicle->Move(FInputActionValue(FVector2D::ZeroVector));
	const FVector RepositionedVehicleLocation = TargetVehicle->GetActorLocation();
	const FVector Offset = -TargetVehicle->GetActorForwardVector().GetSafeNormal2D() * 250.0f;
	const FVector TargetLocation = RepositionedVehicleLocation + Offset + FVector(0.0f, 0.0f, 30.0f);
	const FRotator TargetRotation = (RepositionedVehicleLocation - TargetLocation).Rotation();
	PlayerCharacter->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	SetControlRotation(FRotator(-5.0f, TargetRotation.Yaw, 0.0f));
	PlayerCharacter->UpdateInteractionTarget();
	UE_LOG(LogTemp, Display, TEXT("FW vehicle drive evidence positioned player for %s. Target=%s Interactable=%s"), *VehicleDriveEvidenceTarget, *GetNameSafe(TargetVehicle), *GetNameSafe(PlayerCharacter->GetCurrentInteractableActor()));
}

void AFWPlayerController::PumpVehicleDriveEvidence(float TotalSeconds, float StepSeconds) const
{
	AFWVehicleBase* VehiclePawn = Cast<AFWVehicleBase>(GetPawn());
	if (!VehiclePawn)
	{
		return;
	}

	const int32 StepCount = FMath::Max(1, FMath::CeilToInt(TotalSeconds / FMath::Max(StepSeconds, KINDA_SMALL_NUMBER)));
	for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
	{
		VehiclePawn->Tick(StepSeconds);
	}
}

void AFWPlayerController::StartVehicleDestructionEvidenceSequence()
{
	if (!GetWorld() || !IsLocalPlayerController())
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("FWPIEVehicleDestructionEvidence")))
	{
		return;
	}

	VehicleDestructionEvidenceStep = 0;
	bVehicleDestructionEvidenceActive = true;
	bVehicleDestructionEvidenceEndMatch = FParse::Param(FCommandLine::Get(), TEXT("FWPIEVehicleDestructionEndMatchEvidence"));
	VehicleDestructionEvidenceStartTime = GetWorld()->GetTimeSeconds();
	VehicleDestructionEvidenceDelegateCount = 0;
	VehicleDestructionEvidenceGameplayEventCount = 0;
	VehicleDestructionEvidenceRespawnEventCount = 0;
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleDestructionEvidenceFolder="), VehicleDestructionEvidenceFolder);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleDestructionEvidencePrefix="), VehicleDestructionEvidencePrefix);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEVehicleDestructionTarget="), VehicleDestructionEvidenceTarget);

	if (!VehicleDestructionEvidenceFolder.IsEmpty())
	{
		const FString EvidencePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(VehicleDestructionEvidenceFolder), FString::Printf(TEXT("%s_state-evidence.jsonl"), *VehicleDestructionEvidencePrefix));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidencePath), true);
		const FString Header = FString::Printf(TEXT("{\"event\":\"PIE vehicle destruction evidence start\",\"prefix\":\"%s\",\"target\":\"%s\",\"worldType\":%d}\n"), *FWJsonEscape(VehicleDestructionEvidencePrefix), *FWJsonEscape(VehicleDestructionEvidenceTarget), static_cast<int32>(GetWorld()->WorldType));
		FFileHelper::SaveStringToFile(Header, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>())
		{
			EventSubsystem->OnGameplayEvent.AddUniqueDynamic(this, &AFWPlayerController::HandleVehicleDestructionEvidenceGameplayEvent);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("FW vehicle destruction evidence sequence started. Target=%s WorldType=%d"), *VehicleDestructionEvidenceTarget, static_cast<int32>(GetWorld()->WorldType));
	VehicleDestructionEvidenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
	{
		TickVehicleDestructionEvidenceSequence();
		return bVehicleDestructionEvidenceActive;
	}));
}

void AFWPlayerController::AdvanceVehicleDestructionEvidenceSequence()
{
	if (!GetWorld())
	{
		return;
	}

	++VehicleDestructionEvidenceStep;
	if (VehicleDestructionEvidenceStep == 1)
	{
		VehicleDestructionEvidenceTargetActor = FindVehicleDestructionEvidenceTarget();
		if (VehicleDestructionEvidenceTargetActor)
		{
			VehicleDestructionEvidenceTargetActor->OnVehicleDestroyed.AddUniqueDynamic(this, &AFWPlayerController::HandleVehicleDestructionEvidenceDestroyed);
			VehicleDestructionEvidenceTargetActor->SetActorLocationAndRotation(FVector(0.0f, 1800.0f, VehicleDestructionEvidenceTargetActor->GetActorLocation().Z + 20.0f), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (bVehicleDestructionEvidenceEndMatch)
		{
			if (AFWPlayerState* FWPlayerState = GetPlayerState<AFWPlayerState>())
			{
				FWPlayerState->SetLives(1);
			}
		}
		LastCombatDebugText = bVehicleDestructionEvidenceEndMatch
			? FString::Printf(TEXT("Vehicle destruction end-match evidence ready for %s"), *VehicleDestructionEvidenceTarget)
			: FString::Printf(TEXT("Vehicle destruction evidence ready for %s"), *VehicleDestructionEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDestructionEvidenceState(TEXT("00-before-destruction"), TEXT("before-destruction"));
	}
	else if (VehicleDestructionEvidenceStep == 2)
	{
		if (AFWVehicleBase* TargetVehicle = VehicleDestructionEvidenceTargetActor.Get())
		{
			if (UFWHealthComponent* VehicleHealth = TargetVehicle->FindComponentByClass<UFWHealthComponent>())
			{
				VehicleHealth->ApplyDamage(VehicleHealth->MaxHealth + 100.0f);
			}
			TargetVehicle->Tick(0.016f);
		}
		LastCombatDebugText = FString::Printf(TEXT("Vehicle destruction evidence destroyed %s once"), *VehicleDestructionEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDestructionEvidenceState(TEXT("01-after-first-destruction"), TEXT("after-first-destruction"));
	}
	else if (VehicleDestructionEvidenceStep == 3)
	{
		if (AFWVehicleBase* TargetVehicle = VehicleDestructionEvidenceTargetActor.Get())
		{
			TargetVehicle->HandleVehicleDestroyed();
			TargetVehicle->Tick(0.016f);
		}
		LastCombatDebugText = FString::Printf(TEXT("Vehicle destruction evidence repeated %s destroy attempt"), *VehicleDestructionEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDestructionEvidenceState(TEXT("02-after-repeat-destruction"), TEXT("after-repeat-destruction"));
	}
	else if (VehicleDestructionEvidenceStep == 4)
	{
		LastCombatDebugText = bVehicleDestructionEvidenceEndMatch
			? FString::Printf(TEXT("Vehicle destruction evidence observed %s end-match presentation"), *VehicleDestructionEvidenceTarget)
			: FString::Printf(TEXT("Vehicle destruction evidence observed %s respawn presentation"), *VehicleDestructionEvidenceTarget);
		ForceRefreshHUD();
		WriteVehicleDestructionEvidenceState(
			bVehicleDestructionEvidenceEndMatch ? TEXT("03-after-end-match") : TEXT("03-after-respawn"),
			bVehicleDestructionEvidenceEndMatch ? TEXT("after-end-match") : TEXT("after-respawn"));
		bVehicleDestructionEvidenceActive = false;
		FTSTicker::GetCoreTicker().RemoveTicker(VehicleDestructionEvidenceTickerHandle);
	}
}

void AFWPlayerController::TickVehicleDestructionEvidenceSequence()
{
	if (!bVehicleDestructionEvidenceActive)
	{
		return;
	}

	const double ElapsedSeconds = GetWorld()->GetTimeSeconds() - VehicleDestructionEvidenceStartTime;
	constexpr double StepTimes[] = { 2.0, 4.0, 6.0, 10.0 };
	if (VehicleDestructionEvidenceStep < UE_ARRAY_COUNT(StepTimes) && ElapsedSeconds >= StepTimes[VehicleDestructionEvidenceStep])
	{
		AdvanceVehicleDestructionEvidenceSequence();
	}
}

AFWVehicleBase* AFWPlayerController::FindVehicleDestructionEvidenceTarget() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AFWVehicleBase* BestVehicle = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const APawn* ControlledPawn = GetPawn();
	const FVector ReferenceLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	for (TActorIterator<AFWVehicleBase> It(GetWorld()); It; ++It)
	{
		AFWVehicleBase* Vehicle = *It;
		if (!Vehicle || Vehicle->IsDestroyed())
		{
			continue;
		}

		const FString VehicleClassName = Vehicle->GetClass()->GetName();
		const bool bMatchesTarget = VehicleDestructionEvidenceTarget.Equals(TEXT("Motorcycle"), ESearchCase::IgnoreCase)
			? VehicleClassName.Contains(TEXT("Motorcycle"), ESearchCase::IgnoreCase)
			: VehicleClassName.Contains(TEXT("Car"), ESearchCase::IgnoreCase);
		if (!bMatchesTarget)
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

void AFWPlayerController::WriteVehicleDestructionEvidenceState(const FString& StepSuffix, const FString& ActionLabel) const
{
	if (VehicleDestructionEvidenceFolder.IsEmpty())
	{
		return;
	}

	const AFWVehicleBase* TargetVehicle = VehicleDestructionEvidenceTargetActor.Get();
	const UFWHealthComponent* VehicleHealth = TargetVehicle ? TargetVehicle->FindComponentByClass<UFWHealthComponent>() : nullptr;
	const UFWStateMachineComponent* StateMachine = TargetVehicle ? TargetVehicle->FindComponentByClass<UFWStateMachineComponent>() : nullptr;
	const AFWPlayerState* FWPlayerState = GetPlayerState<AFWPlayerState>();
	const FFWHUDStateSnapshot Snapshot = BuildHUDStateSnapshot();
	const AFWCompetitiveGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFWCompetitiveGameMode>() : nullptr;
	const APawn* CurrentPawn = GetPawn();
	const FString AbsoluteFolder = FPaths::ConvertRelativePathToFull(VehicleDestructionEvidenceFolder);
	IFileManager::Get().MakeDirectory(*AbsoluteFolder, true);
	const FString EvidencePath = FPaths::Combine(AbsoluteFolder, FString::Printf(TEXT("%s_state-evidence.jsonl"), *VehicleDestructionEvidencePrefix));
	const FString Line = FString::Printf(
		TEXT("{\"event\":\"PIE vehicle destruction evidence step\",\"target\":\"%s\",\"endMatchEvidence\":%s,\"step\":\"%s\",\"stepIndex\":%d,\"action\":\"%s\",\"vehicle\":\"%s\",\"vehicleClass\":\"%s\",\"isCoreVehicle\":%s,\"isDestroyed\":%s,\"health\":%.2f,\"maxHealth\":%.2f,\"isHealthDead\":%s,\"state\":\"%s\",\"playerLives\":%d,\"hudLives\":%d,\"playerHealth\":%.2f,\"playerMaxHealth\":%.2f,\"playerLocationX\":%.2f,\"playerLocationY\":%.2f,\"playerLocationZ\":%.2f,\"playerPawn\":\"%s\",\"playerPawnClass\":\"%s\",\"coreVehicleHealth\":%.2f,\"coreVehicleMaxHealth\":%.2f,\"matchState\":\"%s\",\"delegateCount\":%d,\"gameplayEventCount\":%d,\"respawnEventCount\":%d,\"lastCombat\":\"%s\"}\n"),
		*FWJsonEscape(VehicleDestructionEvidenceTarget),
		bVehicleDestructionEvidenceEndMatch ? TEXT("true") : TEXT("false"),
		*FWJsonEscape(StepSuffix),
		VehicleDestructionEvidenceStep,
		*FWJsonEscape(ActionLabel),
		*FWJsonEscape(GetNameSafe(TargetVehicle)),
		TargetVehicle ? *FWJsonEscape(TargetVehicle->GetClass()->GetName()) : TEXT(""),
		FWPlayerState && TargetVehicle && FWPlayerState->CoreVehicle == TargetVehicle ? TEXT("true") : TEXT("false"),
		TargetVehicle && TargetVehicle->IsDestroyed() ? TEXT("true") : TEXT("false"),
		VehicleHealth ? VehicleHealth->CurrentHealth : 0.0f,
		VehicleHealth ? VehicleHealth->MaxHealth : 0.0f,
		VehicleHealth && VehicleHealth->IsDead() ? TEXT("true") : TEXT("false"),
		StateMachine ? *FWJsonEscape(StateMachine->CurrentState.ToString()) : TEXT(""),
		FWPlayerState ? FWPlayerState->Lives : 0,
		Snapshot.Lives,
		Snapshot.PlayerHealth,
		Snapshot.PlayerMaxHealth,
		Snapshot.PlayerLocation.X,
		Snapshot.PlayerLocation.Y,
		Snapshot.PlayerLocation.Z,
		*FWJsonEscape(GetNameSafe(CurrentPawn)),
		CurrentPawn ? *FWJsonEscape(CurrentPawn->GetClass()->GetName()) : TEXT(""),
		Snapshot.CoreVehicleHealth,
		Snapshot.CoreVehicleMaxHealth,
		GameMode ? *FWJsonEscape(GameMode->GetCurrentMatchStateTag().ToString()) : TEXT(""),
		VehicleDestructionEvidenceDelegateCount,
		VehicleDestructionEvidenceGameplayEventCount,
		VehicleDestructionEvidenceRespawnEventCount,
		*FWJsonEscape(LastCombatDebugText));

	FFileHelper::SaveStringToFile(Line, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogTemp, Display, TEXT("FW vehicle destruction wrote state evidence: %s target=%s step=%s"), *EvidencePath, *VehicleDestructionEvidenceTarget, *StepSuffix);
}

void AFWPlayerController::HandleVehicleDestructionEvidenceDestroyed(AFWVehicleBase* Vehicle)
{
	if (Vehicle && Vehicle == VehicleDestructionEvidenceTargetActor.Get())
	{
		++VehicleDestructionEvidenceDelegateCount;
	}
}

void AFWPlayerController::HandleVehicleDestructionEvidenceGameplayEvent(const FFWGameplayEvent& Event)
{
	const FGameplayTag VehicleDestroyedTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Vehicle.Destroyed"));
	const FGameplayTag CoreVehicleDestroyedTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Vehicle.CoreDestroyed"));
	const FGameplayTag RespawnCompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Respawn.Completed"));
	if (Event.Target && Event.Target == VehicleDestructionEvidenceTargetActor.Get() && (Event.EventTag == VehicleDestroyedTag || Event.EventTag == CoreVehicleDestroyedTag))
	{
		++VehicleDestructionEvidenceGameplayEventCount;
	}
	else if (Event.EventTag == RespawnCompletedTag)
	{
		++VehicleDestructionEvidenceRespawnEventCount;
	}
}

void AFWPlayerController::StartAIEvidenceSequence()
{
	if (!GetWorld() || !IsLocalPlayerController())
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("FWPIEAIPresentationEvidence")))
	{
		return;
	}

	AIEvidenceStep = 0;
	bAIEvidenceActive = true;
	AIEvidenceStartTime = GetWorld()->GetTimeSeconds();
	AIEvidenceStateChangedCount = 0;
	AIEvidenceWeaponFiredCount = 0;
	AIEvidenceDamageAppliedCount = 0;
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEAIPresentationEvidenceFolder="), AIEvidenceFolder);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEAIPresentationEvidencePrefix="), AIEvidencePrefix);

	if (const AFWCharacterBase* PlayerCharacter = Cast<AFWCharacterBase>(GetPawn()))
	{
		if (const UFWHealthComponent* PlayerHealth = PlayerCharacter->GetHealthComponent())
		{
			AIEvidenceInitialPlayerHealth = PlayerHealth->CurrentHealth;
		}
	}

	if (!AIEvidenceFolder.IsEmpty())
	{
		const FString EvidencePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(AIEvidenceFolder), FString::Printf(TEXT("%s_state-evidence.jsonl"), *AIEvidencePrefix));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidencePath), true);
		const FString Header = FString::Printf(TEXT("{\"event\":\"PIE AI presentation evidence start\",\"prefix\":\"%s\",\"worldType\":%d}\n"), *FWJsonEscape(AIEvidencePrefix), static_cast<int32>(GetWorld()->WorldType));
		FFileHelper::SaveStringToFile(Header, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFWEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UFWEventSubsystem>())
		{
			EventSubsystem->OnGameplayEvent.AddUniqueDynamic(this, &AFWPlayerController::HandleAIEvidenceGameplayEvent);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("FW AI presentation evidence sequence started. WorldType=%d"), static_cast<int32>(GetWorld()->WorldType));
	AIEvidenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
	{
		TickAIEvidenceSequence();
		return bAIEvidenceActive;
	}));
}

void AFWPlayerController::AdvanceAIEvidenceSequence()
{
	if (!GetWorld())
	{
		return;
	}

	++AIEvidenceStep;
	if (AIEvidenceStep == 1)
	{
		AIEvidenceTargetActor = FindNearestAICharacter();
		if (AIEvidenceTargetActor && !AIEvidenceTargetActor->GetController())
		{
			AIEvidenceTargetActor->SpawnDefaultController();
		}
		PositionAIEvidenceTarget(1500.0f);
		LastCombatDebugText = TEXT("AI evidence positioned target for Alert pursuit");
		ForceRefreshHUD();
		WriteAIEvidenceState(TEXT("00-alert-setup"), TEXT("alert-setup"));
	}
	else if (AIEvidenceStep == 2)
	{
		LastCombatDebugText = TEXT("AI evidence observed Alert pursuit");
		ForceRefreshHUD();
		WriteAIEvidenceState(TEXT("01-alert-pursuit"), TEXT("alert-pursuit"));
	}
	else if (AIEvidenceStep == 3)
	{
		PositionAIEvidenceTarget(450.0f);
		LastCombatDebugText = TEXT("AI evidence positioned target for AttackOnFoot");
		ForceRefreshHUD();
		WriteAIEvidenceState(TEXT("02-attack-setup"), TEXT("attack-setup"));
	}
	else if (AIEvidenceStep == 4)
	{
		LastCombatDebugText = TEXT("AI evidence observed AttackOnFoot presentation");
		ForceRefreshHUD();
		WriteAIEvidenceState(TEXT("03-attack-observed"), TEXT("attack-observed"));
		bAIEvidenceActive = false;
		FTSTicker::GetCoreTicker().RemoveTicker(AIEvidenceTickerHandle);
	}
}

void AFWPlayerController::TickAIEvidenceSequence()
{
	if (!bAIEvidenceActive || !GetWorld())
	{
		return;
	}

	const double ElapsedSeconds = GetWorld()->GetTimeSeconds() - AIEvidenceStartTime;
	constexpr double StepTimes[] = { 2.0, 5.0, 7.0, 9.0 };
	if (AIEvidenceStep < UE_ARRAY_COUNT(StepTimes) && ElapsedSeconds >= StepTimes[AIEvidenceStep])
	{
		AdvanceAIEvidenceSequence();
	}
}

void AFWPlayerController::WriteAIEvidenceState(const FString& StepSuffix, const FString& ActionLabel) const
{
	if (AIEvidenceFolder.IsEmpty())
	{
		return;
	}

	const AFWAICharacter* TargetAI = AIEvidenceTargetActor.Get();
	const UFWHealthComponent* AIHealth = TargetAI ? TargetAI->GetHealthComponent() : nullptr;
	const UFWStateMachineComponent* AIStateMachine = TargetAI ? TargetAI->GetStateMachineComponent() : nullptr;
	const AFWWeaponBase* AIWeapon = TargetAI ? TargetAI->GetCurrentWeapon() : nullptr;
	const AFWCharacterBase* PlayerCharacter = Cast<AFWCharacterBase>(GetPawn());
	const UFWHealthComponent* PlayerHealth = PlayerCharacter ? PlayerCharacter->GetHealthComponent() : nullptr;
	const FFWHUDStateSnapshot Snapshot = BuildHUDStateSnapshot();
	const float DistanceToPlayer = TargetAI && PlayerCharacter ? FVector::Dist2D(TargetAI->GetActorLocation(), PlayerCharacter->GetActorLocation()) : 0.0f;
	const FString AbsoluteFolder = FPaths::ConvertRelativePathToFull(AIEvidenceFolder);
	IFileManager::Get().MakeDirectory(*AbsoluteFolder, true);
	const FString EvidencePath = FPaths::Combine(AbsoluteFolder, FString::Printf(TEXT("%s_state-evidence.jsonl"), *AIEvidencePrefix));
	const FString Line = FString::Printf(
		TEXT("{\"event\":\"PIE AI presentation evidence step\",\"step\":\"%s\",\"stepIndex\":%d,\"action\":\"%s\",\"ai\":\"%s\",\"aiClass\":\"%s\",\"aiState\":\"%s\",\"aiHealth\":%.2f,\"aiMaxHealth\":%.2f,\"aiAmmo\":%d,\"distanceToPlayer\":%.2f,\"playerHealth\":%.2f,\"playerInitialHealth\":%.2f,\"hudAICount\":%d,\"hudAliveAICount\":%d,\"hudNearestAIHealth\":%.2f,\"hudNearestAIState\":\"%s\",\"stateChangedCount\":%d,\"weaponFiredCount\":%d,\"damageAppliedCount\":%d,\"lastCombat\":\"%s\"}\n"),
		*FWJsonEscape(StepSuffix),
		AIEvidenceStep,
		*FWJsonEscape(ActionLabel),
		*FWJsonEscape(GetNameSafe(TargetAI)),
		TargetAI ? *FWJsonEscape(TargetAI->GetClass()->GetName()) : TEXT(""),
		AIStateMachine ? *FWJsonEscape(AIStateMachine->CurrentState.ToString()) : TEXT(""),
		AIHealth ? AIHealth->CurrentHealth : 0.0f,
		AIHealth ? AIHealth->MaxHealth : 0.0f,
		AIWeapon ? AIWeapon->GetCurrentAmmo() : 0,
		DistanceToPlayer,
		PlayerHealth ? PlayerHealth->CurrentHealth : 0.0f,
		AIEvidenceInitialPlayerHealth,
		Snapshot.AICount,
		Snapshot.AliveAICount,
		Snapshot.NearestAIHealth,
		*FWJsonEscape(Snapshot.NearestAIStateTag.ToString()),
		AIEvidenceStateChangedCount,
		AIEvidenceWeaponFiredCount,
		AIEvidenceDamageAppliedCount,
		*FWJsonEscape(LastCombatDebugText));

	FFileHelper::SaveStringToFile(Line, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogTemp, Display, TEXT("FW AI presentation wrote state evidence: %s step=%s"), *EvidencePath, *StepSuffix);
}

void AFWPlayerController::PositionAIEvidenceTarget(float DistanceFromPlayer)
{
	AFWAICharacter* TargetAI = AIEvidenceTargetActor.Get();
	const APawn* PlayerPawn = GetPawn();
	if (!TargetAI || !PlayerPawn)
	{
		return;
	}

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const FVector TargetLocation = PlayerLocation + FVector(DistanceFromPlayer, 0.0f, 0.0f);
	TargetAI->SetActorLocationAndRotation(TargetLocation, (PlayerLocation - TargetLocation).Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AFWEnemyAIController* AIController = Cast<AFWEnemyAIController>(TargetAI->GetController()))
	{
		AIController->StopMovement();
	}
}

void AFWPlayerController::HandleAIEvidenceGameplayEvent(const FFWGameplayEvent& Event)
{
	const FGameplayTag AIStateChangedTag = FGameplayTag::RequestGameplayTag(TEXT("Event.AI.StateChanged"));
	const FGameplayTag WeaponFiredTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Weapon.Fired"));
	const FGameplayTag DamageAppliedTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Damage.Applied"));
	if (Event.EventTag == AIStateChangedTag && Event.Target == AIEvidenceTargetActor.Get())
	{
		++AIEvidenceStateChangedCount;
	}
	else if (Event.EventTag == WeaponFiredTag && Event.InstigatorActor == AIEvidenceTargetActor.Get())
	{
		++AIEvidenceWeaponFiredCount;
	}
	else if (Event.EventTag == DamageAppliedTag && Event.InstigatorActor == AIEvidenceTargetActor.Get())
	{
		++AIEvidenceDamageAppliedCount;
	}
}

void AFWPlayerController::StartDebugEvidenceSequence()
{
	if (!GetWorld() || !IsLocalPlayerController())
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("FWPIEDebugPresentationEvidence")))
	{
		return;
	}

	DebugEvidenceStep = 0;
	bDebugEvidenceActive = true;
	DebugEvidenceStartTime = GetWorld()->GetTimeSeconds();
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEDebugPresentationEvidenceFolder="), DebugEvidenceFolder);
	FParse::Value(FCommandLine::Get(), TEXT("FWPIEDebugPresentationEvidencePrefix="), DebugEvidencePrefix);

	if (!DebugEvidenceFolder.IsEmpty())
	{
		const FString EvidencePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(DebugEvidenceFolder), FString::Printf(TEXT("%s_state-evidence.jsonl"), *DebugEvidencePrefix));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidencePath), true);
		const FString Header = FString::Printf(TEXT("{\"event\":\"PIE debug presentation evidence start\",\"prefix\":\"%s\",\"worldType\":%d}\n"), *FWJsonEscape(DebugEvidencePrefix), static_cast<int32>(GetWorld()->WorldType));
		FFileHelper::SaveStringToFile(Header, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	UE_LOG(LogTemp, Display, TEXT("FW debug presentation evidence sequence started. WorldType=%d"), static_cast<int32>(GetWorld()->WorldType));
	DebugEvidenceTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
	{
		TickDebugEvidenceSequence();
		return bDebugEvidenceActive;
	}));
}

void AFWPlayerController::AdvanceDebugEvidenceSequence()
{
	if (!GetWorld())
	{
		return;
	}

	UFWDebugSubsystem* DebugSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFWDebugSubsystem>() : nullptr;
	++DebugEvidenceStep;
	if (DebugEvidenceStep == 1)
	{
		DebugEvidenceActor = FindDebugEvidenceActor();
		if (AFWAICharacter* TargetAI = Cast<AFWAICharacter>(DebugEvidenceActor.Get()))
		{
			if (!TargetAI->GetController())
			{
				TargetAI->SpawnDefaultController();
			}
			if (const APawn* PlayerPawn = GetPawn())
			{
				const FVector PlayerLocation = PlayerPawn->GetActorLocation();
				const FVector TargetLocation = PlayerLocation + FVector(550.0f, 0.0f, 0.0f);
				TargetAI->SetActorLocationAndRotation(TargetLocation, (PlayerLocation - TargetLocation).Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		if (DebugSubsystem)
		{
			DebugSubsystem->SetDebugEnabled(true);
			DebugSubsystem->ClearRecentEvents();
			DebugSubsystem->PrintActorDebugState(DebugEvidenceActor.Get(), 3.0f);
		}
		LastCombatDebugText = TEXT("Debug evidence enabled actor state output");
		ForceRefreshHUD();
		WriteDebugEvidenceState(TEXT("00-debug-enabled"), TEXT("debug-enabled"));
	}
	else if (DebugEvidenceStep == 2)
	{
		DebugFireNearestAI();
		if (DebugSubsystem)
		{
			DebugSubsystem->PrintActorDebugState(DebugEvidenceActor.Get(), 3.0f);
		}
		LastCombatDebugText = FString::Printf(TEXT("Debug evidence recorded recent events: %s"), *LastCombatDebugText);
		ForceRefreshHUD();
		WriteDebugEvidenceState(TEXT("01-events-recorded"), TEXT("events-recorded"));
	}
	else if (DebugEvidenceStep == 3)
	{
		if (DebugSubsystem)
		{
			DebugSubsystem->PrintActorDebugState(DebugEvidenceActor.Get(), 3.0f);
		}
		LastCombatDebugText = TEXT("Debug evidence verified HUD/debug buffer output");
		ForceRefreshHUD();
		WriteDebugEvidenceState(TEXT("02-buffer-visible"), TEXT("buffer-visible"));
		bDebugEvidenceActive = false;
		FTSTicker::GetCoreTicker().RemoveTicker(DebugEvidenceTickerHandle);
	}
}

void AFWPlayerController::TickDebugEvidenceSequence()
{
	if (!bDebugEvidenceActive || !GetWorld())
	{
		return;
	}

	const double ElapsedSeconds = GetWorld()->GetTimeSeconds() - DebugEvidenceStartTime;
	constexpr double StepTimes[] = { 2.0, 4.0, 6.0 };
	if (DebugEvidenceStep < UE_ARRAY_COUNT(StepTimes) && ElapsedSeconds >= StepTimes[DebugEvidenceStep])
	{
		AdvanceDebugEvidenceSequence();
	}
}

void AFWPlayerController::WriteDebugEvidenceState(const FString& StepSuffix, const FString& ActionLabel) const
{
	if (DebugEvidenceFolder.IsEmpty())
	{
		return;
	}

	const UFWDebugSubsystem* DebugSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFWDebugSubsystem>() : nullptr;
	const AActor* TargetActor = DebugEvidenceActor.Get();
	const UFWHealthComponent* Health = TargetActor ? TargetActor->FindComponentByClass<UFWHealthComponent>() : nullptr;
	const UFWStateMachineComponent* StateMachine = TargetActor ? TargetActor->FindComponentByClass<UFWStateMachineComponent>() : nullptr;
	const UFWHitZoneComponent* HitZone = TargetActor ? TargetActor->FindComponentByClass<UFWHitZoneComponent>() : nullptr;
	const FFWHUDStateSnapshot Snapshot = BuildHUDStateSnapshot();

	FString RecentEventTags;
	FString LatestEventTag;
	FString LatestEventTarget;
	FString LatestEventInstigator;
	float LatestEventMagnitude = 0.0f;
	int32 RecentEventCount = 0;
	if (DebugSubsystem)
	{
		const TArray<FFWDebugEventRecord>& RecentEvents = DebugSubsystem->GetRecentEvents();
		RecentEventCount = RecentEvents.Num();
		TArray<FString> EventTagStrings;
		EventTagStrings.Reserve(RecentEvents.Num());
		for (const FFWDebugEventRecord& Record : RecentEvents)
		{
			EventTagStrings.Add(Record.EventTag.ToString());
		}
		RecentEventTags = FString::Join(EventTagStrings, TEXT("|"));
		if (!RecentEvents.IsEmpty())
		{
			const FFWDebugEventRecord& LatestEvent = RecentEvents.Last();
			LatestEventTag = LatestEvent.EventTag.ToString();
			LatestEventTarget = GetNameSafe(LatestEvent.Target.Get());
			LatestEventInstigator = GetNameSafe(LatestEvent.InstigatorActor.Get());
			LatestEventMagnitude = LatestEvent.Magnitude;
		}
	}

	const FString AbsoluteFolder = FPaths::ConvertRelativePathToFull(DebugEvidenceFolder);
	IFileManager::Get().MakeDirectory(*AbsoluteFolder, true);
	const FString EvidencePath = FPaths::Combine(AbsoluteFolder, FString::Printf(TEXT("%s_state-evidence.jsonl"), *DebugEvidencePrefix));
	const FString Line = FString::Printf(
		TEXT("{\"event\":\"PIE debug presentation evidence step\",\"step\":\"%s\",\"stepIndex\":%d,\"action\":\"%s\",\"debugEnabled\":%s,\"actor\":\"%s\",\"actorClass\":\"%s\",\"actorState\":\"%s\",\"actorHealth\":%.2f,\"actorMaxHealth\":%.2f,\"hitZone\":\"%s\",\"hitZoneMultiplier\":%.2f,\"recentEventCount\":%d,\"recentEventTags\":\"%s\",\"latestEventTag\":\"%s\",\"latestEventMagnitude\":%.2f,\"latestEventTarget\":\"%s\",\"latestEventInstigator\":\"%s\",\"hudPlayerState\":\"%s\",\"hudPlayerHealth\":%.2f,\"hudLives\":%d,\"hudAICount\":%d,\"hudAliveAICount\":%d,\"hudNearestAIHealth\":%.2f,\"hudNearestAIState\":\"%s\",\"lastCombat\":\"%s\"}\n"),
		*FWJsonEscape(StepSuffix),
		DebugEvidenceStep,
		*FWJsonEscape(ActionLabel),
		DebugSubsystem && DebugSubsystem->IsDebugEnabled() ? TEXT("true") : TEXT("false"),
		*FWJsonEscape(GetNameSafe(TargetActor)),
		TargetActor ? *FWJsonEscape(TargetActor->GetClass()->GetName()) : TEXT(""),
		StateMachine ? *FWJsonEscape(StateMachine->CurrentState.ToString()) : TEXT("NoState"),
		Health ? Health->CurrentHealth : 0.0f,
		Health ? Health->MaxHealth : 0.0f,
		HitZone ? *FWJsonEscape(HitZone->HitZoneName.ToString()) : TEXT("NoHitZone"),
		HitZone ? HitZone->DamageMultiplier : 0.0f,
		RecentEventCount,
		*FWJsonEscape(RecentEventTags),
		*FWJsonEscape(LatestEventTag),
		LatestEventMagnitude,
		*FWJsonEscape(LatestEventTarget),
		*FWJsonEscape(LatestEventInstigator),
		*FWJsonEscape(Snapshot.PlayerStateTag.ToString()),
		Snapshot.PlayerHealth,
		Snapshot.Lives,
		Snapshot.AICount,
		Snapshot.AliveAICount,
		Snapshot.NearestAIHealth,
		*FWJsonEscape(Snapshot.NearestAIStateTag.ToString()),
		*FWJsonEscape(Snapshot.LastCombatDebugText));

	FFileHelper::SaveStringToFile(Line, *EvidencePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogTemp, Display, TEXT("FW debug presentation wrote state evidence: %s step=%s"), *EvidencePath, *StepSuffix);
}

AActor* AFWPlayerController::FindDebugEvidenceActor() const
{
	if (AFWAICharacter* NearestAI = FindNearestAICharacter())
	{
		return NearestAI;
	}

	if (AActor* PawnActor = GetPawn())
	{
		return PawnActor;
	}

	return FindNearestVehicle();
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
