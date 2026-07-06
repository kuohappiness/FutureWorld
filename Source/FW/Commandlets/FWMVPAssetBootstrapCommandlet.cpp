#include "FWMVPAssetBootstrapCommandlet.h"

#if WITH_EDITOR

#include "../Vehicles/FWVehicleData.h"
#include "../Weapons/FWWeaponData.h"
#include "../UI/FWHUDWidget.h"
#include "../World/FWCombatGarageSpawnPoint.h"
#include "../World/FWCombatGarageTestDirector.h"
#include "../Characters/FWAICharacter.h"
#include "../Vehicles/FWVehicleBase.h"
#include "../Vehicles/FWMVPVehicleClasses.h"
#include "../Weapons/FWMVPWeaponClasses.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Factories/WorldFactory.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

UFWMVPAssetBootstrapCommandlet::UFWMVPAssetBootstrapCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UFWMVPAssetBootstrapCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Display, TEXT("FW MVP asset bootstrap started."));

	CreateWeaponDataAssets();
	CreateVehicleDataAssets();
	CreateInputAssets();
	ConfigureOnFootInputMappings();
	ConfigureVehicleInputMappings();
	CreateTestMap();
	PopulateCombatGarageMap();
	CreateHUDWidgetBlueprint();

	UE_LOG(LogTemp, Display, TEXT("FW MVP asset bootstrap finished."));
	return bHadError ? 1 : 0;
}

void UFWMVPAssetBootstrapCommandlet::CreateWeaponDataAssets()
{
	UFWWeaponData* Pistol = CreateOrLoadAsset<UFWWeaponData>(TEXT("/Game/FW/Weapons/Data"), TEXT("DA_Weapon_Pistol"));
	if (Pistol)
	{
		Pistol->ContentId = TEXT("Weapon.Pistol.MVP");
		Pistol->DisplayName = FText::FromString(TEXT("Pistol"));
		Pistol->WeaponType = EFWWeaponType::Pistol;
		Pistol->BaseDamage = 25.0f;
		Pistol->FireRate = 3.0f;
		Pistol->MagazineSize = 12;
		Pistol->ReloadTime = 1.4f;
		Pistol->Range = 8000.0f;
		Pistol->WeakPointMultiplier = 1.5f;
		SaveAsset(Pistol);
	}

	UFWWeaponData* Rifle = CreateOrLoadAsset<UFWWeaponData>(TEXT("/Game/FW/Weapons/Data"), TEXT("DA_Weapon_Rifle"));
	if (Rifle)
	{
		Rifle->ContentId = TEXT("Weapon.Rifle.MVP");
		Rifle->DisplayName = FText::FromString(TEXT("Rifle"));
		Rifle->WeaponType = EFWWeaponType::Rifle;
		Rifle->BaseDamage = 18.0f;
		Rifle->FireRate = 8.0f;
		Rifle->MagazineSize = 30;
		Rifle->ReloadTime = 2.1f;
		Rifle->Range = 12000.0f;
		Rifle->WeakPointMultiplier = 1.4f;
		SaveAsset(Rifle);
	}

	UFWWeaponData* Sniper = CreateOrLoadAsset<UFWWeaponData>(TEXT("/Game/FW/Weapons/Data"), TEXT("DA_Weapon_Sniper"));
	if (Sniper)
	{
		Sniper->ContentId = TEXT("Weapon.Sniper.MVP");
		Sniper->DisplayName = FText::FromString(TEXT("Sniper"));
		Sniper->WeaponType = EFWWeaponType::Sniper;
		Sniper->BaseDamage = 90.0f;
		Sniper->FireRate = 0.8f;
		Sniper->MagazineSize = 5;
		Sniper->ReloadTime = 2.8f;
		Sniper->Range = 25000.0f;
		Sniper->WeakPointMultiplier = 2.5f;
		SaveAsset(Sniper);
	}
}

