#include "FWCombatGarageSpawnPoint.h"

AFWCombatGarageSpawnPoint::AFWCombatGarageSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
