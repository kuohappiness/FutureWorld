#include "FWCombatGarageTestDirector.h"

#include "../Characters/FWAICharacter.h"
#include "../Characters/FWCharacterBase.h"
#include "../Core/FWCompetitiveGameMode.h"
#include "../Core/FWPlayerState.h"
#include "../Events/FWEventSubsystem.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Vehicles/FWMVPVehicleClasses.h"
#include "../Weapons/FWWeaponBase.h"
#include "../Weapons/FWMVPWeaponClasses.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace FWCombatGarageEventTags
{
	const FName Ready(TEXT("Event.Test.CombatGarageReady"));
}

AFWCombatGarageTestDirector::AFWCombatGarageTestDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	ConfigureMVPDefaults();
}

void AFWCombatGarageTestDirector::ConfigureMVPDefaults()
{
	CarVehicleClass = AFWCarVehicle::StaticClass();
	MotorcycleVehicleClass = AFWMotorcycleVehicle::StaticClass();
	TestWeaponClass = AFWPistolWeapon::StaticClass();
	AICharacterClass = AFWAICharacter::StaticClass();
	DesiredAICount = 3;
	MinimumInitialAIDistanceFromPlayer = 2800.0f;
	bSetupOnBeginPlay = true;
	bStartMatchAfterSetup = true;
	BeginPlaySetupDelay = 0.0f;
}

void AFWCombatGarageTestDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bSetupOnBeginPlay)
	{
		if (BeginPlaySetupDelay <= 0.0f)
		{
			SetupCombatGarage();
		}
		else
		{
			GetWorldTimerManager().SetTimer(SetupTimerHandle, this, &AFWCombatGarageTestDirector::SetupCombatGarage, BeginPlaySetupDelay, false);
		}
	}
}

void AFWCombatGarageTestDirector::SetupCombatGarage()
{
	if (bHasSetup || !HasAuthority())
	{
		return;
	}

	FString ValidationMessage;
	if (!ValidateCombatGarageSetup(ValidationMessage))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *ValidationMessage);
	}

	bHasSetup = true;
	MovePlayerToSpawn();
	SpawnAI();
	SpawnVehicles();
	SpawnTestWeapons();
	BroadcastGarageReady();

	if (bStartMatchAfterSetup)
	{
		StartMatchFlow();
	}
}

bool AFWCombatGarageTestDirector::ValidateCombatGarageSetup(FString& OutValidationMessage) const
{
	TArray<FString> MissingItems;
	if (GetSpawnPointsByType(EFWCombatGarageSpawnType::Player).Num() == 0)
	{
		MissingItems.Add(TEXT("Player spawn point"));
	}
	if (GetSpawnPointsByType(EFWCombatGarageSpawnType::AI).Num() < DesiredAICount)
	{
		MissingItems.Add(TEXT("enough AI spawn points"));
	}
	if (GetSpawnPointsByType(EFWCombatGarageSpawnType::Car).Num() == 0)
	{
		MissingItems.Add(TEXT("car spawn point"));
	}
	if (GetSpawnPointsByType(EFWCombatGarageSpawnType::Motorcycle).Num() == 0)
	{
		MissingItems.Add(TEXT("motorcycle spawn point"));
	}
	if (!AICharacterClass)
	{
		MissingItems.Add(TEXT("AICharacterClass"));
	}

	if (MissingItems.Num() == 0)
	{
		OutValidationMessage = TEXT("Combat Garage setup is ready.");
		return true;
	}

	OutValidationMessage = FString::Printf(TEXT("Combat Garage missing: %s"), *FString::Join(MissingItems, TEXT(", ")));
	return false;
}

TArray<AFWCombatGarageSpawnPoint*> AFWCombatGarageTestDirector::GetSpawnPointsByType(EFWCombatGarageSpawnType SpawnType) const
{
	TArray<AFWCombatGarageSpawnPoint*> MatchingPoints;
	if (!GetWorld())
	{
		return MatchingPoints;
	}

	for (TActorIterator<AFWCombatGarageSpawnPoint> It(GetWorld()); It; ++It)
	{
		AFWCombatGarageSpawnPoint* SpawnPoint = *It;
		if (SpawnPoint && SpawnPoint->SpawnType == SpawnType)
		{
			MatchingPoints.Add(SpawnPoint);
		}
	}

	MatchingPoints.Sort([](const AFWCombatGarageSpawnPoint& A, const AFWCombatGarageSpawnPoint& B)
	{
		return A.SpawnId.LexicalLess(B.SpawnId);
	});
	return MatchingPoints;
}

APlayerController* AFWCombatGarageTestDirector::GetFirstPlayerController() const
{
	return GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
}

void AFWCombatGarageTestDirector::MovePlayerToSpawn() const
{
	const TArray<AFWCombatGarageSpawnPoint*> PlayerSpawns = GetSpawnPointsByType(EFWCombatGarageSpawnType::Player);
	APlayerController* PlayerController = GetFirstPlayerController();
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerSpawns.Num() == 0 || !PlayerPawn)
	{
		return;
	}

	PlayerPawn->SetActorLocationAndRotation(PlayerSpawns[0]->GetActorLocation(), PlayerSpawns[0]->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	PlayerController->SetControlRotation(PlayerSpawns[0]->GetActorRotation());
	PlayerController->SetViewTarget(PlayerPawn);

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	UE_LOG(LogTemp, Display, TEXT("CombatGarage player view aligned. Pawn=%s PawnLocation=%s ControlRotation=%s ViewLocation=%s ViewRotation=%s"),
		*GetNameSafe(PlayerPawn),
		*PlayerPawn->GetActorLocation().ToCompactString(),
		*PlayerController->GetControlRotation().ToCompactString(),
		*ViewLocation.ToCompactString(),
		*ViewRotation.ToCompactString());
}

