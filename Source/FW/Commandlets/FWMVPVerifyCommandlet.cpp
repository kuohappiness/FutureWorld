#include "FWMVPVerifyCommandlet.h"

#if WITH_EDITOR

#include "../Characters/FWAICharacter.h"
#include "../Characters/FWPlayerCharacter.h"
#include "../Core/FWPlayerController.h"
#include "../UI/FWHUDWidget.h"
#include "../Vehicles/FWMVPVehicleClasses.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Vehicles/FWVehicleData.h"
#include "../Weapons/FWMVPWeaponClasses.h"
#include "../Weapons/FWWeaponData.h"
#include "../World/FWCombatGarageSpawnPoint.h"
#include "../World/FWCombatGarageTestDirector.h"
#include "Components/BoxComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/UnrealType.h"

UFWMVPVerifyCommandlet::UFWMVPVerifyCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UFWMVPVerifyCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("FW MVP verification started."));

	VerifyDataAssets();
	VerifyInputMappings();
	VerifyCombatGarageMap();
	VerifyClassDefaults();

	if (IssueCount > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FW MVP verification failed with %d issue(s)."), IssueCount);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("FW MVP verification passed."));
	return 0;
}

void UFWMVPVerifyCommandlet::VerifyDataAssets()
{
	struct FWeaponExpectation
	{
		const TCHAR* Path;
		FName ContentId;
		EFWWeaponType Type;
	};

	const FWeaponExpectation WeaponExpectations[] =
	{
		{ TEXT("/Game/FW/Weapons/Data/DA_Weapon_Pistol"), TEXT("Weapon.Pistol.MVP"), EFWWeaponType::Pistol },
		{ TEXT("/Game/FW/Weapons/Data/DA_Weapon_Rifle"), TEXT("Weapon.Rifle.MVP"), EFWWeaponType::Rifle },
		{ TEXT("/Game/FW/Weapons/Data/DA_Weapon_Sniper"), TEXT("Weapon.Sniper.MVP"), EFWWeaponType::Sniper }
	};

	for (const FWeaponExpectation& Expectation : WeaponExpectations)
	{
		const UFWWeaponData* WeaponData = LoadObject<UFWWeaponData>(nullptr, Expectation.Path);
		Check(WeaponData != nullptr, FString::Printf(TEXT("Weapon DataAsset exists: %s"), Expectation.Path));
		if (WeaponData)
		{
			Check(WeaponData->ContentId == Expectation.ContentId, FString::Printf(TEXT("Weapon ContentId matches: %s"), *Expectation.ContentId.ToString()));
			Check(WeaponData->WeaponType == Expectation.Type, FString::Printf(TEXT("Weapon type matches: %s"), Expectation.Path));
			Check(WeaponData->MagazineSize > 0 && WeaponData->BaseDamage > 0.0f && WeaponData->Range > 0.0f, FString::Printf(TEXT("Weapon tuning is usable: %s"), Expectation.Path));
		}
	}

	struct FVehicleExpectation
	{
		const TCHAR* Path;
		FName ContentId;
		EFWVehicleType Type;
	};

	const FVehicleExpectation VehicleExpectations[] =
	{
		{ TEXT("/Game/FW/Vehicles/Data/DA_Vehicle_Car_Balanced"), TEXT("Vehicle.Car.Balanced"), EFWVehicleType::Car },
		{ TEXT("/Game/FW/Vehicles/Data/DA_Vehicle_Motorcycle_Fast"), TEXT("Vehicle.Motorcycle.Fast"), EFWVehicleType::Motorcycle }
	};

	for (const FVehicleExpectation& Expectation : VehicleExpectations)
	{
		const UFWVehicleData* VehicleData = LoadObject<UFWVehicleData>(nullptr, Expectation.Path);
		Check(VehicleData != nullptr, FString::Printf(TEXT("Vehicle DataAsset exists: %s"), Expectation.Path));
		if (VehicleData)
		{
			Check(VehicleData->ContentId == Expectation.ContentId, FString::Printf(TEXT("Vehicle ContentId matches: %s"), *Expectation.ContentId.ToString()));
			Check(VehicleData->VehicleType == Expectation.Type, FString::Printf(TEXT("Vehicle type matches: %s"), Expectation.Path));
			Check(VehicleData->MaxHealth > 0.0f && VehicleData->MaxSpeed > 0.0f && VehicleData->Acceleration > 0.0f, FString::Printf(TEXT("Vehicle tuning is usable: %s"), Expectation.Path));
		}
	}
}