void UFWMVPAssetBootstrapCommandlet::CreateVehicleDataAssets()
{
	UFWVehicleData* Car = CreateOrLoadAsset<UFWVehicleData>(TEXT("/Game/FW/Vehicles/Data"), TEXT("DA_Vehicle_Car_Balanced"));
	if (Car)
	{
		Car->ContentId = TEXT("Vehicle.Car.Balanced");
		Car->DisplayName = FText::FromString(TEXT("Balanced Car"));
		Car->VehicleType = EFWVehicleType::Car;
		Car->MaxHealth = 650.0f;
		Car->Armor = 25.0f;
		Car->MaxSpeed = 1800.0f;
		Car->Acceleration = 750.0f;
		Car->Handling = 0.8f;
		Car->SeatCount = 2;
		SaveAsset(Car);
	}

	UFWVehicleData* Motorcycle = CreateOrLoadAsset<UFWVehicleData>(TEXT("/Game/FW/Vehicles/Data"), TEXT("DA_Vehicle_Motorcycle_Fast"));
	if (Motorcycle)
	{
		Motorcycle->ContentId = TEXT("Vehicle.Motorcycle.Fast");
		Motorcycle->DisplayName = FText::FromString(TEXT("Fast Motorcycle"));
		Motorcycle->VehicleType = EFWVehicleType::Motorcycle;
		Motorcycle->MaxHealth = 300.0f;
		Motorcycle->Armor = 5.0f;
		Motorcycle->MaxSpeed = 2400.0f;
		Motorcycle->Acceleration = 1100.0f;
		Motorcycle->Handling = 1.25f;
		Motorcycle->SeatCount = 1;
		SaveAsset(Motorcycle);
	}
}

void UFWMVPAssetBootstrapCommandlet::CreateInputAssets()
{
	struct FInputActionSpec
	{
		const TCHAR* Name;
		EInputActionValueType ValueType;
	};

	const FInputActionSpec ActionSpecs[] =
	{
		{ TEXT("IA_Move"), EInputActionValueType::Axis2D },
		{ TEXT("IA_Look"), EInputActionValueType::Axis2D },
		{ TEXT("IA_Jump"), EInputActionValueType::Boolean },
		{ TEXT("IA_Run"), EInputActionValueType::Boolean },
		{ TEXT("IA_Crouch"), EInputActionValueType::Boolean },
		{ TEXT("IA_Crawl"), EInputActionValueType::Boolean },
		{ TEXT("IA_Fire"), EInputActionValueType::Boolean },
		{ TEXT("IA_Aim"), EInputActionValueType::Boolean },
		{ TEXT("IA_Reload"), EInputActionValueType::Boolean },
		{ TEXT("IA_Interact"), EInputActionValueType::Boolean },
		{ TEXT("IA_ToggleCamera"), EInputActionValueType::Boolean },
		{ TEXT("IA_WeaponSlot1"), EInputActionValueType::Boolean },
		{ TEXT("IA_WeaponSlot2"), EInputActionValueType::Boolean },
		{ TEXT("IA_WeaponSlot3"), EInputActionValueType::Boolean }
	};

	for (const FInputActionSpec& Spec : ActionSpecs)
	{
		UInputAction* Action = CreateOrLoadAsset<UInputAction>(TEXT("/Game/FW/Input/Actions"), Spec.Name);
		if (Action)
		{
			Action->ValueType = Spec.ValueType;
			SaveAsset(Action);
		}
	}

	const TCHAR* MappingContextNames[] =
	{
		TEXT("IMC_OnFoot"),
		TEXT("IMC_Vehicle"),
		TEXT("IMC_Menu")
	};

	for (const TCHAR* MappingContextName : MappingContextNames)
	{
		if (UInputMappingContext* MappingContext = CreateOrLoadAsset<UInputMappingContext>(TEXT("/Game/FW/Input/MappingContexts"), MappingContextName))
		{
			SaveAsset(MappingContext);
		}
	}
}

