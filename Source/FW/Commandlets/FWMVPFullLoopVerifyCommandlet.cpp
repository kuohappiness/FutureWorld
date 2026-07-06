#include "FWMVPFullLoopVerifyCommandlet.h"

#include "../AI/FWEnemyAIController.h"
#include "../Characters/FWAICharacter.h"
#include "../Characters/FWPlayerCharacter.h"
#include "../Combat/FWHealthComponent.h"
#include "../Core/FWCompetitiveGameMode.h"
#include "../Core/FWPlayerController.h"
#include "../Core/FWPlayerState.h"
#include "../Rules/FWMatchRuleSet.h"
#include "../State/FWStateMachineComponent.h"
#include "../UI/FWHUDTypes.h"
#include "../Vehicles/FWMVPVehicleClasses.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Weapons/FWWeaponBase.h"
#include "../World/FWCombatGarageSpawnPoint.h"
#include "../World/FWCombatGarageTestDirector.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "InputActionValue.h"
#include "UObject/UnrealType.h"

namespace
{
	void BeginActorForFullLoopVerify(AActor* Actor, bool bDispatchBeginPlay = true)
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

	template <typename TActor>
	TActor* FindFirstActor(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<TActor> It(World); It; ++It)
		{
			return *It;
		}

		return nullptr;
	}

	template <typename TActor>
	int32 CountActors(UWorld* World)
	{
		int32 Count = 0;
		if (!World)
		{
			return Count;
		}

		for (TActorIterator<TActor> It(World); It; ++It)
		{
			++Count;
		}

		return Count;
	}

	bool SetFloatPropertyForFullLoopVerify(UObject* Object, const FName PropertyName, float Value)
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

UFWMVPFullLoopVerifyCommandlet::UFWMVPFullLoopVerifyCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UFWMVPFullLoopVerifyCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("FW MVP headless full-loop verification started."));
	IssueCount = 0;

	if (!VerifyHeadlessFullLoop())
	{
		++IssueCount;
	}

	if (IssueCount > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FW MVP headless full-loop verification failed with %d issue(s)."), IssueCount);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("FW MVP headless full-loop verification passed."));
	return 0;
}

void UFWMVPFullLoopVerifyCommandlet::Check(bool bCondition, const FString& Message)
{
	if (bCondition)
	{
		UE_LOG(LogTemp, Display, TEXT("[OK] %s"), *Message);
		return;
	}

	++IssueCount;
	UE_LOG(LogTemp, Error, TEXT("[FAIL] %s"), *Message);
}