void UFWMVPVerifyCommandlet::VerifyInputMappings()
{
	const TCHAR* OnFootContext = TEXT("/Game/FW/Input/MappingContexts/IMC_OnFoot");
	const TCHAR* VehicleContext = TEXT("/Game/FW/Input/MappingContexts/IMC_Vehicle");

	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::W), TEXT("On-foot Move maps W"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::A), TEXT("On-foot Move maps A"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::S), TEXT("On-foot Move maps S"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::D), TEXT("On-foot Move maps D"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_Fire"), EKeys::LeftMouseButton), TEXT("On-foot Fire maps LeftMouseButton"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_Interact"), EKeys::E), TEXT("On-foot Interact maps E"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_WeaponSlot1"), EKeys::One), TEXT("Weapon slot 1 maps 1"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_WeaponSlot2"), EKeys::Two), TEXT("Weapon slot 2 maps 2"));
	Check(HasInputMapping(OnFootContext, TEXT("/Game/FW/Input/Actions/IA_WeaponSlot3"), EKeys::Three), TEXT("Weapon slot 3 maps 3"));

	Check(HasInputMapping(VehicleContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::W), TEXT("Vehicle Move maps W"));
	Check(HasInputMapping(VehicleContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::A), TEXT("Vehicle Move maps A"));
	Check(HasInputMapping(VehicleContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::S), TEXT("Vehicle Move maps S"));
	Check(HasInputMapping(VehicleContext, TEXT("/Game/FW/Input/Actions/IA_Move"), EKeys::D), TEXT("Vehicle Move maps D"));
	Check(HasInputMapping(VehicleContext, TEXT("/Game/FW/Input/Actions/IA_Interact"), EKeys::E), TEXT("Vehicle Interact maps E"));
}

