#include "FWMVPRuntimeVerifyCommandlet.h"

#include "../AI/FWEnemyAIController.h"
#include "../Characters/FWAICharacter.h"
#include "../Characters/FWPlayerCharacter.h"
#include "../Combat/FWHealthComponent.h"
#include "../Core/FWCompetitiveGameMode.h"
#include "../Core/FWPlayerController.h"
#include "../Core/FWPlayerState.h"
#include "../Debug/FWDebugSubsystem.h"
#include "../Rules/FWMatchRuleSet.h"
#include "../State/FWStateMachineComponent.h"
#include "../UI/FWHUDTypes.h"
#include "../Vehicles/FWMVPVehicleClasses.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Weapons/FWMVPWeaponClasses.h"
#include "../Weapons/FWWeaponBase.h"
#include "../Weapons/FWWeaponData.h"
#include "../World/FWCombatGarageSpawnPoint.h"
#include "../World/FWCombatGarageTestDirector.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "UObject/UnrealType.h"

namespace
{
	void BeginActorForRuntimeVerify(AActor* Actor, bool bDispatchBeginPlay = true)
	{
		if (!Actor)
		{
			return;
		}

		Actor->RegisterAllComponents();
		Actor->PostInitializeComponents();
		if (bDispatchBeginPlay && !Actor->HasActorBegunPlay())
		{
			Actor->DispatchBeginPlay();
		}
	}

	bool SetFloatPropertyForRuntimeVerify(UObject* Object, const FName PropertyName, float Value)
	{
		if (!Object)
		{
			return false;
		}

		if (FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName))
		{
			FloatProperty->SetPropertyValue_InContainer(Object, Value);
			return true;
		}

		return false;
	}
}

UFWMVPRuntimeVerifyCommandlet::UFWMVPRuntimeVerifyCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UFWMVPRuntimeVerifyCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("FW MVP runtime verification started."));
	IssueCount = 0;

	if (!VerifyRuntimeWorld())
	{
		++IssueCount;
	}

	if (IssueCount > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FW MVP runtime verification failed with %d issue(s)."), IssueCount);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("FW MVP runtime verification passed."));
	return 0;
}

