#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "../Rules/FWMatchRuleSet.h"
#include "../Rules/FWMatchSettingsData.h"
#include "../State/FWStateMachineComponent.h"
#include "../Vehicles/FWVehicleData.h"
#include "../Weapons/FWWeaponData.h"
#include "../Weapons/FWWeaponLoadout.h"
#include "../World/FWCombatGarageSpawnPoint.h"
#include "../World/FWCombatGarageTestDirector.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFWMVPSmokeTest,
	"FW.MVP.Smoke.CoreDataAndState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFWMVPSmokeTest::RunTest(const FString& Parameters)
{
	UFWMatchSettingsData* Settings = NewObject<UFWMatchSettingsData>();
	TestNotNull(TEXT("Settings object can be created"), Settings);
	Settings->AIEnemyCount = 3;
	Settings->PlayerLives = 2;
	Settings->bRespawnEnabled = true;
	Settings->bCoreVehicleEnabled = true;
	Settings->CoreVehiclePenalty = 1;

	UFWMatchRuleSet* RuntimeRuleSet = Settings->CreateRuntimeRuleSet(GetTransientPackage());
	TestNotNull(TEXT("Runtime rule set can be created"), RuntimeRuleSet);
	TestEqual(TEXT("Rule set copies AI count"), RuntimeRuleSet->AIEnemyCount, 3);
	TestEqual(TEXT("Rule set copies player lives"), RuntimeRuleSet->PlayerLives, 2);
	TestTrue(TEXT("Runtime rule set uses stable ContentId"), RuntimeRuleSet->ContentId == TEXT("RuntimeMatchSettings"));

	UFWWeaponData* PistolData = NewObject<UFWWeaponData>();
	PistolData->ContentId = TEXT("Weapon.Pistol.Test");
	UFWWeaponData* RifleData = NewObject<UFWWeaponData>();
	RifleData->ContentId = TEXT("Weapon.Rifle.Test");
	UFWWeaponData* SniperData = NewObject<UFWWeaponData>();
	SniperData->ContentId = TEXT("Weapon.Sniper.Test");

	UFWWeaponLoadout* Loadout = NewObject<UFWWeaponLoadout>();
	Loadout->Pistol = PistolData;
	Loadout->Rifle = RifleData;
	Loadout->Sniper = SniperData;

	const TArray<FName> ContentIds = Loadout->GetSelectedContentIds();
	TestEqual(TEXT("Loadout contains three selected ContentIds"), ContentIds.Num(), 3);
	TestTrue(TEXT("Loadout includes pistol ContentId"), ContentIds.Contains(TEXT("Weapon.Pistol.Test")));
	TestTrue(TEXT("Loadout includes rifle ContentId"), ContentIds.Contains(TEXT("Weapon.Rifle.Test")));
	TestTrue(TEXT("Loadout includes sniper ContentId"), ContentIds.Contains(TEXT("Weapon.Sniper.Test")));

	UFWStateMachineComponent* StateMachine = NewObject<UFWStateMachineComponent>();
	const FGameplayTag FirstState = FGameplayTag::RequestGameplayTag(TEXT("State.Character.OnFoot"));
	const FGameplayTag SecondState = FGameplayTag::RequestGameplayTag(TEXT("State.Character.Dead"));
	TestTrue(TEXT("State machine accepts first state"), StateMachine->ChangeState(FirstState));
	TestFalse(TEXT("State machine rejects duplicate state"), StateMachine->ChangeState(FirstState));
	TestTrue(TEXT("State machine accepts second state"), StateMachine->ChangeState(SecondState));
	TestEqual(TEXT("State machine tracks previous state"), StateMachine->PreviousState, FirstState);
	TestEqual(TEXT("State machine tracks current state"), StateMachine->CurrentState, SecondState);

	UFWWeaponData* PistolAsset = LoadObject<UFWWeaponData>(nullptr, TEXT("/Game/FW/Weapons/Data/DA_Weapon_Pistol"));
	UFWWeaponData* RifleAsset = LoadObject<UFWWeaponData>(nullptr, TEXT("/Game/FW/Weapons/Data/DA_Weapon_Rifle"));
	UFWWeaponData* SniperAsset = LoadObject<UFWWeaponData>(nullptr, TEXT("/Game/FW/Weapons/Data/DA_Weapon_Sniper"));
	TestNotNull(TEXT("MVP pistol data asset loads"), PistolAsset);
	TestNotNull(TEXT("MVP rifle data asset loads"), RifleAsset);
	TestNotNull(TEXT("MVP sniper data asset loads"), SniperAsset);
	if (PistolAsset)
	{
		TestTrue(TEXT("MVP pistol asset uses stable ContentId"), PistolAsset->ContentId == TEXT("Weapon.Pistol.MVP"));
	}
	if (RifleAsset)
	{
		TestTrue(TEXT("MVP rifle asset uses stable ContentId"), RifleAsset->ContentId == TEXT("Weapon.Rifle.MVP"));
	}
	if (SniperAsset)
	{
		TestTrue(TEXT("MVP sniper asset uses stable ContentId"), SniperAsset->ContentId == TEXT("Weapon.Sniper.MVP"));
	}

	UFWVehicleData* CarAsset = LoadObject<UFWVehicleData>(nullptr, TEXT("/Game/FW/Vehicles/Data/DA_Vehicle_Car_Balanced"));
	UFWVehicleData* MotorcycleAsset = LoadObject<UFWVehicleData>(nullptr, TEXT("/Game/FW/Vehicles/Data/DA_Vehicle_Motorcycle_Fast"));
	TestNotNull(TEXT("MVP car data asset loads"), CarAsset);
	TestNotNull(TEXT("MVP motorcycle data asset loads"), MotorcycleAsset);
	if (CarAsset)
	{
		TestTrue(TEXT("MVP car asset uses stable ContentId"), CarAsset->ContentId == TEXT("Vehicle.Car.Balanced"));
	}
	if (MotorcycleAsset)
	{
		TestTrue(TEXT("MVP motorcycle asset uses stable ContentId"), MotorcycleAsset->ContentId == TEXT("Vehicle.Motorcycle.Fast"));
	}

	UWorld* CombatGarageMap = LoadObject<UWorld>(nullptr, TEXT("/Game/FW/Maps/Test/L_Test_CombatGarage"));
	TestNotNull(TEXT("MVP Combat Garage map loads"), CombatGarageMap);
	TestNotNull(TEXT("MVP Combat Garage map has a persistent level"), CombatGarageMap ? CombatGarageMap->PersistentLevel.Get() : nullptr);
	if (CombatGarageMap && CombatGarageMap->PersistentLevel)
	{
		int32 DirectorCount = 0;
		int32 PlayerSpawnCount = 0;
		int32 AISpawnCount = 0;
		int32 CarSpawnCount = 0;
		int32 MotorcycleSpawnCount = 0;
		int32 WeaponSpawnCount = 0;

		for (AActor* Actor : CombatGarageMap->PersistentLevel->Actors)
		{
			if (Actor && Actor->IsA<AFWCombatGarageTestDirector>())
			{
				++DirectorCount;
			}

			const AFWCombatGarageSpawnPoint* SpawnPoint = Cast<AFWCombatGarageSpawnPoint>(Actor);
			if (!SpawnPoint)
			{
				continue;
			}

			switch (SpawnPoint->SpawnType)
			{
			case EFWCombatGarageSpawnType::Player:
				++PlayerSpawnCount;
				break;
			case EFWCombatGarageSpawnType::AI:
				++AISpawnCount;
				break;
			case EFWCombatGarageSpawnType::Car:
				++CarSpawnCount;
				break;
			case EFWCombatGarageSpawnType::Motorcycle:
				++MotorcycleSpawnCount;
				break;
			case EFWCombatGarageSpawnType::Weapon:
				++WeaponSpawnCount;
				break;
			default:
				break;
			}
		}

		TestEqual(TEXT("Combat Garage has one test director"), DirectorCount, 1);
		TestEqual(TEXT("Combat Garage has one player spawn"), PlayerSpawnCount, 1);
		TestEqual(TEXT("Combat Garage has three AI spawns"), AISpawnCount, 3);
		TestEqual(TEXT("Combat Garage has one car spawn"), CarSpawnCount, 1);
		TestEqual(TEXT("Combat Garage has one motorcycle spawn"), MotorcycleSpawnCount, 1);
		TestEqual(TEXT("Combat Garage has one weapon spawn"), WeaponSpawnCount, 1);
	}

	return true;
}

#endif