void UFWMVPAssetBootstrapCommandlet::ConfigureOnFootInputMappings()
{
	UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/FW/Input/MappingContexts/IMC_OnFoot"));
	if (!MappingContext)
	{
		UE_LOG(LogTemp, Error, TEXT("Missing on-foot mapping context."));
		bHadError = true;
		return;
	}

	auto LoadAction = [this](const TCHAR* AssetName) -> UInputAction*
	{
		const FString AssetPath = FString::Printf(TEXT("/Game/FW/Input/Actions/%s"), AssetName);
		UInputAction* Action = LoadObject<UInputAction>(nullptr, *AssetPath);
		if (!Action)
		{
			UE_LOG(LogTemp, Error, TEXT("Missing input action: %s"), *AssetPath);
			bHadError = true;
		}
		return Action;
	};

	UInputAction* MoveAction = LoadAction(TEXT("IA_Move"));
	UInputAction* LookAction = LoadAction(TEXT("IA_Look"));
	UInputAction* JumpAction = LoadAction(TEXT("IA_Jump"));
	UInputAction* RunAction = LoadAction(TEXT("IA_Run"));
	UInputAction* CrouchAction = LoadAction(TEXT("IA_Crouch"));
	UInputAction* CrawlAction = LoadAction(TEXT("IA_Crawl"));
	UInputAction* FireAction = LoadAction(TEXT("IA_Fire"));
	UInputAction* ReloadAction = LoadAction(TEXT("IA_Reload"));
	UInputAction* InteractAction = LoadAction(TEXT("IA_Interact"));
	UInputAction* ToggleCameraAction = LoadAction(TEXT("IA_ToggleCamera"));
	UInputAction* WeaponSlot1Action = LoadAction(TEXT("IA_WeaponSlot1"));
	UInputAction* WeaponSlot2Action = LoadAction(TEXT("IA_WeaponSlot2"));
	UInputAction* WeaponSlot3Action = LoadAction(TEXT("IA_WeaponSlot3"));

	if (bHadError)
	{
		return;
	}

	MappingContext->UnmapAll();

	auto AddNegateModifier = [MappingContext](FEnhancedActionKeyMapping& Mapping)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
	};

	auto AddSwizzleModifier = [MappingContext](FEnhancedActionKeyMapping& Mapping)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(MappingContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	};

	MappingContext->MapKey(MoveAction, EKeys::D);
	FEnhancedActionKeyMapping& MoveLeft = MappingContext->MapKey(MoveAction, EKeys::A);
	AddNegateModifier(MoveLeft);
	FEnhancedActionKeyMapping& MoveForward = MappingContext->MapKey(MoveAction, EKeys::W);
	AddSwizzleModifier(MoveForward);
	FEnhancedActionKeyMapping& MoveBackward = MappingContext->MapKey(MoveAction, EKeys::S);
	AddNegateModifier(MoveBackward);
	AddSwizzleModifier(MoveBackward);

	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(RunAction, EKeys::LeftShift);
	MappingContext->MapKey(CrouchAction, EKeys::LeftControl);
	MappingContext->MapKey(CrawlAction, EKeys::C);
	MappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(ReloadAction, EKeys::R);
	MappingContext->MapKey(InteractAction, EKeys::E);
	MappingContext->MapKey(ToggleCameraAction, EKeys::V);
	MappingContext->MapKey(WeaponSlot1Action, EKeys::One);
	MappingContext->MapKey(WeaponSlot2Action, EKeys::Two);
	MappingContext->MapKey(WeaponSlot3Action, EKeys::Three);

	MappingContext->MarkPackageDirty();
	SaveAsset(MappingContext);
}

