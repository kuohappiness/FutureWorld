#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "../World/FWCombatGarageSpawnPoint.h"
#include "FWMVPAssetBootstrapCommandlet.generated.h"

UCLASS()
class FW_API UFWMVPAssetBootstrapCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFWMVPAssetBootstrapCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	bool bHadError = false;

	void CreateWeaponDataAssets();
	void CreateVehicleDataAssets();
	void CreateInputAssets();
	void CreateTestMap();
	void CreateHUDWidgetBlueprint();
	void PopulateCombatGarageMap();
	void RebuildCombatGarageVisualFixtures(class UWorld* World);
	void ConfigureOnFootInputMappings();
	void ConfigureVehicleInputMappings();

	class AFWCombatGarageSpawnPoint* SpawnCombatGaragePoint(class UWorld* World, EFWCombatGarageSpawnType SpawnType, FName SpawnId, const FVector& Location, const FRotator& Rotation);
	class AStaticMeshActor* SpawnCombatGarageVisualBlock(class UWorld* World, class UStaticMesh* Mesh, class UMaterialInterface* Material, FName ActorName, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	bool HasActorOfClass(class UWorld* World, TSubclassOf<AActor> ActorClass) const;

	template <typename TAsset>
	TAsset* CreateOrLoadAsset(const FString& PackagePath, const FString& AssetName);

	bool SaveAsset(UObject* Asset);
	void LogCreatedOrUpdated(const FString& AssetPath) const;
};