bool UFWMVPFullLoopVerifyCommandlet::VerifyHeadlessFullLoop()
{
	const int32 StartingIssues = IssueCount;

	if (!GEngine)
	{
		Check(false, TEXT("GEngine is available for headless full-loop verification"));
		return false;
	}

	UWorld* MapWorld = LoadObject<UWorld>(nullptr, TEXT("/Game/FW/Maps/Test/L_Test_CombatGarage"));
	Check(MapWorld != nullptr, TEXT("Full-loop verifier loads canonical L_Test_CombatGarage fixture"));
	if (!MapWorld)
	{
		return IssueCount == StartingIssues;
	}

	TArray<AFWCombatGarageSpawnPoint*> MapSpawnPoints;
	if (MapWorld->PersistentLevel)
	{
		for (AActor* Actor : MapWorld->PersistentLevel->Actors)
		{
			if (AFWCombatGarageSpawnPoint* SpawnPoint = Cast<AFWCombatGarageSpawnPoint>(Actor))
			{
				MapSpawnPoints.Add(SpawnPoint);
			}
		}
	}
	Check(MapSpawnPoints.Num() >= 7, FString::Printf(TEXT("Full-loop fixture exposes expected spawn point set (count: %d)"), MapSpawnPoints.Num()));

	AFWCombatGarageSpawnPoint* PlayerSpawn = nullptr;
	for (AFWCombatGarageSpawnPoint* SpawnPoint : MapSpawnPoints)
	{
		if (SpawnPoint && SpawnPoint->SpawnType == EFWCombatGarageSpawnType::Player)
		{
			PlayerSpawn = SpawnPoint;
			break;
		}
	}
	Check(PlayerSpawn != nullptr, TEXT("Full-loop fixture has a player spawn"));
	if (!PlayerSpawn)
	{
		return IssueCount == StartingIssues;
	}

	UWorld* RuntimeWorld = UWorld::CreateWorld(EWorldType::Game, false, TEXT("FW_MVP_HeadlessFullLoop"));
	Check(RuntimeWorld != nullptr, TEXT("Headless full-loop transient world can be created"));
	if (!RuntimeWorld)
	{
		return IssueCount == StartingIssues;
	}

	FWorldContext& RuntimeWorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	RuntimeWorldContext.SetCurrentWorld(RuntimeWorld);

	auto FinishRuntimeWorld = [RuntimeWorld]()
	{
		RuntimeWorld->DestroyWorld(false);
		GEngine->DestroyWorldContext(RuntimeWorld);
	};

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;

	AFWCompetitiveGameMode* GameMode = RuntimeWorld->SpawnActor<AFWCompetitiveGameMode>(AFWCompetitiveGameMode::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AFWPlayerController* PlayerController = RuntimeWorld->SpawnActor<AFWPlayerController>(AFWPlayerController::StaticClass(), PlayerSpawn->GetActorLocation(), PlayerSpawn->GetActorRotation(), SpawnParameters);
	AFWPlayerState* PlayerState = RuntimeWorld->SpawnActor<AFWPlayerState>(AFWPlayerState::StaticClass(), PlayerSpawn->GetActorLocation(), PlayerSpawn->GetActorRotation(), SpawnParameters);
	AFWPlayerCharacter* Player = RuntimeWorld->SpawnActor<AFWPlayerCharacter>(AFWPlayerCharacter::StaticClass(), PlayerSpawn->GetActorLocation(), PlayerSpawn->GetActorRotation(), SpawnParameters);
	RuntimeWorld->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), PlayerSpawn->GetActorLocation(), PlayerSpawn->GetActorRotation(), SpawnParameters);

	BeginActorForFullLoopVerify(GameMode);
	BeginActorForFullLoopVerify(PlayerController, false);
	BeginActorForFullLoopVerify(PlayerState);
	BeginActorForFullLoopVerify(Player);

	Check(GameMode != nullptr, TEXT("Headless full-loop GameMode spawns"));
	Check(PlayerController != nullptr, TEXT("Headless full-loop player controller spawns"));
	Check(PlayerState != nullptr, TEXT("Headless full-loop player state spawns"));
	Check(Player != nullptr, TEXT("Headless full-loop player character spawns"));
	if (!GameMode || !PlayerController || !PlayerState || !Player)
	{
		FinishRuntimeWorld();
		return IssueCount == StartingIssues;
	}

	PlayerState->SetOwner(PlayerController);
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
			BeginActorForFullLoopVerify(RuntimeSpawnPoint);
		}
	}

	AFWCombatGarageTestDirector* Director = RuntimeWorld->SpawnActor<AFWCombatGarageTestDirector>(AFWCombatGarageTestDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	BeginActorForFullLoopVerify(Director, false);
	Check(Director != nullptr, TEXT("Headless full-loop Combat Garage director spawns"));
	if (!Director)
	{
		FinishRuntimeWorld();
		return IssueCount == StartingIssues;
	}

	FString ValidationMessage;
	Check(Director->ValidateCombatGarageSetup(ValidationMessage), FString::Printf(TEXT("Headless full-loop fixture validates before setup: %s"), *ValidationMessage));

	const int32 AICountBefore = CountActors<AFWAICharacter>(RuntimeWorld);
	const int32 CarCountBefore = CountActors<AFWCarVehicle>(RuntimeWorld);
	const int32 MotorcycleCountBefore = CountActors<AFWMotorcycleVehicle>(RuntimeWorld);
	const int32 WeaponCountBefore = CountActors<AFWWeaponBase>(RuntimeWorld);

	Director->SetupCombatGarage();
	for (TActorIterator<AActor> It(RuntimeWorld); It; ++It)
	{
		BeginActorForFullLoopVerify(*It);
	}
	GameMode->StartMatchFlow();

	const int32 AISpawnDelta = CountActors<AFWAICharacter>(RuntimeWorld) - AICountBefore;
	const int32 CarSpawnDelta = CountActors<AFWCarVehicle>(RuntimeWorld) - CarCountBefore;
	const int32 MotorcycleSpawnDelta = CountActors<AFWMotorcycleVehicle>(RuntimeWorld) - MotorcycleCountBefore;
	const int32 WeaponSpawnDelta = CountActors<AFWWeaponBase>(RuntimeWorld) - WeaponCountBefore;
	const float PlayerSpawnDistance = FVector::Dist(Player->GetActorLocation(), PlayerSpawn->GetActorLocation());

	Check(PlayerSpawnDistance < 5.0f, FString::Printf(TEXT("Headless full-loop director places player at fixture spawn (distance: %.2f)"), PlayerSpawnDistance));
	Check(AISpawnDelta >= 3, FString::Printf(TEXT("Headless full-loop director spawns at least three AI (delta: %d)"), AISpawnDelta));
	Check(CarSpawnDelta >= 1, FString::Printf(TEXT("Headless full-loop director spawns car (delta: %d)"), CarSpawnDelta));
	Check(MotorcycleSpawnDelta >= 1, FString::Printf(TEXT("Headless full-loop director spawns motorcycle (delta: %d)"), MotorcycleSpawnDelta));
	Check(WeaponSpawnDelta >= 1, FString::Printf(TEXT("Headless full-loop director spawns test weapon (delta: %d)"), WeaponSpawnDelta));
	Check(PlayerState->CoreVehicle != nullptr, TEXT("Headless full-loop registers a player core vehicle"));
	Check(GameMode->GetCurrentMatchStateTag() == FGameplayTag::RequestGameplayTag(TEXT("Match.InProgress")), TEXT("Headless full-loop match reaches InProgress"));

	AFWAICharacter* TargetAI = FindFirstActor<AFWAICharacter>(RuntimeWorld);
	AFWVehicleBase* CoreVehicle = PlayerState->CoreVehicle;
	AFWWeaponBase* PlayerWeapon = Player->GetCurrentWeapon();
	BeginActorForFullLoopVerify(PlayerWeapon);
	Check(TargetAI != nullptr, TEXT("Headless full-loop has an AI combat target"));
	Check(CoreVehicle != nullptr, TEXT("Headless full-loop has a core vehicle target"));
	Check(PlayerWeapon != nullptr, TEXT("Headless full-loop player has weapon after setup/default loadout"));
	if (!TargetAI || !CoreVehicle || !PlayerWeapon)
	{
		FinishRuntimeWorld();
		return IssueCount == StartingIssues;
	}

	UFWHealthComponent* AIHealth = TargetAI->GetHealthComponent();
	UFWHealthComponent* CoreVehicleHealth = CoreVehicle->FindComponentByClass<UFWHealthComponent>();
	Check(AIHealth != nullptr, TEXT("Headless full-loop AI target has health"));
	Check(CoreVehicleHealth != nullptr, TEXT("Headless full-loop core vehicle has health"));
	if (!AIHealth || !CoreVehicleHealth)
	{
		FinishRuntimeWorld();
		return IssueCount == StartingIssues;
	}

	const float AIHealthBefore = AIHealth->CurrentHealth;
	const int32 AmmoBeforeAIShot = PlayerWeapon->GetCurrentAmmo();
	Check(PlayerWeapon->FireAtActor(TargetAI), TEXT("Headless full-loop player weapon fires at spawned AI"));
	Check(PlayerWeapon->GetCurrentAmmo() == AmmoBeforeAIShot - 1, TEXT("Headless full-loop player weapon consumes ammo"));
	Check(AIHealth->CurrentHealth < AIHealthBefore,
		FString::Printf(TEXT("Headless full-loop player damages spawned AI (before: %.1f after: %.1f)"), AIHealthBefore, AIHealth->CurrentHealth));

	AFWEnemyAIController* SpawnedAIController = Cast<AFWEnemyAIController>(TargetAI->GetController());
	if (!SpawnedAIController)
	{
		TargetAI->SpawnDefaultController();
		SpawnedAIController = Cast<AFWEnemyAIController>(TargetAI->GetController());
	}
	Check(SpawnedAIController != nullptr, TEXT("Headless full-loop spawned AI has enemy AI controller"));

	UFWHealthComponent* PlayerHealth = Player->GetHealthComponent();
	UFWStateMachineComponent* AIStateMachine = TargetAI->GetStateMachineComponent();
	AFWWeaponBase* AIWeapon = TargetAI->GetCurrentWeapon();
	BeginActorForFullLoopVerify(AIWeapon);
	Check(PlayerHealth != nullptr, TEXT("Headless full-loop player has health for spawned AI attack"));
	Check(AIStateMachine != nullptr, TEXT("Headless full-loop spawned AI has state machine"));
	Check(AIWeapon != nullptr, TEXT("Headless full-loop spawned AI has weapon"));
	if (SpawnedAIController && PlayerHealth && AIStateMachine && AIWeapon)
	{
		Check(SetFloatPropertyForFullLoopVerify(SpawnedAIController, TEXT("DetectionRadius"), 3000.0f), TEXT("Headless full-loop configures spawned AI detection radius"));
		Check(SetFloatPropertyForFullLoopVerify(SpawnedAIController, TEXT("AttackRange"), 700.0f), TEXT("Headless full-loop configures spawned AI attack range"));
		Check(SetFloatPropertyForFullLoopVerify(SpawnedAIController, TEXT("AttackInterval"), 0.01f), TEXT("Headless full-loop configures spawned AI attack interval"));
		Check(SetFloatPropertyForFullLoopVerify(SpawnedAIController, TEXT("Accuracy"), 1.0f), TEXT("Headless full-loop configures spawned AI accuracy"));

		TargetAI->SetActorLocation(Player->GetActorLocation() + FVector(450.0f, 0.0f, 0.0f));
		const float PlayerHealthBeforeAIAttack = PlayerHealth->CurrentHealth;
		const int32 AIAmmoBeforeAttack = AIWeapon->GetCurrentAmmo();
		SpawnedAIController->Tick(0.5f);

		Check(AIStateMachine->CurrentState == FGameplayTag::RequestGameplayTag(TEXT("State.AI.AttackOnFoot")),
			FString::Printf(TEXT("Headless full-loop spawned AI enters AttackOnFoot (state: %s)"), *AIStateMachine->CurrentState.ToString()));
		Check(PlayerHealth->CurrentHealth < PlayerHealthBeforeAIAttack,
			FString::Printf(TEXT("Headless full-loop spawned AI damages player (before: %.1f after: %.1f)"), PlayerHealthBeforeAIAttack, PlayerHealth->CurrentHealth));
		Check(AIWeapon->GetCurrentAmmo() == AIAmmoBeforeAttack - 1,
			FString::Printf(TEXT("Headless full-loop spawned AI consumes ammo (before: %d after: %d)"), AIAmmoBeforeAttack, AIWeapon->GetCurrentAmmo()));
	}

	Check(CoreVehicle->EnterVehicle(PlayerController), TEXT("Headless full-loop player enters core vehicle"));
	CoreVehicle->SetActorLocation(FVector(3000.0f, -1000.0f, 120.0f), false, nullptr, ETeleportType::TeleportPhysics);
	CoreVehicle->SetActorRotation(FRotator::ZeroRotator, ETeleportType::TeleportPhysics);
	const FVector VehicleStart = CoreVehicle->GetActorLocation();
	CoreVehicle->Move(FInputActionValue(FVector2D(0.0f, 1.0f)));
	for (int32 Step = 0; Step < 8; ++Step)
	{
		CoreVehicle->Tick(0.1f);
	}
	const FVector VehicleDriveDelta = CoreVehicle->GetActorLocation() - VehicleStart;
	Check(VehicleDriveDelta.Size() > 100.0f,
		FString::Printf(TEXT("Headless full-loop player drives core vehicle (delta: %s)"), *VehicleDriveDelta.ToCompactString()));
	CoreVehicle->HandleExitInput();
	Check(PlayerController->GetPawn() == Player, TEXT("Headless full-loop vehicle exit restores player pawn"));

	const FFWHUDStateSnapshot HUDBeforePenalty = PlayerController->BuildHUDStateSnapshot();
	Check(HUDBeforePenalty.Lives == 2, FString::Printf(TEXT("Headless full-loop HUD snapshot captures starting lives (actual: %d)"), HUDBeforePenalty.Lives));
	Check(HUDBeforePenalty.CoreVehicleHealth > 0.0f, FString::Printf(TEXT("Headless full-loop HUD snapshot captures core vehicle health (actual: %.1f)"), HUDBeforePenalty.CoreVehicleHealth));

	GameMode->HandleCoreVehicleDestroyed(PlayerController);
	const FFWHUDStateSnapshot HUDAfterPenalty = PlayerController->BuildHUDStateSnapshot();
	Check(PlayerState->Lives == 1, FString::Printf(TEXT("Headless full-loop core vehicle penalty subtracts one life (actual: %d)"), PlayerState->Lives));
	Check(GameMode->GetCurrentMatchStateTag() == FGameplayTag::RequestGameplayTag(TEXT("Match.InProgress")), TEXT("Headless full-loop respawn returns match to InProgress"));
	Check(PlayerController->GetPawn() != nullptr, TEXT("Headless full-loop respawn leaves controller with pawn"));
	Check(HUDAfterPenalty.Lives == 1, FString::Printf(TEXT("Headless full-loop HUD snapshot reflects life penalty (actual: %d)"), HUDAfterPenalty.Lives));

	GameMode->HandleCoreVehicleDestroyed(PlayerController);
	Check(PlayerState->Lives == 0, FString::Printf(TEXT("Headless full-loop second core vehicle penalty reaches zero lives (actual: %d)"), PlayerState->Lives));
	Check(GameMode->GetCurrentMatchStateTag() == FGameplayTag::RequestGameplayTag(TEXT("Match.Ended")), TEXT("Headless full-loop zero lives ends match"));

	FinishRuntimeWorld();
	return IssueCount == StartingIssues;
}