void UFWMVPAssetBootstrapCommandlet::ConfigureVehicleInputMappings()
{
	UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/FW/Input/MappingContexts/IMC_Vehicle"));
	if (!MappingContext)
	{
		UE_LOG(LogTemp, Error, TEXT("Missing vehicle mapping context."));
		bHadError = true;
		return;
	}

	auto LoadAction = [this](const TCHAR* AssetName) -> UInputAction*
	{
		const FString AssetPath = FString::Printf(TEXT("/Game/FW/Input/Actions/%s"), AssetName);
		UInputAction* Action = LoadObject<UInputAction>(nullptr, *AssetPath);
		if (!Action)
		{
			UE_LOG(LogTemp, Error, TEXT("Missing input action: %s"), *AssetPath);
			bHadError = true;
		}
		return Action;
	};

	UInputAction* MoveAction = LoadAction(TEXT("IA_Move"));
	UInputAction* LookAction = LoadAction(TEXT("IA_Look"));
	UInputAction* InteractAction = LoadAction(TEXT("IA_Interact"));

	if (bHadError)
	{
		return;
	}

	MappingContext->UnmapAll();

	auto AddNegateModifier = [MappingContext](FEnhancedActionKeyMapping& Mapping)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
	};

	auto AddSwizzleModifier = [MappingContext](FEnhancedActionKeyMapping& Mapping)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(MappingContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	};

	MappingContext->MapKey(MoveAction, EKeys::D);
	FEnhancedActionKeyMapping& SteerLeft = MappingContext->MapKey(MoveAction, EKeys::A);
	AddNegateModifier(SteerLeft);
	FEnhancedActionKeyMapping& ThrottleForward = MappingContext->MapKey(MoveAction, EKeys::W);
	AddSwizzleModifier(ThrottleForward);
	FEnhancedActionKeyMapping& ThrottleReverse = MappingContext->MapKey(MoveAction, EKeys::S);
	AddNegateModifier(ThrottleReverse);
	AddSwizzleModifier(ThrottleReverse);

	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(InteractAction, EKeys::E);

	MappingContext->MarkPackageDirty();
	SaveAsset(MappingContext);
}

void UFWMVPAssetBootstrapCommandlet::CreateTestMap()
{
	const FString PackagePath = TEXT("/Game/FW/Maps/Test");
	const FString AssetName = TEXT("L_Test_CombatGarage");
	const FString PackageName = PackagePath / AssetName;

	if (LoadObject<UWorld>(nullptr, *PackageName))
	{
		UE_LOG(LogTemp, Display, TEXT("Asset already exists: %s"), *PackageName);
		return;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	UWorldFactory* WorldFactory = NewObject<UWorldFactory>();
	UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UWorld::StaticClass(), WorldFactory);
	if (!CreatedAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create test map asset: %s"), *PackageName);
		bHadError = true;
		return;
	}

	SaveAsset(CreatedAsset);
}

void UFWMVPAssetBootstrapCommandlet::CreateHUDWidgetBlueprint()
{
	const FString PackagePath = TEXT("/Game/FW/UI/Widgets");
	const FString AssetName = TEXT("WBP_HUD");
	const FString PackageName = PackagePath / AssetName;

	if (LoadObject<UWidgetBlueprint>(nullptr, *PackageName))
	{
		UE_LOG(LogTemp, Display, TEXT("Asset already exists: %s"), *PackageName);
		return;
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageName);
		bHadError = true;
		return;
	}

	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		UFWHUDWidget::StaticClass(),
		Package,
		*AssetName,
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass(),
		NAME_None);

	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create HUD widget blueprint: %s"), *PackageName);
		bHadError = true;
		return;
	}

	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();
	SaveAsset(Blueprint);
}

