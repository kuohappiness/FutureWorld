#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FWCombatGarageSpawnPoint.generated.h"

UENUM(BlueprintType)
enum class EFWCombatGarageSpawnType : uint8
{
	Player,
	AI,
	Car,
	Motorcycle,
	Weapon,
	Cover
};

UCLASS(Blueprintable)
class FW_API AFWCombatGarageSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AFWCombatGarageSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage")
	EFWCombatGarageSpawnType SpawnType = EFWCombatGarageSpawnType::AI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Combat Garage")
	FName SpawnId = NAME_None;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<USceneComponent> SceneRoot;
};