void AFWCombatGarageTestDirector::SpawnAI()
{
	if (!AICharacterClass)
	{
		return;
	}

	const TArray<AFWCombatGarageSpawnPoint*> AISpawns = GetSpawnPointsByType(EFWCombatGarageSpawnType::AI);
	const int32 SpawnCount = FMath::Min(DesiredAICount, AISpawns.Num());
	const APlayerController* PlayerController = GetFirstPlayerController();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		FTransform SpawnTransform = AISpawns[Index]->GetActorTransform();
		if (PlayerPawn && MinimumInitialAIDistanceFromPlayer > 0.0f)
		{
			FVector SpawnLocation = SpawnTransform.GetLocation();
			FVector FromPlayer = SpawnLocation - PlayerLocation;
			FromPlayer.Z = 0.0f;
			const float Distance2D = FromPlayer.Size();
			if (Distance2D < MinimumInitialAIDistanceFromPlayer)
			{
				const FVector Direction = Distance2D > KINDA_SMALL_NUMBER ? FromPlayer / Distance2D : FVector::ForwardVector;
				SpawnLocation = PlayerLocation + Direction * MinimumInitialAIDistanceFromPlayer;
				SpawnLocation.Z = SpawnTransform.GetLocation().Z;
				SpawnTransform.SetLocation(SpawnLocation);
			}
		}
		GetWorld()->SpawnActor<AFWAICharacter>(AICharacterClass, SpawnTransform);
	}
}

void AFWCombatGarageTestDirector::SpawnVehicles()
{
	AFWVehicleBase* FirstSpawnedVehicle = nullptr;

	if (CarVehicleClass)
	{
		for (AFWCombatGarageSpawnPoint* CarSpawn : GetSpawnPointsByType(EFWCombatGarageSpawnType::Car))
		{
			AFWVehicleBase* SpawnedVehicle = GetWorld()->SpawnActor<AFWVehicleBase>(CarVehicleClass, CarSpawn->GetActorTransform());
			FirstSpawnedVehicle = FirstSpawnedVehicle ? FirstSpawnedVehicle : SpawnedVehicle;
		}
	}

	if (MotorcycleVehicleClass)
	{
		for (AFWCombatGarageSpawnPoint* MotorcycleSpawn : GetSpawnPointsByType(EFWCombatGarageSpawnType::Motorcycle))
		{
			AFWVehicleBase* SpawnedVehicle = GetWorld()->SpawnActor<AFWVehicleBase>(MotorcycleVehicleClass, MotorcycleSpawn->GetActorTransform());
			FirstSpawnedVehicle = FirstSpawnedVehicle ? FirstSpawnedVehicle : SpawnedVehicle;
		}
	}

	RegisterCoreVehicleForPlayer(FirstSpawnedVehicle);
}

void AFWCombatGarageTestDirector::SpawnTestWeapons()
{
	if (!TestWeaponClass)
	{
		return;
	}

	for (AFWCombatGarageSpawnPoint* WeaponSpawn : GetSpawnPointsByType(EFWCombatGarageSpawnType::Weapon))
	{
		AFWWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AFWWeaponBase>(TestWeaponClass, WeaponSpawn->GetActorTransform());
		if (SpawnedWeapon)
		{
			if (APlayerController* PlayerController = GetFirstPlayerController())
			{
				if (AFWCharacterBase* PlayerCharacter = Cast<AFWCharacterBase>(PlayerController->GetPawn()))
				{
					if (!PlayerCharacter->GetCurrentWeapon())
					{
						PlayerCharacter->EquipWeapon(SpawnedWeapon);
					}
				}
			}
		}
	}
}

void AFWCombatGarageTestDirector::RegisterCoreVehicleForPlayer(AFWVehicleBase* Vehicle) const
{
	if (!Vehicle)
	{
		return;
	}

	APlayerController* PlayerController = GetFirstPlayerController();
	AFWPlayerState* FWPlayerState = PlayerController ? PlayerController->GetPlayerState<AFWPlayerState>() : nullptr;
	if (!FWPlayerState || FWPlayerState->CoreVehicle)
	{
		return;
	}

	FWPlayerState->SetCoreVehicle(Vehicle);
}

void AFWCombatGarageTestDirector::StartMatchFlow() const
{
	if (AFWCompetitiveGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFWCompetitiveGameMode>() : nullptr)
	{
		GameMode->StartMatchFlow();
	}
}

void AFWCombatGarageTestDirector::BroadcastGarageReady() const
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
	Event.EventTag = FGameplayTag::RequestGameplayTag(FWCombatGarageEventTags::Ready);
	Event.InstigatorActor = const_cast<AFWCombatGarageTestDirector*>(this);
	Event.Target = const_cast<AFWCombatGarageTestDirector*>(this);
	Event.Magnitude = 1.0f;
	Event.WorldLocation = GetActorLocation();
	EventSubsystem->BroadcastGameplayEvent(Event);
}