void UFWMVPAssetBootstrapCommandlet::PopulateCombatGarageMap()
{
	const FString PackageName = TEXT("/Game/FW/Maps/Test/L_Test_CombatGarage");
	UWorld* World = LoadObject<UWorld>(nullptr, *PackageName);
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat Garage map was not available for population: %s"), *PackageName);
		return;
	}

	AFWCombatGarageTestDirector* Director = nullptr;
	TArray<AFWCombatGarageTestDirector*> ExtraDirectors;
	TArray<AFWCombatGarageSpawnPoint*> ExistingSpawnPoints;
	if (World->PersistentLevel)
	{
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (AFWCombatGarageTestDirector* CandidateDirector = Cast<AFWCombatGarageTestDirector>(Actor))
			{
				if (!Director)
				{
					Director = CandidateDirector;
				}
				else
				{
					ExtraDirectors.Add(CandidateDirector);
				}
			}
			else if (AFWCombatGarageSpawnPoint* SpawnPoint = Cast<AFWCombatGarageSpawnPoint>(Actor))
			{
				ExistingSpawnPoints.Add(SpawnPoint);
			}
		}
	}

	if (Director)
	{
		const FClassProperty* CarClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("CarVehicleClass"));
		const FClassProperty* MotorcycleClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("MotorcycleVehicleClass"));
		const UClass* SavedCarClass = CarClassProperty ? Cast<UClass>(CarClassProperty->GetPropertyValue_InContainer(Director)) : nullptr;
		const UClass* SavedMotorcycleClass = MotorcycleClassProperty ? Cast<UClass>(MotorcycleClassProperty->GetPropertyValue_InContainer(Director)) : nullptr;
		if (SavedCarClass != AFWCarVehicle::StaticClass() || SavedMotorcycleClass != AFWMotorcycleVehicle::StaticClass())
		{
			UE_LOG(LogTemp, Display, TEXT("Replacing stale Combat Garage director vehicle classes: car=%s motorcycle=%s"),
				*GetNameSafe(SavedCarClass),
				*GetNameSafe(SavedMotorcycleClass));
			World->EditorDestroyActor(Director, true);
			Director = nullptr;
		}
	}

	for (AFWCombatGarageTestDirector* ExtraDirector : ExtraDirectors)
	{
		if (ExtraDirector)
		{
			World->EditorDestroyActor(ExtraDirector, true);
		}
	}

	if (!Director)
	{
		Director = World->SpawnActor<AFWCombatGarageTestDirector>(
			AFWCombatGarageTestDirector::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator);
	}

	if (Director)
	{
		Director->Modify();
		Director->ConfigureMVPDefaults();
		FClassProperty* CarClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("CarVehicleClass"));
		FClassProperty* MotorcycleClassProperty = FindFProperty<FClassProperty>(Director->GetClass(), TEXT("MotorcycleVehicleClass"));
		if (CarClassProperty)
		{
			CarClassProperty->SetPropertyValue_InContainer(Director, AFWCarVehicle::StaticClass());
		}
		if (MotorcycleClassProperty)
		{
			MotorcycleClassProperty->SetPropertyValue_InContainer(Director, AFWMotorcycleVehicle::StaticClass());
		}
		Director->PostEditChange();
		Director->MarkPackageDirty();
		UE_LOG(LogTemp, Display, TEXT("Combat Garage director vehicle classes: car=%s motorcycle=%s"),
			*GetNameSafe(CarClassProperty ? CarClassProperty->GetPropertyValue_InContainer(Director) : nullptr),
			*GetNameSafe(MotorcycleClassProperty ? MotorcycleClassProperty->GetPropertyValue_InContainer(Director) : nullptr));
	}

	for (AFWCombatGarageSpawnPoint* ExistingSpawnPoint : ExistingSpawnPoints)
	{
		if (ExistingSpawnPoint)
		{
			World->EditorDestroyActor(ExistingSpawnPoint, true);
		}
	}

	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::Player, TEXT("Player_01"), FVector(0.0f, 0.0f, 100.0f), FRotator(-10.0f, 0.0f, 0.0f));
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::AI, TEXT("AI_01"), FVector(3200.0f, -500.0f, 100.0f), FRotator(0.0f, 180.0f, 0.0f));
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::AI, TEXT("AI_02"), FVector(3600.0f, 0.0f, 100.0f), FRotator(0.0f, 180.0f, 0.0f));
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::AI, TEXT("AI_03"), FVector(3200.0f, 500.0f, 100.0f), FRotator(0.0f, 180.0f, 0.0f));
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::Car, TEXT("Car_01"), FVector(300.0f, -500.0f, 100.0f), FRotator::ZeroRotator);
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::Motorcycle, TEXT("Motorcycle_01"), FVector(300.0f, 500.0f, 100.0f), FRotator::ZeroRotator);
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::Weapon, TEXT("Weapon_01"), FVector(150.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::Cover, TEXT("Cover_01"), FVector(600.0f, -200.0f, 100.0f), FRotator::ZeroRotator);
	SpawnCombatGaragePoint(World, EFWCombatGarageSpawnType::Cover, TEXT("Cover_02"), FVector(600.0f, 200.0f, 100.0f), FRotator::ZeroRotator);

	RebuildCombatGarageVisualFixtures(World);

	World->MarkPackageDirty();
	World->PersistentLevel->MarkPackageDirty();
	SaveAsset(World);
}