void UFWMVPRuntimeVerifyCommandlet::Check(bool bCondition, const FString& Message)
{
	if (bCondition)
	{
		UE_LOG(LogTemp, Display, TEXT("[OK] %s"), *Message);
		return;
	}

	++IssueCount;
	UE_LOG(LogTemp, Error, TEXT("[FAIL] %s"), *Message);
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyRuntimeWorld()
{
	if (!GEngine)
	{
		Check(false, TEXT("GEngine is available"));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("FW_MVP_RuntimeVerify"));
	Check(World != nullptr, TEXT("Transient runtime world can be created"));
	if (!World)
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	VerifyPlayerInputSemantics(World);
	VerifyAICombatRuntime(World);
	VerifyWeaponDamage(World);
	VerifyVehicleEnterExitAndDestruction(World);
	VerifyVehicleDrivingRuntime(World);
	VerifyCombatGarageDirectorRuntime(World);
	VerifyCombatGarageMapRuntime(World);
	VerifyHUDSnapshotRuntime(World);
	VerifyRulesRespawnRuntime(World);
	VerifyDebugRuntime();

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyPlayerInputSemantics(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFWPlayerController* PlayerController = World->SpawnActor<AFWPlayerController>(AFWPlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerState* PlayerState = World->SpawnActor<AFWPlayerState>(AFWPlayerState::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWCarVehicle* InteractableCar = World->SpawnActor<AFWCarVehicle>(AFWCarVehicle::StaticClass(), FVector(250.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);

	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(PlayerState);
	BeginActorForRuntimeVerify(Player);
	BeginActorForRuntimeVerify(InteractableCar);

	Check(PlayerController != nullptr, TEXT("Runtime input player controller spawns"));
	Check(PlayerState != nullptr, TEXT("Runtime input player state spawns"));
	Check(Player != nullptr, TEXT("Runtime input player character spawns"));
	Check(InteractableCar != nullptr, TEXT("Runtime input interactable car spawns"));
	if (!PlayerController || !PlayerState || !Player || !InteractableCar)
	{
		return IssueCount == StartingIssues;
	}

	PlayerController->SetPlayerState(PlayerState);
	PlayerController->Possess(Player);
	PlayerController->SetControlRotation(FRotator::ZeroRotator);

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	InteractableCar->SetActorLocation(ViewLocation + (ViewRotation.Vector() * 250.0f));

	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	Check(Movement != nullptr, TEXT("Runtime input player has character movement component"));
	if (!Movement)
	{
		return IssueCount == StartingIssues;
	}

	Player->ConsumeMovementInputVector();
	Player->Move(FInputActionValue(FVector2D(0.0f, 1.0f)));
	Check(true, TEXT("Runtime input invokes Move handler"));
	const FVector MoveInput = Player->ConsumeMovementInputVector();
	Check(MoveInput.X > 0.5f, FString::Printf(TEXT("Runtime Move input queues forward movement (input: %s)"), *MoveInput.ToCompactString()));

	const float WalkSpeed = Movement->MaxWalkSpeed;
	Player->StartRun();
	Check(true, TEXT("Runtime input invokes StartRun handler"));
	const float RunSpeed = Movement->MaxWalkSpeed;
	Check(RunSpeed > WalkSpeed, FString::Printf(TEXT("Runtime StartRun increases max walk speed (walk: %.1f run: %.1f)"), WalkSpeed, RunSpeed));
	Player->StopRun();
	Check(true, TEXT("Runtime input invokes StopRun handler"));
	Check(FMath::IsNearlyEqual(Movement->MaxWalkSpeed, WalkSpeed), FString::Printf(TEXT("Runtime StopRun restores walk speed (actual: %.1f expected: %.1f)"), Movement->MaxWalkSpeed, WalkSpeed));

	Player->ToggleCrawl();
	Check(true, TEXT("Runtime input invokes ToggleCrawl handler"));
	const float CrawlSpeed = Movement->MaxWalkSpeed;
	Check(CrawlSpeed > 0.0f && CrawlSpeed < WalkSpeed, FString::Printf(TEXT("Runtime ToggleCrawl lowers movement speed (crawl: %.1f walk: %.1f)"), CrawlSpeed, WalkSpeed));
	Player->ToggleCrawl();
	Check(true, TEXT("Runtime input invokes ToggleCrawl handler again"));
	Check(FMath::IsNearlyEqual(Movement->MaxWalkSpeed, WalkSpeed), FString::Printf(TEXT("Runtime second ToggleCrawl restores walk speed (actual: %.1f expected: %.1f)"), Movement->MaxWalkSpeed, WalkSpeed));

	const bool bInitialCameraMode = Player->IsFirstPersonCamera();
	Player->ToggleCamera();
	Check(true, TEXT("Runtime input invokes ToggleCamera handler"));
	Check(Player->IsFirstPersonCamera() != bInitialCameraMode, TEXT("Runtime ToggleCamera changes camera mode flag"));
	Player->ToggleCamera();
	Check(true, TEXT("Runtime input invokes ToggleCamera handler again"));
	Check(Player->IsFirstPersonCamera() == bInitialCameraMode, TEXT("Runtime second ToggleCamera restores camera mode flag"));

	Check(Player->GetCurrentWeapon() != nullptr, TEXT("Runtime input player begins with a default weapon"));
	Check(Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->GetWeaponData() != nullptr, TEXT("Runtime input default weapon loads weapon data"));

	Player->EquipWeaponSlot1();
	Check(true, TEXT("Runtime input invokes weapon slot 1 handler"));
	Check(Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->GetWeaponData() && Player->GetCurrentWeapon()->GetWeaponData()->ContentId == TEXT("Weapon.Pistol.MVP"),
		TEXT("Runtime weapon slot 1 equips pistol"));
	Player->EquipWeaponSlot2();
	Check(true, TEXT("Runtime input invokes weapon slot 2 handler"));
	Check(Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->GetWeaponData() && Player->GetCurrentWeapon()->GetWeaponData()->ContentId == TEXT("Weapon.Rifle.MVP"),
		TEXT("Runtime weapon slot 2 equips rifle"));
	Player->EquipWeaponSlot3();
	Check(true, TEXT("Runtime input invokes weapon slot 3 handler"));
	Check(Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->GetWeaponData() && Player->GetCurrentWeapon()->GetWeaponData()->ContentId == TEXT("Weapon.Sniper.MVP"),
		TEXT("Runtime weapon slot 3 equips sniper"));

	AFWWeaponBase* CurrentWeapon = Player->GetCurrentWeapon();
	BeginActorForRuntimeVerify(CurrentWeapon);
	const int32 AmmoBeforeFire = CurrentWeapon ? CurrentWeapon->GetCurrentAmmo() : 0;
	Player->StartFire();
	Check(true, TEXT("Runtime input invokes StartFire handler"));
	Check(CurrentWeapon && CurrentWeapon->GetCurrentAmmo() == AmmoBeforeFire - 1,
		FString::Printf(TEXT("Runtime StartFire consumes one round from current weapon (before: %d after: %d)"), AmmoBeforeFire, CurrentWeapon ? CurrentWeapon->GetCurrentAmmo() : -1));
	Player->StopFire();
	Check(true, TEXT("Runtime input invokes StopFire handler"));

	Player->UpdateInteractionTarget();
	Check(Player->GetCurrentInteractableActor() == InteractableCar, TEXT("Runtime input trace detects visible vehicle before interaction"));
	Player->Interact();
	Check(true, TEXT("Runtime input invokes Interact handler"));
	Check(PlayerController->GetPawn() == InteractableCar, TEXT("Runtime Interact enters visible vehicle"));
	Check(InteractableCar->GetPreviousPawn() == Player, TEXT("Runtime Interact vehicle remembers previous player pawn"));
	Check(InteractableCar->ExitVehicle(), TEXT("Runtime input interaction test exits vehicle"));
	Check(PlayerController->GetPawn() == Player, TEXT("Runtime input interaction test restores player pawn after vehicle exit"));

	PlayerController->Possess(Player);
	if (UFWHealthComponent* HealthComponent = Player->GetHealthComponent())
	{
		HealthComponent->ApplyDamage(HealthComponent->MaxHealth + 100.0f);
		Check(HealthComponent->IsDead(), TEXT("Runtime input player health reaches dead state"));
	}

	Player->ConsumeMovementInputVector();
	Movement->MaxWalkSpeed = WalkSpeed;
	Player->StartRun();
	Check(true, TEXT("Runtime input invokes StartRun after death"));
	Check(FMath::IsNearlyEqual(Movement->MaxWalkSpeed, WalkSpeed), TEXT("Runtime dead player cannot start running"));
	Player->Move(FInputActionValue(FVector2D(0.0f, 1.0f)));
	Check(true, TEXT("Runtime input invokes Move after death"));
	Check(Player->ConsumeMovementInputVector().IsNearlyZero(), TEXT("Runtime dead player does not queue movement input"));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyAICombatRuntime(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFWPlayerController* PlayerController = World->SpawnActor<AFWPlayerController>(AFWPlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWAICharacter* AICharacter = World->SpawnActor<AFWAICharacter>(AFWAICharacter::StaticClass(), FVector(1500.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	AFWEnemyAIController* AIController = World->SpawnActor<AFWEnemyAIController>(AFWEnemyAIController::StaticClass(), FVector(1500.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);

	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(Player);
	BeginActorForRuntimeVerify(AICharacter);
	BeginActorForRuntimeVerify(AIController, false);

	Check(PlayerController != nullptr, TEXT("Runtime AI test player controller spawns"));
	Check(Player != nullptr, TEXT("Runtime AI test player spawns"));
	Check(AICharacter != nullptr, TEXT("Runtime AI character spawns"));
	Check(AIController != nullptr, TEXT("Runtime AI controller spawns"));
	if (!PlayerController || !Player || !AICharacter || !AIController)
	{
		return IssueCount == StartingIssues;
	}

	PlayerController->Possess(Player);
	AIController->Possess(AICharacter);
	Check(SetFloatPropertyForRuntimeVerify(AIController, TEXT("DetectionRadius"), 3000.0f), TEXT("Runtime AI test configures detection radius"));
	Check(SetFloatPropertyForRuntimeVerify(AIController, TEXT("AttackRange"), 700.0f), TEXT("Runtime AI test configures attack range"));
	Check(SetFloatPropertyForRuntimeVerify(AIController, TEXT("AttackInterval"), 0.01f), TEXT("Runtime AI test configures attack interval"));
	Check(SetFloatPropertyForRuntimeVerify(AIController, TEXT("Accuracy"), 1.0f), TEXT("Runtime AI test configures deterministic accuracy"));

	AFWWeaponBase* AIWeapon = AICharacter->GetCurrentWeapon();
	BeginActorForRuntimeVerify(AIWeapon);
	Check(AIWeapon != nullptr, TEXT("Runtime AI spawns default weapon"));
	Check(AIWeapon && AIWeapon->GetWeaponData() != nullptr, TEXT("Runtime AI default weapon loads weapon data"));
	Check(AIWeapon && AIWeapon->GetCurrentAmmo() > 0, FString::Printf(TEXT("Runtime AI default weapon has ammo (actual: %d)"), AIWeapon ? AIWeapon->GetCurrentAmmo() : -1));

	const FGameplayTag AlertState = FGameplayTag::RequestGameplayTag(TEXT("State.AI.Alert"));
	const FGameplayTag AttackState = FGameplayTag::RequestGameplayTag(TEXT("State.AI.AttackOnFoot"));
	const FGameplayTag DeadState = FGameplayTag::RequestGameplayTag(TEXT("State.AI.Dead"));
	UFWStateMachineComponent* AIStateMachine = AICharacter->GetStateMachineComponent();
	UFWHealthComponent* PlayerHealth = Player->GetHealthComponent();
	UFWHealthComponent* AIHealth = AICharacter->GetHealthComponent();
	Check(AIStateMachine != nullptr, TEXT("Runtime AI character has state machine"));
	Check(PlayerHealth != nullptr, TEXT("Runtime AI target player has health component"));
	Check(AIHealth != nullptr, TEXT("Runtime AI character has health component"));
	if (!AIStateMachine || !PlayerHealth || !AIHealth || !AIWeapon)
	{
		return IssueCount == StartingIssues;
	}

	AICharacter->SetActorLocation(FVector(1500.0f, 0.0f, 0.0f));
	Player->SetActorLocation(FVector::ZeroVector);
	AIController->Tick(0.016f);
	Check(AIStateMachine->CurrentState == AlertState, FString::Printf(TEXT("Runtime AI enters Alert while target is detected but outside attack range (state: %s)"), *AIStateMachine->CurrentState.ToString()));

	const float PlayerHealthBeforeAttack = PlayerHealth->CurrentHealth;
	AICharacter->SetActorLocation(FVector(450.0f, 0.0f, 0.0f));
	AIController->Tick(0.5f);
	Check(AIStateMachine->CurrentState == AttackState, FString::Printf(TEXT("Runtime AI enters AttackOnFoot inside attack range (state: %s)"), *AIStateMachine->CurrentState.ToString()));
	Check(PlayerHealth->CurrentHealth < PlayerHealthBeforeAttack,
		FString::Printf(TEXT("Runtime AI attack damages player health (before: %.1f after: %.1f)"), PlayerHealthBeforeAttack, PlayerHealth->CurrentHealth));
	Check(AIWeapon->GetCurrentAmmo() >= 0 && AIWeapon->GetCurrentAmmo() < AIWeapon->GetWeaponData()->MagazineSize,
		FString::Printf(TEXT("Runtime AI attack consumes weapon ammo (current: %d magazine: %d)"), AIWeapon->GetCurrentAmmo(), AIWeapon->GetWeaponData()->MagazineSize));

	AIHealth->ApplyDamage(AIHealth->MaxHealth + 100.0f);
	Check(AIHealth->IsDead(), TEXT("Runtime AI health reaches dead state"));
	AIController->Tick(0.016f);
	Check(AIStateMachine->CurrentState == DeadState, FString::Printf(TEXT("Runtime AI enters Dead state after lethal damage (state: %s)"), *AIStateMachine->CurrentState.ToString()));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyWeaponDamage(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWAICharacter* Target = World->SpawnActor<AFWAICharacter>(AFWAICharacter::StaticClass(), FVector(600.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	BeginActorForRuntimeVerify(Player);
	BeginActorForRuntimeVerify(Target);

	Check(Player != nullptr, TEXT("Runtime player character spawns"));
	Check(Target != nullptr, TEXT("Runtime AI target spawns"));
	if (!Player || !Target)
	{
		return IssueCount == StartingIssues;
	}

	AFWWeaponBase* Weapon = World->SpawnActor<AFWWeaponBase>(AFWPistolWeapon::StaticClass(), Player->GetActorTransform(), SpawnParameters);
	BeginActorForRuntimeVerify(Weapon);
	Check(Weapon != nullptr, TEXT("Runtime pistol weapon spawns"));
	if (!Weapon)
	{
		return IssueCount == StartingIssues;
	}

	Player->EquipWeapon(Weapon);
	const UFWHealthComponent* TargetHealthBefore = Target->GetHealthComponent();
	const float InitialHealth = TargetHealthBefore ? TargetHealthBefore->CurrentHealth : 0.0f;
	const bool bFired = Weapon->FireAtActor(Target);
	const UFWHealthComponent* TargetHealthAfter = Target->GetHealthComponent();

	Check(Weapon->GetWeaponData() != nullptr, TEXT("Runtime pistol loads weapon data"));
	Check(Weapon->GetCurrentAmmo() >= 0, TEXT("Runtime pistol initializes ammo"));
	Check(bFired, TEXT("Runtime pistol fires at AI target"));
	Check(TargetHealthAfter && TargetHealthAfter->CurrentHealth < InitialHealth,
		FString::Printf(TEXT("Runtime pistol damage reduces AI health (before: %.1f after: %.1f)"),
			InitialHealth,
			TargetHealthAfter ? TargetHealthAfter->CurrentHealth : -1.0f));

	auto VerifyVehicleDamage = [this, World, &SpawnParameters, Player](TSubclassOf<AFWVehicleBase> VehicleClass, const FString& VehicleLabel, const FVector& VehicleLocation)
	{
		const int32 VehicleStartingIssues = IssueCount;

		AFWVehicleBase* Vehicle = World->SpawnActor<AFWVehicleBase>(VehicleClass, VehicleLocation, FRotator::ZeroRotator, SpawnParameters);
		AFWWeaponBase* VehicleTestWeapon = World->SpawnActor<AFWWeaponBase>(AFWPistolWeapon::StaticClass(), Player->GetActorTransform(), SpawnParameters);
		BeginActorForRuntimeVerify(Vehicle);
		BeginActorForRuntimeVerify(VehicleTestWeapon);

		Check(Vehicle != nullptr, FString::Printf(TEXT("Runtime weapon damage %s target spawns"), *VehicleLabel));
		Check(VehicleTestWeapon != nullptr, FString::Printf(TEXT("Runtime weapon damage %s test pistol spawns"), *VehicleLabel));
		if (!Vehicle || !VehicleTestWeapon)
		{
			return IssueCount == VehicleStartingIssues;
		}

		VehicleTestWeapon->Equip(Player);
		UFWHealthComponent* VehicleHealth = Vehicle->FindComponentByClass<UFWHealthComponent>();
		Check(VehicleHealth != nullptr, FString::Printf(TEXT("Runtime weapon damage %s target has health component"), *VehicleLabel));
		Check(VehicleTestWeapon->GetWeaponData() != nullptr, FString::Printf(TEXT("Runtime weapon damage %s pistol loads weapon data"), *VehicleLabel));
		if (!VehicleHealth || !VehicleTestWeapon->GetWeaponData())
		{
			return IssueCount == VehicleStartingIssues;
		}

		const float VehicleHealthBefore = VehicleHealth->CurrentHealth;
		const int32 AmmoBeforeVehicleShot = VehicleTestWeapon->GetCurrentAmmo();
		const bool bVehicleShotFired = VehicleTestWeapon->FireAtActor(Vehicle);

		Check(bVehicleShotFired, FString::Printf(TEXT("Runtime pistol fires at %s target"), *VehicleLabel));
		Check(VehicleTestWeapon->GetCurrentAmmo() == AmmoBeforeVehicleShot - 1,
			FString::Printf(TEXT("Runtime pistol consumes ammo when firing at %s (before: %d after: %d)"), *VehicleLabel, AmmoBeforeVehicleShot, VehicleTestWeapon->GetCurrentAmmo()));
		Check(VehicleHealth->CurrentHealth < VehicleHealthBefore,
			FString::Printf(TEXT("Runtime pistol damage reduces %s health (before: %.1f after: %.1f)"), *VehicleLabel, VehicleHealthBefore, VehicleHealth->CurrentHealth));

		return IssueCount == VehicleStartingIssues;
	};

	VerifyVehicleDamage(AFWCarVehicle::StaticClass(), TEXT("car"), FVector(900.0f, -300.0f, 0.0f));
	VerifyVehicleDamage(AFWMotorcycleVehicle::StaticClass(), TEXT("motorcycle"), FVector(900.0f, 300.0f, 0.0f));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyVehicleEnterExitAndDestruction(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerController* PlayerController = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWCarVehicle* Car = World->SpawnActor<AFWCarVehicle>(AFWCarVehicle::StaticClass(), FVector(250.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(Player);
	BeginActorForRuntimeVerify(Car);

	Check(PlayerController != nullptr, TEXT("Runtime player controller spawns"));
	Check(Player != nullptr, TEXT("Runtime vehicle test player spawns"));
	Check(Car != nullptr, TEXT("Runtime car spawns"));
	if (!PlayerController || !Player || !Car)
	{
		return IssueCount == StartingIssues;
	}

	PlayerController->Possess(Player);
	Check(PlayerController->GetPawn() == Player, TEXT("Runtime player controller possesses player"));
	Check(Car->GetMovementComponent() != nullptr, TEXT("Runtime car has movement component"));
	Check(Car->CanEnterVehicle(PlayerController), TEXT("Runtime car can be entered"));
	Check(Car->EnterVehicle(PlayerController), TEXT("Runtime car enter succeeds"));
	Check(PlayerController->GetPawn() == Car, TEXT("Runtime car possession succeeds"));
	Check(Car->GetPreviousPawn() == Player, TEXT("Runtime car remembers previous pawn"));
	Check(Car->ExitVehicle(), TEXT("Runtime car exit succeeds"));
	Check(PlayerController->GetPawn() == Player, TEXT("Runtime car restores previous pawn"));

	Car->SetOwnerController(PlayerController);
	Car->SetCoreVehicle(true);
	if (UFWHealthComponent* VehicleHealth = Car->FindComponentByClass<UFWHealthComponent>())
	{
		VehicleHealth->ApplyDamage(VehicleHealth->MaxHealth + 100.0f);
		Check(VehicleHealth->IsDead(), TEXT("Runtime car health component reaches dead state"));
	}
	Car->Tick(0.016f);

	Check(Car->IsDestroyed(), FString::Printf(TEXT("Runtime car enters destroyed state after lethal damage (destroyed: %s)"), Car->IsDestroyed() ? TEXT("true") : TEXT("false")));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyVehicleDrivingRuntime(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto VerifyVehicleClass = [this, World, &SpawnParameters](TSubclassOf<AFWVehicleBase> VehicleClass, const FString& VehicleLabel, const FVector& VehicleLocation)
	{
		const int32 VehicleStartingIssues = IssueCount;

		APlayerController* PlayerController = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), VehicleLocation + FVector(-500.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
		AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), VehicleLocation + FVector(-350.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
		AFWVehicleBase* Vehicle = World->SpawnActor<AFWVehicleBase>(VehicleClass, VehicleLocation, FRotator::ZeroRotator, SpawnParameters);

		BeginActorForRuntimeVerify(PlayerController, false);
		BeginActorForRuntimeVerify(Player);
		BeginActorForRuntimeVerify(Vehicle);

		Check(PlayerController != nullptr, FString::Printf(TEXT("Runtime %s driving controller spawns"), *VehicleLabel));
		Check(Player != nullptr, FString::Printf(TEXT("Runtime %s driving player spawns"), *VehicleLabel));
		Check(Vehicle != nullptr, FString::Printf(TEXT("Runtime %s driving vehicle spawns"), *VehicleLabel));
		if (!PlayerController || !Player || !Vehicle)
		{
			return IssueCount == VehicleStartingIssues;
		}

		PlayerController->Possess(Player);
		Check(PlayerController->GetPawn() == Player, FString::Printf(TEXT("Runtime %s driving controller starts on player"), *VehicleLabel));
		Check(Vehicle->GetMovementComponent() != nullptr, FString::Printf(TEXT("Runtime %s has movement component for driving"), *VehicleLabel));
		Check(Vehicle->EnterVehicle(PlayerController), FString::Printf(TEXT("Runtime %s driving enter succeeds"), *VehicleLabel));
		Check(PlayerController->GetPawn() == Vehicle, FString::Printf(TEXT("Runtime %s driving possession succeeds"), *VehicleLabel));

		const FVector ForwardStart = Vehicle->GetActorLocation();
		Vehicle->Move(FInputActionValue(FVector2D(0.0f, 1.0f)));
		Check(true, FString::Printf(TEXT("Runtime %s driving invokes Move throttle handler"), *VehicleLabel));
		for (int32 Step = 0; Step < 12; ++Step)
		{
			Vehicle->Tick(0.1f);
		}

		const FVector ForwardDelta = Vehicle->GetActorLocation() - ForwardStart;
		Check(FVector::DotProduct(ForwardDelta, FVector::ForwardVector) > 100.0f,
			FString::Printf(TEXT("Runtime %s throttle moves vehicle forward (delta: %s)"), *VehicleLabel, *ForwardDelta.ToCompactString()));

		const float YawBeforeSteer = Vehicle->GetActorRotation().Yaw;
		Vehicle->Move(FInputActionValue(FVector2D(1.0f, 1.0f)));
		Check(true, FString::Printf(TEXT("Runtime %s driving invokes Move steering handler"), *VehicleLabel));
		for (int32 Step = 0; Step < 8; ++Step)
		{
			Vehicle->Tick(0.1f);
		}

		const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(YawBeforeSteer, Vehicle->GetActorRotation().Yaw));
		Check(YawDelta > 1.0f,
			FString::Printf(TEXT("Runtime %s steering changes yaw (delta: %.2f)"), *VehicleLabel, YawDelta));

		Vehicle->Look(FInputActionValue(FVector2D(5.0f, 0.0f)));
		Check(PlayerController->GetPawn() == Vehicle, FString::Printf(TEXT("Runtime %s look input keeps vehicle possession stable"), *VehicleLabel));

		Vehicle->HandleExitInput();
		Check(true, FString::Printf(TEXT("Runtime %s driving invokes exit input handler"), *VehicleLabel));
		Check(PlayerController->GetPawn() == Player, FString::Printf(TEXT("Runtime %s exit input restores player pawn"), *VehicleLabel));
		Check(Vehicle->GetOccupantController() == nullptr, FString::Printf(TEXT("Runtime %s exit input clears occupant"), *VehicleLabel));

		return IssueCount == VehicleStartingIssues;
	};

	VerifyVehicleClass(AFWCarVehicle::StaticClass(), TEXT("car"), FVector(2000.0f, -500.0f, 0.0f));
	VerifyVehicleClass(AFWMotorcycleVehicle::StaticClass(), TEXT("motorcycle"), FVector(2000.0f, 500.0f, 0.0f));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyCombatGarageDirectorRuntime(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerController* PlayerController = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(Player);
	if (PlayerController && Player)
	{
		PlayerController->Possess(Player);
	}

	auto SpawnPoint = [World, &SpawnParameters](EFWCombatGarageSpawnType SpawnType, FName SpawnId, const FVector& Location)
	{
		AFWCombatGarageSpawnPoint* Point = World->SpawnActor<AFWCombatGarageSpawnPoint>(AFWCombatGarageSpawnPoint::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
		if (Point)
		{
			Point->SpawnType = SpawnType;
			Point->SpawnId = SpawnId;
		}
		return Point;
	};

	SpawnPoint(EFWCombatGarageSpawnType::Player, TEXT("Player_Runtime"), FVector::ZeroVector);
	SpawnPoint(EFWCombatGarageSpawnType::AI, TEXT("AI_Runtime_01"), FVector(700.0f, -200.0f, 0.0f));
	SpawnPoint(EFWCombatGarageSpawnType::AI, TEXT("AI_Runtime_02"), FVector(800.0f, 0.0f, 0.0f));
	SpawnPoint(EFWCombatGarageSpawnType::AI, TEXT("AI_Runtime_03"), FVector(700.0f, 200.0f, 0.0f));
	SpawnPoint(EFWCombatGarageSpawnType::Car, TEXT("Car_Runtime"), FVector(250.0f, -300.0f, 0.0f));
	SpawnPoint(EFWCombatGarageSpawnType::Motorcycle, TEXT("Motorcycle_Runtime"), FVector(250.0f, 300.0f, 0.0f));
	SpawnPoint(EFWCombatGarageSpawnType::Weapon, TEXT("Weapon_Runtime"), FVector(150.0f, 0.0f, 0.0f));

	AFWCombatGarageTestDirector* Director = World->SpawnActor<AFWCombatGarageTestDirector>(AFWCombatGarageTestDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	BeginActorForRuntimeVerify(Director, false);
	Check(Director != nullptr, TEXT("Runtime Combat Garage director spawns"));
	if (!Director)
	{
		return IssueCount == StartingIssues;
	}

	FString ValidationMessage;
	Check(Director->ValidateCombatGarageSetup(ValidationMessage), FString::Printf(TEXT("Runtime Combat Garage validates before setup: %s"), *ValidationMessage));

	auto CountActors = [World](UClass* ActorClass)
	{
		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->IsA(ActorClass))
			{
				++Count;
			}
		}
		return Count;
	};

	const int32 AICountBefore = CountActors(AFWAICharacter::StaticClass());
	const int32 CarCountBefore = CountActors(AFWCarVehicle::StaticClass());
	const int32 MotorcycleCountBefore = CountActors(AFWMotorcycleVehicle::StaticClass());
	const int32 WeaponCountBefore = CountActors(AFWWeaponBase::StaticClass());

	Director->SetupCombatGarage();

	const int32 AISpawnDelta = CountActors(AFWAICharacter::StaticClass()) - AICountBefore;
	const int32 CarSpawnDelta = CountActors(AFWCarVehicle::StaticClass()) - CarCountBefore;
	const int32 MotorcycleSpawnDelta = CountActors(AFWMotorcycleVehicle::StaticClass()) - MotorcycleCountBefore;
	const int32 WeaponSpawnDelta = CountActors(AFWWeaponBase::StaticClass()) - WeaponCountBefore;

	Check(AISpawnDelta >= 3, FString::Printf(TEXT("Runtime Combat Garage spawns at least three AI (delta: %d)"), AISpawnDelta));
	Check(CarSpawnDelta >= 1, FString::Printf(TEXT("Runtime Combat Garage spawns a car (delta: %d)"), CarSpawnDelta));
	Check(MotorcycleSpawnDelta >= 1, FString::Printf(TEXT("Runtime Combat Garage spawns a motorcycle (delta: %d)"), MotorcycleSpawnDelta));
	Check(WeaponSpawnDelta >= 1, FString::Printf(TEXT("Runtime Combat Garage spawns at least one test weapon (delta: %d)"), WeaponSpawnDelta));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyCombatGarageMapRuntime(UWorld* World)
{
	(void)World;
	const int32 StartingIssues = IssueCount;

	UWorld* MapWorld = LoadObject<UWorld>(nullptr, TEXT("/Game/FW/Maps/Test/L_Test_CombatGarage"));
	Check(MapWorld != nullptr, TEXT("Runtime Combat Garage map loads for map fixture test"));
	if (!MapWorld)
	{
		return IssueCount == StartingIssues;
	}

	bool bMapHasDirector = false;
	int32 MapPlayerSpawnCount = 0;
	int32 MapAISpawnCount = 0;
	int32 MapCarSpawnCount = 0;
	int32 MapMotorcycleSpawnCount = 0;
	int32 MapWeaponSpawnCount = 0;
	TArray<AFWCombatGarageSpawnPoint*> MapSpawnPoints;
	if (MapWorld->PersistentLevel)
	{
		for (AActor* Actor : MapWorld->PersistentLevel->Actors)
		{
			if (!Actor)
			{
				continue;
			}

			bMapHasDirector = bMapHasDirector || Actor->IsA<AFWCombatGarageTestDirector>();
			if (AFWCombatGarageSpawnPoint* SpawnPoint = Cast<AFWCombatGarageSpawnPoint>(Actor))
			{
				MapSpawnPoints.Add(SpawnPoint);
				switch (SpawnPoint->SpawnType)
				{
				case EFWCombatGarageSpawnType::Player:
					++MapPlayerSpawnCount;
					break;
				case EFWCombatGarageSpawnType::AI:
					++MapAISpawnCount;
					break;
				case EFWCombatGarageSpawnType::Car:
					++MapCarSpawnCount;
					break;
				case EFWCombatGarageSpawnType::Motorcycle:
					++MapMotorcycleSpawnCount;
					break;
				case EFWCombatGarageSpawnType::Weapon:
					++MapWeaponSpawnCount;
					break;
				default:
					break;
				}
			}
		}
	}

	AFWCombatGarageSpawnPoint* PlayerSpawn = nullptr;
	for (AFWCombatGarageSpawnPoint* SpawnPoint : MapSpawnPoints)
	{
		if (SpawnPoint && SpawnPoint->SpawnType == EFWCombatGarageSpawnType::Player)
		{
			PlayerSpawn = SpawnPoint;
			break;
		}
	}

	Check(bMapHasDirector, TEXT("Runtime Combat Garage map has director"));
	Check(MapPlayerSpawnCount >= 1, FString::Printf(TEXT("Runtime Combat Garage map has player spawn fixture(s) (count: %d)"), MapPlayerSpawnCount));
	Check(MapAISpawnCount >= 3, FString::Printf(TEXT("Runtime Combat Garage map has at least three AI spawn fixtures (count: %d)"), MapAISpawnCount));
	Check(MapCarSpawnCount >= 1, FString::Printf(TEXT("Runtime Combat Garage map has car spawn fixture(s) (count: %d)"), MapCarSpawnCount));
	Check(MapMotorcycleSpawnCount >= 1, FString::Printf(TEXT("Runtime Combat Garage map has motorcycle spawn fixture(s) (count: %d)"), MapMotorcycleSpawnCount));
	Check(MapWeaponSpawnCount >= 1, FString::Printf(TEXT("Runtime Combat Garage map has weapon spawn fixture(s) (count: %d)"), MapWeaponSpawnCount));
	Check(PlayerSpawn != nullptr, TEXT("Runtime Combat Garage map exposes a concrete player spawn actor"));
	if (!bMapHasDirector || !PlayerSpawn || MapAISpawnCount < 3 || MapCarSpawnCount < 1 || MapMotorcycleSpawnCount < 1 || MapWeaponSpawnCount < 1)
	{
		return IssueCount == StartingIssues;
	}

	if (!GEngine)
	{
		Check(false, TEXT("GEngine is available for Combat Garage map runtime world"));
		return IssueCount == StartingIssues;
	}

	UWorld* RuntimeWorld = UWorld::CreateWorld(EWorldType::Game, false, TEXT("FW_MVP_CombatGarageMapRuntime"));
	Check(RuntimeWorld != nullptr, TEXT("Runtime Combat Garage map fixture transient world can be created"));
	if (!RuntimeWorld)
	{
		return IssueCount == StartingIssues;
	}

	FWorldContext& RuntimeWorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	RuntimeWorldContext.SetCurrentWorld(RuntimeWorld);

	auto FinishRuntimeWorld = [&RuntimeWorld]()
	{
		RuntimeWorld->DestroyWorld(false);
		GEngine->DestroyWorldContext(RuntimeWorld);
	};

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;

	AFWPlayerController* PlayerController = RuntimeWorld->SpawnActor<AFWPlayerController>(AFWPlayerController::StaticClass(), PlayerSpawn->GetActorLocation(), PlayerSpawn->GetActorRotation(), SpawnParameters);
	AFWPlayerState* PlayerState = RuntimeWorld->SpawnActor<AFWPlayerState>(AFWPlayerState::StaticClass(), PlayerSpawn->GetActorLocation(), PlayerSpawn->GetActorRotation(), SpawnParameters);
	AFWPlayerCharacter* Player = RuntimeWorld->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), PlayerSpawn->GetActorLocation() + FVector(150.0f, 0.0f, 0.0f), PlayerSpawn->GetActorRotation(), SpawnParameters);
	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(PlayerState);
	BeginActorForRuntimeVerify(Player);

	Check(PlayerController != nullptr, TEXT("Runtime Combat Garage map player controller spawns"));
	Check(PlayerState != nullptr, TEXT("Runtime Combat Garage map player state spawns"));
	Check(Player != nullptr, TEXT("Runtime Combat Garage map player spawns"));
	if (!PlayerController || !PlayerState || !Player)
	{
		FinishRuntimeWorld();
		return IssueCount == StartingIssues;
	}

	PlayerController->SetPlayerState(PlayerState);
	PlayerController->Possess(Player);

	for (const AFWCombatGarageSpawnPoint* MapSpawnPoint : MapSpawnPoints)
	{
		if (!MapSpawnPoint)
		{
			continue;
		}

		AFWCombatGarageSpawnPoint* RuntimeSpawnPoint = RuntimeWorld->SpawnActor<AFWCombatGarageSpawnPoint>(
			AFWCombatGarageSpawnPoint::StaticClass(),
			MapSpawnPoint->GetActorTransform(),
			SpawnParameters);
		if (RuntimeSpawnPoint)
		{
			RuntimeSpawnPoint->SpawnType = MapSpawnPoint->SpawnType;
			RuntimeSpawnPoint->SpawnId = MapSpawnPoint->SpawnId;
		}
	}

	AFWCombatGarageTestDirector* RuntimeDirector = RuntimeWorld->SpawnActor<AFWCombatGarageTestDirector>(
		AFWCombatGarageTestDirector::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	BeginActorForRuntimeVerify(RuntimeDirector, false);
	Check(RuntimeDirector != nullptr, TEXT("Runtime Combat Garage map fixture director spawns from map data"));
	if (!RuntimeDirector)
	{
		FinishRuntimeWorld();
		return IssueCount == StartingIssues;
	}

	FString ValidationMessage;
	Check(RuntimeDirector->ValidateCombatGarageSetup(ValidationMessage), FString::Printf(TEXT("Runtime Combat Garage map fixture validates before setup: %s"), *ValidationMessage));

	auto CountActors = [RuntimeWorld](UClass* ActorClass)
	{
		int32 Count = 0;
		for (TActorIterator<AActor> It(RuntimeWorld); It; ++It)
		{
			if (It->IsA(ActorClass))
			{
				++Count;
			}
		}
		return Count;
	};

	const int32 AICountBefore = CountActors(AFWAICharacter::StaticClass());
	const int32 CarCountBefore = CountActors(AFWCarVehicle::StaticClass());
	const int32 MotorcycleCountBefore = CountActors(AFWMotorcycleVehicle::StaticClass());
	const int32 WeaponCountBefore = CountActors(AFWWeaponBase::StaticClass());

	RuntimeDirector->SetupCombatGarage();
	for (TActorIterator<AActor> It(RuntimeWorld); It; ++It)
	{
		BeginActorForRuntimeVerify(*It);
	}

	const int32 AISpawnDelta = CountActors(AFWAICharacter::StaticClass()) - AICountBefore;
	const int32 CarSpawnDelta = CountActors(AFWCarVehicle::StaticClass()) - CarCountBefore;
	const int32 MotorcycleSpawnDelta = CountActors(AFWMotorcycleVehicle::StaticClass()) - MotorcycleCountBefore;
	const int32 WeaponSpawnDelta = CountActors(AFWWeaponBase::StaticClass()) - WeaponCountBefore;
	const float PlayerSpawnDistance = FVector::Dist(Player->GetActorLocation(), PlayerSpawn->GetActorLocation());

	Check(PlayerSpawnDistance < 5.0f, FString::Printf(TEXT("Runtime Combat Garage map moves player to player spawn (distance: %.2f)"), PlayerSpawnDistance));
	Check(AISpawnDelta >= 3, FString::Printf(TEXT("Runtime Combat Garage map director spawns at least three AI (delta: %d)"), AISpawnDelta));
	Check(CarSpawnDelta >= 1, FString::Printf(TEXT("Runtime Combat Garage map director spawns a car (delta: %d)"), CarSpawnDelta));
	Check(MotorcycleSpawnDelta >= 1, FString::Printf(TEXT("Runtime Combat Garage map director spawns a motorcycle (delta: %d)"), MotorcycleSpawnDelta));
	Check(WeaponSpawnDelta >= 1, FString::Printf(TEXT("Runtime Combat Garage map director spawns at least one test weapon (delta: %d)"), WeaponSpawnDelta));
	Check(PlayerState->CoreVehicle != nullptr, TEXT("Runtime Combat Garage map registers first spawned vehicle as player core vehicle"));
	Check(PlayerState->CoreVehicle && PlayerState->CoreVehicle->IsCoreVehicle(), TEXT("Runtime Combat Garage map marks registered vehicle as core vehicle"));

	UFWHealthComponent* PlayerHealth = Player->GetHealthComponent();
	UFWStateMachineComponent* PlayerStateMachine = Player->GetStateMachineComponent();
	Check(PlayerHealth != nullptr, TEXT("Runtime Combat Garage safe start player has health component"));
	Check(PlayerStateMachine != nullptr, TEXT("Runtime Combat Garage safe start player has state machine"));
	if (PlayerHealth && PlayerStateMachine)
	{
		const float InitialPlayerHealth = PlayerHealth->CurrentHealth;
		const FGameplayTag DeadState = FGameplayTag::RequestGameplayTag(TEXT("State.Character.Dead"));
		const FGameplayTag AIAttackState = FGameplayTag::RequestGameplayTag(TEXT("State.AI.AttackOnFoot"));
		bool bAnyAIStartedInAttackRange = false;
		int32 TickedAIControllers = 0;

		for (TActorIterator<AFWAICharacter> It(RuntimeWorld); It; ++It)
		{
			AFWAICharacter* AICharacter = *It;
			if (!AICharacter)
			{
				continue;
			}

			if (!AICharacter->GetController())
			{
				AICharacter->SpawnDefaultController();
			}

			AFWEnemyAIController* AIController = Cast<AFWEnemyAIController>(AICharacter->GetController());
			if (!AIController)
			{
				continue;
			}

			for (int32 Step = 0; Step < 20; ++Step)
			{
				AIController->Tick(0.25f);
			}
			++TickedAIControllers;

			if (const UFWStateMachineComponent* AIStateMachine = AICharacter->GetStateMachineComponent())
			{
				if (AIStateMachine->CurrentState == AIAttackState)
				{
					bAnyAIStartedInAttackRange = true;
				}
			}
		}

		Check(TickedAIControllers >= 3, FString::Printf(TEXT("Runtime Combat Garage safe start ticks default AI controllers (count: %d)"), TickedAIControllers));
		Check(!bAnyAIStartedInAttackRange, TEXT("Runtime Combat Garage safe start keeps default AI out of AttackOnFoot"));
		Check(FMath::IsNearlyEqual(PlayerHealth->CurrentHealth, InitialPlayerHealth),
			FString::Printf(TEXT("Runtime Combat Garage safe start preserves player health (initial: %.1f after: %.1f)"), InitialPlayerHealth, PlayerHealth->CurrentHealth));
		Check(PlayerHealth->CurrentHealth > 0.0f && !PlayerHealth->IsDead(), TEXT("Runtime Combat Garage safe start leaves player alive"));
		Check(PlayerStateMachine->CurrentState != DeadState,
			FString::Printf(TEXT("Runtime Combat Garage safe start keeps player out of Dead state (state: %s)"), *PlayerStateMachine->CurrentState.ToString()));
	}

	FinishRuntimeWorld();
	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyHUDSnapshotRuntime(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFWPlayerController* PlayerController = World->SpawnActor<AFWPlayerController>(AFWPlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerState* PlayerState = World->SpawnActor<AFWPlayerState>(AFWPlayerState::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWCarVehicle* CoreVehicle = World->SpawnActor<AFWCarVehicle>(AFWCarVehicle::StaticClass(), FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	AFWWeaponBase* Weapon = World->SpawnActor<AFWWeaponBase>(AFWPistolWeapon::StaticClass(), Player ? Player->GetActorTransform() : FTransform::Identity, SpawnParameters);
	AFWAICharacter* AI = World->SpawnActor<AFWAICharacter>(AFWAICharacter::StaticClass(), FVector(500.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);

	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(PlayerState);
	BeginActorForRuntimeVerify(Player);
	BeginActorForRuntimeVerify(CoreVehicle);
	BeginActorForRuntimeVerify(Weapon);
	BeginActorForRuntimeVerify(AI);

	Check(PlayerController != nullptr, TEXT("Runtime HUD player controller spawns"));
	Check(PlayerState != nullptr, TEXT("Runtime HUD player state spawns"));
	Check(Player != nullptr, TEXT("Runtime HUD player character spawns"));
	Check(CoreVehicle != nullptr, TEXT("Runtime HUD core vehicle spawns"));
	Check(Weapon != nullptr, TEXT("Runtime HUD weapon spawns"));
	Check(AI != nullptr, TEXT("Runtime HUD AI target spawns"));
	if (!PlayerController || !PlayerState || !Player || !CoreVehicle || !Weapon || !AI)
	{
		return IssueCount == StartingIssues;
	}

	PlayerController->SetPlayerState(PlayerState);
	PlayerController->Possess(Player);
	Player->EquipWeapon(Weapon);
	PlayerState->SetLives(2);
	PlayerState->SetCoreVehicle(CoreVehicle);

	if (UFWHealthComponent* CoreHealth = CoreVehicle->FindComponentByClass<UFWHealthComponent>())
	{
		CoreHealth->ApplyDamage(25.0f);
	}

	FFWHUDStateSnapshot OnFootSnapshot = PlayerController->BuildHUDStateSnapshot();
	Check(FMath::IsNearlyEqual(OnFootSnapshot.PlayerHealth, 100.0f), FString::Printf(TEXT("Runtime HUD captures player health (actual: %.1f)"), OnFootSnapshot.PlayerHealth));
	Check(OnFootSnapshot.Lives == 2, FString::Printf(TEXT("Runtime HUD captures player lives (actual: %d)"), OnFootSnapshot.Lives));
	Check(OnFootSnapshot.WeaponContentId == TEXT("Weapon.Pistol.MVP"), FString::Printf(TEXT("Runtime HUD captures weapon ContentId (actual: %s)"), *OnFootSnapshot.WeaponContentId.ToString()));
	Check(OnFootSnapshot.CurrentAmmo > 0, FString::Printf(TEXT("Runtime HUD captures current ammo (actual: %d)"), OnFootSnapshot.CurrentAmmo));
	Check(OnFootSnapshot.CoreVehicleHealth > 0.0f && OnFootSnapshot.CoreVehicleHealth < OnFootSnapshot.CoreVehicleMaxHealth,
		FString::Printf(TEXT("Runtime HUD captures damaged core vehicle health (actual: %.1f/%.1f)"), OnFootSnapshot.CoreVehicleHealth, OnFootSnapshot.CoreVehicleMaxHealth));
	Check(OnFootSnapshot.AICount >= 1, FString::Printf(TEXT("Runtime HUD captures AI count (actual: %d)"), OnFootSnapshot.AICount));
	Check(OnFootSnapshot.AliveAICount >= 1, FString::Printf(TEXT("Runtime HUD captures alive AI count (actual: %d)"), OnFootSnapshot.AliveAICount));
	Check(FMath::IsNearlyEqual(OnFootSnapshot.NearestAIHealth, 100.0f), FString::Printf(TEXT("Runtime HUD captures nearest AI health (actual: %.1f)"), OnFootSnapshot.NearestAIHealth));
	Check(OnFootSnapshot.NearestAIStateTag == FGameplayTag::RequestGameplayTag(TEXT("State.AI.Idle")),
		FString::Printf(TEXT("Runtime HUD captures nearest AI state (actual: %s)"), *OnFootSnapshot.NearestAIStateTag.ToString()));
	Check(OnFootSnapshot.VehicleCount >= 1, FString::Printf(TEXT("Runtime HUD captures vehicle count (actual: %d)"), OnFootSnapshot.VehicleCount));
	Check(OnFootSnapshot.ActiveVehicleCount >= 1, FString::Printf(TEXT("Runtime HUD captures active vehicle count (actual: %d)"), OnFootSnapshot.ActiveVehicleCount));
	Check(OnFootSnapshot.NearestVehicleHealth > 0.0f && OnFootSnapshot.NearestVehicleMaxHealth > 0.0f,
		FString::Printf(TEXT("Runtime HUD captures nearest active vehicle health (actual: %.1f/%.1f)"), OnFootSnapshot.NearestVehicleHealth, OnFootSnapshot.NearestVehicleMaxHealth));
	Check(OnFootSnapshot.NearestVehicleStateTag.IsValid() && OnFootSnapshot.NearestVehicleStateTag != FGameplayTag::RequestGameplayTag(TEXT("State.Vehicle.Destroyed")),
		FString::Printf(TEXT("Runtime HUD captures nearest vehicle state (actual: %s)"), *OnFootSnapshot.NearestVehicleStateTag.ToString()));

	CoreVehicle->EnterVehicle(PlayerController);
	const FFWHUDStateSnapshot VehicleSnapshot = PlayerController->BuildHUDStateSnapshot();
	Check(VehicleSnapshot.VehicleHealth > 0.0f, FString::Printf(TEXT("Runtime HUD captures possessed vehicle health (actual: %.1f)"), VehicleSnapshot.VehicleHealth));
	Check(FMath::IsNearlyEqual(VehicleSnapshot.PlayerHealth, OnFootSnapshot.PlayerHealth), TEXT("Runtime HUD keeps previous player health while vehicle is possessed"));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyRulesRespawnRuntime(UWorld* World)
{
	const int32 StartingIssues = IssueCount;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFWCompetitiveGameMode* BeginOrderGameMode = World->SpawnActor<AFWCompetitiveGameMode>(AFWCompetitiveGameMode::StaticClass(), FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator, SpawnParameters);
	if (BeginOrderGameMode)
	{
		BeginOrderGameMode->StartMatchFlow();
		BeginActorForRuntimeVerify(BeginOrderGameMode);
		Check(BeginOrderGameMode->GetCurrentMatchStateTag() == FGameplayTag::RequestGameplayTag(TEXT("Match.InProgress")),
			TEXT("Runtime rules GameMode BeginPlay preserves already-started match state"));
	}

	AFWCompetitiveGameMode* GameMode = World->SpawnActor<AFWCompetitiveGameMode>(AFWCompetitiveGameMode::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerController* PlayerController = World->SpawnActor<AFWPlayerController>(AFWPlayerController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerState* PlayerState = World->SpawnActor<AFWPlayerState>(AFWPlayerState::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerCharacter* Player = World->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);

	BeginActorForRuntimeVerify(GameMode);
	BeginActorForRuntimeVerify(PlayerController, false);
	BeginActorForRuntimeVerify(PlayerState);
	BeginActorForRuntimeVerify(Player);

	Check(GameMode != nullptr, TEXT("Runtime rules GameMode spawns"));
	Check(PlayerController != nullptr, TEXT("Runtime rules player controller spawns"));
	Check(PlayerState != nullptr, TEXT("Runtime rules player state spawns"));
	Check(Player != nullptr, TEXT("Runtime rules player spawns"));
	if (!GameMode || !PlayerController || !PlayerState || !Player)
	{
		return IssueCount == StartingIssues;
	}

	PlayerController->SetPlayerState(PlayerState);
	PlayerController->Possess(Player);

	UFWMatchRuleSet* RuleSet = NewObject<UFWMatchRuleSet>(GameMode);
	RuleSet->PlayerLives = 2;
	RuleSet->bRespawnEnabled = true;
	RuleSet->RespawnDelay = 0.0f;
	RuleSet->bCoreVehicleEnabled = true;
	RuleSet->CoreVehiclePenalty = 1;

	GameMode->SetMatchRuleSet(RuleSet);
	PlayerState->SetLives(RuleSet->PlayerLives);
	Check(PlayerState->Lives == 2, FString::Printf(TEXT("Runtime rules initialize player lives (actual: %d)"), PlayerState->Lives));

	GameMode->StartMatchFlow();
	GameMode->HandleCoreVehicleDestroyed(PlayerController);
	Check(PlayerState->Lives == 1, FString::Printf(TEXT("Runtime core vehicle penalty subtracts lives (actual: %d)"), PlayerState->Lives));
	Check(GameMode->GetCurrentMatchStateTag() == FGameplayTag::RequestGameplayTag(TEXT("Match.InProgress")), TEXT("Runtime respawn returns match to InProgress when lives remain"));
	Check(PlayerController->GetPawn() != nullptr, TEXT("Runtime respawn leaves player controller with a pawn"));

	PlayerState->SetLives(1);
	GameMode->StartMatchFlow();
	GameMode->HandleCoreVehicleDestroyed(PlayerController);
	Check(PlayerState->Lives == 0, FString::Printf(TEXT("Runtime core vehicle penalty can reduce lives to zero (actual: %d)"), PlayerState->Lives));
	Check(GameMode->GetCurrentMatchStateTag() == FGameplayTag::RequestGameplayTag(TEXT("Match.Ended")), TEXT("Runtime lives reaching zero ends match"));

	return IssueCount == StartingIssues;
}

bool UFWMVPRuntimeVerifyCommandlet::VerifyDebugRuntime()
{
	const int32 StartingIssues = IssueCount;

	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UFWDebugSubsystem* DebugSubsystem = GameInstance ? NewObject<UFWDebugSubsystem>(GameInstance) : nullptr;
	Check(DebugSubsystem != nullptr, TEXT("Runtime debug subsystem object can be created"));
	if (!DebugSubsystem)
	{
		return IssueCount == StartingIssues;
	}

	DebugSubsystem->SetDebugEnabled(true);
	Check(DebugSubsystem->IsDebugEnabled(), TEXT("Runtime debug subsystem toggles enabled state"));

	DebugSubsystem->ClearRecentEvents();
	for (int32 Index = 0; Index < 70; ++Index)
	{
		FFWGameplayEvent Event;
		Event.EventTag = FGameplayTag::RequestGameplayTag(Index == 69 ? TEXT("Event.Respawn.Completed") : TEXT("Event.Weapon.Fired"));
		Event.Magnitude = static_cast<float>(Index);
		Event.WorldLocation = FVector(static_cast<float>(Index), 0.0f, 0.0f);
		DebugSubsystem->RecordGameplayEvent(Event);
	}

	const TArray<FFWDebugEventRecord>& Events = DebugSubsystem->GetRecentEvents();
	Check(Events.Num() == 64, FString::Printf(TEXT("Runtime debug event buffer caps recent events (actual: %d)"), Events.Num()));
	Check(Events.Num() > 0 && Events.Last().EventTag == FGameplayTag::RequestGameplayTag(TEXT("Event.Respawn.Completed")), TEXT("Runtime debug event buffer keeps newest event"));
	Check(Events.Num() > 0 && FMath::IsNearlyEqual(Events.Last().Magnitude, 69.0f), FString::Printf(TEXT("Runtime debug event buffer preserves event magnitude (actual: %.1f)"), Events.Num() > 0 ? Events.Last().Magnitude : -1.0f));

	DebugSubsystem->ClearRecentEvents();
	Check(DebugSubsystem->GetRecentEvents().Num() == 0, TEXT("Runtime debug event buffer clears events"));

	return IssueCount == StartingIssues;
}
