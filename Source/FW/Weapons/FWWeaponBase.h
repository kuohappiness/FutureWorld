#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Combat/FWDamageTypes.h"
#include "FWWeaponBase.generated.h"

class UFWWeaponData;
class UFWHealthComponent;

UCLASS(Abstract, Blueprintable)
class FW_API AFWWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFWWeaponBase();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void Equip(APawn* NewOwningPawn);

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void Unequip();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	bool FireAtActor(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "FW|Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "FW|Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "FW|Weapon")
	UFWWeaponData* GetWeaponData() const { return WeaponData; }

	UFUNCTION(BlueprintPure, Category = "FW|Weapon")
	APawn* GetOwningPawn() const { return OwningPawn; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon")
	TObjectPtr<UFWWeaponData> WeaponData;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Weapon")
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Weapon")
	int32 CurrentAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Weapon")
	bool bWantsToFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "FW|Weapon")
	bool bIsReloading = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FW|Weapon")
	FName WeaponAttachSocketName = TEXT("WeaponSocket");

	UFUNCTION(BlueprintCallable, Category = "FW|Weapon")
	bool FireOnce();

	UFUNCTION(BlueprintNativeEvent, Category = "FW|Weapon")
	void OnWeaponFired(const FHitResult& HitResult, const FFWDamageResult& DamageResult);

private:
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	TWeakObjectPtr<AActor> AimTargetOverride;
	float LastFireTime = -FLT_MAX;

	void ScheduleNextShot();
	void FinishReload();
	bool TraceWeapon(FHitResult& OutHit) const;
	FFWDamageResult ApplyHitDamage(const FHitResult& HitResult) const;
	void BroadcastWeaponEvent(FName EventTagName, const FHitResult& HitResult, float Magnitude) const;
	void AttachToOwningPawn();
};