void UFWMVPAssetBootstrapCommandlet::RebuildCombatGarageVisualFixtures(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const FName VisualTag(TEXT("FW_CombatGarageVisual"));
	TArray<AActor*> VisualActorsToDestroy;
	if (World->PersistentLevel)
	{
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (Actor && Actor->Tags.Contains(VisualTag))
			{
				VisualActorsToDestroy.Add(Actor);
			}
		}
	}

	for (AActor* Actor : VisualActorsToDestroy)
	{
		if (Actor)
		{
			Actor->Modify();
			World->DestroyActor(Actor, true, true);
		}
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not load /Engine/BasicShapes/Cube for Combat Garage visual fixtures."));
		return;
	}
	UMaterialInterface* GridMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultWhiteGrid.DefaultWhiteGrid"));

	auto TagActor = [VisualTag](AActor* Actor)
	{
		if (Actor)
		{
			Actor->Tags.AddUnique(VisualTag);
			Actor->MarkPackageDirty();
		}
	};

	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_NearFloor"), FVector(450.0f, 0.0f, -10.0f), FRotator::ZeroRotator, FVector(18.0f, 12.0f, 0.2f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_Floor"), FVector(1700.0f, 0.0f, -12.0f), FRotator::ZeroRotator, FVector(42.0f, 18.0f, 0.16f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_CeilingGuide"), FVector(450.0f, 0.0f, 420.0f), FRotator::ZeroRotator, FVector(10.0f, 10.0f, 0.16f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_FramingWall"), FVector(700.0f, 0.0f, 180.0f), FRotator::ZeroRotator, FVector(0.25f, 8.0f, 3.6f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_RearMarker"), FVector(-450.0f, 0.0f, 150.0f), FRotator::ZeroRotator, FVector(0.4f, 4.0f, 3.0f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_Backstop"), FVector(3900.0f, 0.0f, 180.0f), FRotator::ZeroRotator, FVector(0.35f, 18.0f, 3.6f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_LeftMarker"), FVector(500.0f, -450.0f, 140.0f), FRotator::ZeroRotator, FVector(0.45f, 0.45f, 2.8f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_RightMarker"), FVector(500.0f, 450.0f, 140.0f), FRotator::ZeroRotator, FVector(0.45f, 0.45f, 2.8f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_CoverBlock_01"), FVector(850.0f, -260.0f, 75.0f), FRotator::ZeroRotator, FVector(1.8f, 0.5f, 1.5f)));
	TagActor(SpawnCombatGarageVisualBlock(World, CubeMesh, GridMaterial, TEXT("FW_CombatGarage_CoverBlock_02"), FVector(850.0f, 260.0f, 75.0f), FRotator::ZeroRotator, FVector(1.8f, 0.5f, 1.5f)));

	ADirectionalLight* DirectionalLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(-500.0f, -500.0f, 900.0f), FRotator(-45.0f, -35.0f, 0.0f));
	if (DirectionalLight)
	{
		DirectionalLight->SetActorLabel(TEXT("FW_CombatGarage_KeyLight"));
		if (ULightComponent* LightComponent = DirectionalLight->GetLightComponent())
		{
			LightComponent->SetMobility(EComponentMobility::Movable);
			LightComponent->SetIntensity(8.0f);
		}
		TagActor(DirectionalLight);
	}

	APointLight* FillLight = World->SpawnActor<APointLight>(APointLight::StaticClass(), FVector(600.0f, 0.0f, 700.0f), FRotator::ZeroRotator);
	if (FillLight)
	{
		FillLight->SetActorLabel(TEXT("FW_CombatGarage_FillLight"));
		if (ULightComponent* LightComponent = FillLight->GetLightComponent())
		{
			LightComponent->SetMobility(EComponentMobility::Movable);
			LightComponent->SetIntensity(150000.0f);
			if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(LightComponent))
			{
				PointLightComponent->SetAttenuationRadius(3500.0f);
			}
		}
		TagActor(FillLight);
	}
}