void UFWMVPVerifyCommandlet::VerifyCombatGarageMap()
{
	UWorld* World = LoadObject<UWorld>(nullptr, TEXT("/Game/FW/Maps/Test/L_Test_CombatGarage"));
	Check(World != nullptr, TEXT("L_Test_CombatGarage map loads"));
	if (!World)
	{
		return;
	}

	AFWCombatGarageTestDirector* Director = nullptr;
	int32 PlayerSpawns = 0;
	int32 AISpawns = 0;
	int32 CarSpawns = 0;
	int32 MotorcycleSpawns = 0;
	int32 WeaponSpawns = 0;
	int32 VisualFixtureActors = 0;
	const FName VisualFixtureTag(TEXT("FW_CombatGarageVisual"));

	if (!World->PersistentLevel)
	{
		Check(false, TEXT("Combat Garage persistent level exists"));
		return;
	}

	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (!Actor)
		{
			continue;
		}

		if (!Director)
		{
			Director = Cast<AFWCombatGarageTestDirector>(Actor);
		}
		if (Actor->Tags.Contains(VisualFixtureTag))
		{
			++VisualFixtureActors;
		}

		const AFWCombatGarageSpawnPoint* SpawnPoint = Cast<AFWCombatGarageSpawnPoint>(Actor);
		if (!SpawnPoint)
		{
			continue;
		}

		switch (SpawnPoint->SpawnType)
		{
		case EFWCombatGarageSpawnType::Player:
			++PlayerSpawns;
			break;
		case EFWCombatGarageSpawnType::AI:
			++AISpawns;
			break;
		case EFWCombatGarageSpawnType::Car:
			++CarSpawns;
			break;
		case EFWCombatGarageSpawnType::Motorcycle:
			++MotorcycleSpawns;
			break;
		case EFWCombatGarageSpawnType::Weapon:
			++WeaponSpawns;
			break;
		default:
			break;
		}
	}

	Check(Director != nullptr, TEXT("Combat Garage director exists"));
	Check(PlayerSpawns >= 1, TEXT("Combat Garage has player spawn"));
	Check(AISpawns >= 3, TEXT("Combat Garage has at least three AI spawns"));
	Check(CarSpawns >= 1, TEXT("Combat Garage has car spawn"));
	Check(MotorcycleSpawns >= 1, TEXT("Combat Garage has motorcycle spawn"));
	Check(WeaponSpawns >= 1, TEXT("Combat Garage has weapon spawn"));
	Check(VisualFixtureActors == 12, FString::Printf(TEXT("Combat Garage has deterministic visible framing fixtures (count: %d)"), VisualFixtureActors));

	if (Director)
	{
		if (const FClassProperty* AIClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("AICharacterClass")))
		{
			Check(AIClassProperty->GetPropertyValue_InContainer(Director) == AFWAICharacter::StaticClass(), TEXT("Director AI class is AFWAICharacter"));
		}
		if (const FClassProperty* CarClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("CarVehicleClass")))
		{
			UClass* ActualClass = Cast<UClass>(CarClassProperty->GetPropertyValue_InContainer(Director));
			Check(ActualClass == AFWCarVehicle::StaticClass(), FString::Printf(TEXT("Director car class is AFWCarVehicle (actual: %s)"), *GetNameSafe(ActualClass)));
		}
		if (const FClassProperty* MotorcycleClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("MotorcycleVehicleClass")))
		{
			UClass* ActualClass = Cast<UClass>(MotorcycleClassProperty->GetPropertyValue_InContainer(Director));
			Check(ActualClass == AFWMotorcycleVehicle::StaticClass(), FString::Printf(TEXT("Director motorcycle class is AFWMotorcycleVehicle (actual: %s)"), *GetNameSafe(ActualClass)));
		}
		if (const FClassProperty* TestWeaponClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("TestWeaponClass")))
		{
			Check(TestWeaponClassProperty->GetPropertyValue_InContainer(Director) == AFWPistolWeapon::StaticClass(), TEXT("Director test weapon class is AFWPistolWeapon"));
		}
		if (const FBoolProperty* SetupOnBeginPlayProperty = FindFProperty<FBoolProperty>(Director->GetClass(), TEXT("bSetupOnBeginPlay")))
		{
			Check(SetupOnBeginPlayProperty->GetPropertyValue_InContainer(Director), TEXT("Director setup on BeginPlay is enabled"));
		}
		if (const FBoolProperty* StartMatchAfterSetupProperty = FindFProperty<FBoolProperty>(Director->GetClass(), TEXT("bStartMatchAfterSetup")))
		{
			Check(StartMatchAfterSetupProperty->GetPropertyValue_InContainer(Director), TEXT("Director starts match after setup"));
		}
		if (const FFloatProperty* MinimumAIDistanceProperty = FindFProperty<FFloatProperty>(Director->GetClass(), TEXT("MinimumInitialAIDistanceFromPlayer")))
		{
			const float MinimumAIDistance = MinimumAIDistanceProperty->GetPropertyValue_InContainer(Director);
			Check(MinimumAIDistance >= 2800.0f,
				FString::Printf(TEXT("Director minimum initial AI distance is safe (actual: %.1f)"), MinimumAIDistance));
		}
		if (const FFloatProperty* BeginPlaySetupDelayProperty = FindFProperty<FFloatProperty>(Director->GetClass(), TEXT("BeginPlaySetupDelay")))
		{
			const float BeginPlaySetupDelay = BeginPlaySetupDelayProperty->GetPropertyValue_InContainer(Director);
			Check(FMath::IsNearlyZero(BeginPlaySetupDelay),
				FString::Printf(TEXT("Director setup runs immediately on BeginPlay (delay: %.2f)"), BeginPlaySetupDelay));
		}
	}
}

