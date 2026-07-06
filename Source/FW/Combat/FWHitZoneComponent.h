#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FWHitZoneComponent.generated.h"

UCLASS(ClassGroup = (FW), meta = (BlueprintSpawnableComponent))
class FW_API UFWHitZoneComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFWHitZoneComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Hit Zone")
	FName HitZoneName = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Hit Zone", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FW|Hit Zone")
	bool bWeakPoint = false;

	UFUNCTION(BlueprintPure, Category = "FW|Hit Zone")
	static UFWHitZoneComponent* FindHitZoneFromComponent(const UPrimitiveComponent* HitComponent);

	UFUNCTION(BlueprintPure, Category = "FW|Hit Zone")
	static UFWHitZoneComponent* FindBestHitZone(const FHitResult& HitResult);
};