AFWCombatGarageSpawnPoint* UFWMVPAssetBootstrapCommandlet::SpawnCombatGaragePoint(UWorld* World, EFWCombatGarageSpawnType SpawnType, FName SpawnId, const FVector& Location, const FRotator& Rotation)
{
	if (!World)
	{
		return nullptr;
	}

	AFWCombatGarageSpawnPoint* SpawnPoint = World->SpawnActor<AFWCombatGarageSpawnPoint>(AFWCombatGarageSpawnPoint::StaticClass(), Location, Rotation);
	if (SpawnPoint)
	{
		SpawnPoint->SpawnType = SpawnType;
		SpawnPoint->SpawnId = SpawnId;
	}
	return SpawnPoint;
}

AStaticMeshActor* UFWMVPAssetBootstrapCommandlet::SpawnCombatGarageVisualBlock(UWorld* World, UStaticMesh* Mesh, UMaterialInterface* Material, FName ActorName, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	if (!World || !Mesh)
	{
		return nullptr;
	}

	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation);
	if (!Actor)
	{
		return nullptr;
	}

	Actor->SetActorLabel(ActorName.ToString());
	Actor->SetActorScale3D(Scale);
	if (UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent())
	{
		MeshComponent->SetStaticMesh(Mesh);
		if (Material)
		{
			MeshComponent->SetMaterial(0, Material);
		}
		MeshComponent->SetMobility(EComponentMobility::Movable);
	}
	return Actor;
}

bool UFWMVPAssetBootstrapCommandlet::HasActorOfClass(UWorld* World, TSubclassOf<AActor> ActorClass) const
{
	if (!World || !ActorClass)
	{
		return false;
	}

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		if (*It)
		{
			return true;
		}
	}

	if (World->PersistentLevel)
	{
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (Actor && Actor->IsA(ActorClass))
			{
				return true;
			}
		}
	}

	return false;
}

template <typename TAsset>
TAsset* UFWMVPAssetBootstrapCommandlet::CreateOrLoadAsset(const FString& PackagePath, const FString& AssetName)
{
	const FString PackageName = PackagePath / AssetName;
	if (TAsset* ExistingAsset = LoadObject<TAsset>(nullptr, *PackageName))
	{
		return ExistingAsset;
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageName);
		bHadError = true;
		return nullptr;
	}

	TAsset* Asset = NewObject<TAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (!Asset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create asset: %s"), *PackageName);
		bHadError = true;
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(Asset);
	Package->MarkPackageDirty();
	LogCreatedOrUpdated(PackageName);
	return Asset;
}

bool UFWMVPAssetBootstrapCommandlet::SaveAsset(UObject* Asset)
{
	if (!Asset)
	{
		return false;
	}

	UPackage* Package = Asset->GetOutermost();
	const FString PackageExtension = Asset->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), PackageExtension);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	const bool bSaved = UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save asset: %s"), *Package->GetName());
		bHadError = true;
		return false;
	}

	return true;
}

void UFWMVPAssetBootstrapCommandlet::LogCreatedOrUpdated(const FString& AssetPath) const
{
	UE_LOG(LogTemp, Display, TEXT("Created or loaded asset: %s"), *AssetPath);
}

#else

UFWMVPAssetBootstrapCommandlet::UFWMVPAssetBootstrapCommandlet()
{
}

int32 UFWMVPAssetBootstrapCommandlet::Main(const FString& Params)
{
	return 1;
}

#endif