void UFWMVPVerifyCommandlet::VerifyClassDefaults()
{
	const AFWPlayerController* PlayerControllerCDO = AFWPlayerController::StaticClass()->GetDefaultObject<AFWPlayerController>();
	if (const FClassProperty* HUDClassProperty = FindFProperty<FClassProperty>(AFWPlayerController::StaticClass(), TEXT("HUDWidgetClass")))
	{
		Check(HUDClassProperty->GetPropertyValue_InContainer(PlayerControllerCDO) == UFWHUDWidget::StaticClass(), TEXT("PlayerController defaults to native HUD"));
	}

	Check(!UFWHUDWidget::StaticClass()->HasAnyClassFlags(CLASS_Abstract), TEXT("UFWHUDWidget is concrete"));

	const AFWPlayerCharacter* PlayerCDO = AFWPlayerCharacter::StaticClass()->GetDefaultObject<AFWPlayerCharacter>();
	Check(PlayerCDO != nullptr, TEXT("Player character CDO exists"));
	if (PlayerCDO)
	{
		if (const FClassProperty* PistolProperty = FindFProperty<FClassProperty>(AFWPlayerCharacter::StaticClass(), TEXT("PistolWeaponClass")))
		{
			Check(PistolProperty->GetPropertyValue_InContainer(PlayerCDO) == AFWPistolWeapon::StaticClass(), TEXT("Player pistol slot defaults to AFWPistolWeapon"));
		}
		if (const FClassProperty* RifleProperty = FindFProperty<FClassProperty>(AFWPlayerCharacter::StaticClass(), TEXT("RifleWeaponClass")))
		{
			Check(RifleProperty->GetPropertyValue_InContainer(PlayerCDO) == AFWRifleWeapon::StaticClass(), TEXT("Player rifle slot defaults to AFWRifleWeapon"));
		}
		if (const FClassProperty* SniperProperty = FindFProperty<FClassProperty>(AFWPlayerCharacter::StaticClass(), TEXT("SniperWeaponClass")))
		{
			Check(SniperProperty->GetPropertyValue_InContainer(PlayerCDO) == AFWSniperWeapon::StaticClass(), TEXT("Player sniper slot defaults to AFWSniperWeapon"));
		}
	}

	const AFWVehicleBase* CarCDO = AFWCarVehicle::StaticClass()->GetDefaultObject<AFWVehicleBase>();
	Check(CarCDO && CarCDO->FindComponentByClass<UBoxComponent>() != nullptr, TEXT("Car has collision component"));
	Check(CarCDO && CarCDO->FindComponentByClass<UFloatingPawnMovement>() != nullptr, TEXT("Car has movement component"));

	const AFWVehicleBase* MotorcycleCDO = AFWMotorcycleVehicle::StaticClass()->GetDefaultObject<AFWVehicleBase>();
	Check(MotorcycleCDO && MotorcycleCDO->FindComponentByClass<UBoxComponent>() != nullptr, TEXT("Motorcycle has collision component"));
	Check(MotorcycleCDO && MotorcycleCDO->FindComponentByClass<UFloatingPawnMovement>() != nullptr, TEXT("Motorcycle has movement component"));
}

void UFWMVPVerifyCommandlet::Check(bool bCondition, const FString& Message)
{
	if (bCondition)
	{
		UE_LOG(LogTemp, Display, TEXT("[OK] %s"), *Message);
		return;
	}

	++IssueCount;
	UE_LOG(LogTemp, Error, TEXT("[FAIL] %s"), *Message);
}

bool UFWMVPVerifyCommandlet::HasInputMapping(const TCHAR* MappingContextPath, const TCHAR* InputActionPath, FKey ExpectedKey) const
{
	const UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, MappingContextPath);
	const UInputAction* InputAction = LoadObject<UInputAction>(nullptr, InputActionPath);
	if (!MappingContext || !InputAction)
	{
		return false;
	}

	for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
	{
		if (Mapping.Action == InputAction && Mapping.Key == ExpectedKey)
		{
			return true;
		}
	}

	return false;
}

#else

UFWMVPVerifyCommandlet::UFWMVPVerifyCommandlet()
{
}

int32 UFWMVPVerifyCommandlet::Main(const FString& Params)
{
	return 1;
}

#endif
