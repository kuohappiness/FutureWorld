#include "FWWeaponBase.h"

#include "../Combat/FWHealthComponent.h"
#include "../Combat/FWHitZoneComponent.h"
#include "../Events/FWEventSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "FWWeaponData.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"

namespace FWWeaponEventTags
{
	const FName WeaponFired(TEXT("Event.Weapon.Fired"));
	const FName DamageApplied(TEXT("Event.Damage.Applied"));
}

AFWWeaponBase::AFWWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AFWWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponData)
	{
		CurrentAmmo = WeaponData->MagazineSize;
	}
}

void AFWWeaponBase::Equip(APawn* NewOwningPawn)
{
	OwningPawn = NewOwningPawn;
	SetOwner(NewOwningPawn);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	AttachToOwningPawn();
}

void AFWWeaponBase::Unequip()
{
	StopFire();
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AFWWeaponBase::StartFire()
{
	bWantsToFire = true;

	if (FireOnce())
	{
		ScheduleNextShot();
	}
}

void AFWWeaponBase::StopFire()
{
	bWantsToFire = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AFWWeaponBase::Reload()
{
	if (!WeaponData || bIsReloading || CurrentAmmo >= WeaponData->MagazineSize)
	{
		return;
	}

	bIsReloading = true;
	StopFire();
	if (WeaponData->ReloadTime <= 0.0f)
	{
		FinishReload();
	}
	else
	{
		GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AFWWeaponBase::FinishReload, WeaponData->ReloadTime, false);
	}
}

bool AFWWeaponBase::CanFire() const
{
	return WeaponData && OwningPawn && CurrentAmmo > 0 && !bIsReloading;
}

bool AFWWeaponBase::FireAtActor(AActor* TargetActor)
{
	if (!TargetActor || !CanFire())
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float MinShotInterval = 1.0f / FMath::Max(WeaponData->FireRate, 0.01f);
	if ((Now - LastFireTime) < MinShotInterval)
	{
		return false;
	}

	--CurrentAmmo;
	LastFireTime = Now;

	FHitResult HitResult;
	AimTargetOverride = TargetActor;
	const bool bTraceHit = TraceWeapon(HitResult);
	AimTargetOverride.Reset();

	if (!bTraceHit || HitResult.GetActor() != TargetActor)
	{
		UPrimitiveComponent* TargetPrimitive = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent());
		const FVector HitLocation = TargetActor->GetActorLocation();
		const FVector HitNormal = (GetActorLocation() - HitLocation).GetSafeNormal();
		HitResult = FHitResult(TargetActor, TargetPrimitive, HitLocation, HitNormal);
		HitResult.bBlockingHit = true;
		HitResult.ImpactPoint = HitLocation;
	}

	const FFWDamageResult DamageResult = ApplyHitDamage(HitResult);

	BroadcastWeaponEvent(FWWeaponEventTags::WeaponFired, HitResult, WeaponData->BaseDamage);
	if (DamageResult.AppliedDamage > 0.0f)
	{
		BroadcastWeaponEvent(FWWeaponEventTags::DamageApplied, HitResult, DamageResult.AppliedDamage);
	}

	OnWeaponFired(HitResult, DamageResult);
	return true;
}

bool AFWWeaponBase::FireOnce()
{
	if (!CanFire())
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float MinShotInterval = 1.0f / FMath::Max(WeaponData->FireRate, 0.01f);
	if ((Now - LastFireTime) < MinShotInterval)
	{
		return false;
	}

	--CurrentAmmo;
	LastFireTime = Now;

	FHitResult HitResult;
	TraceWeapon(HitResult);
	const FFWDamageResult DamageResult = ApplyHitDamage(HitResult);

	BroadcastWeaponEvent(FWWeaponEventTags::WeaponFired, HitResult, WeaponData->BaseDamage);
	if (DamageResult.AppliedDamage > 0.0f)
	{
		BroadcastWeaponEvent(FWWeaponEventTags::DamageApplied, HitResult, DamageResult.AppliedDamage);
	}

	OnWeaponFired(HitResult, DamageResult);
	return true;
}

void AFWWeaponBase::OnWeaponFired_Implementation(const FHitResult& HitResult, const FFWDamageResult& DamageResult)
{
}

void AFWWeaponBase::ScheduleNextShot()
{
	if (!bWantsToFire || !WeaponData || WeaponData->FireRate <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AFWWeaponBase::StartFire, 1.0f / WeaponData->FireRate, false);
}

void AFWWeaponBase::FinishReload()
{
	bIsReloading = false;
	if (WeaponData)
	{
		CurrentAmmo = WeaponData->MagazineSize;
	}
}

bool AFWWeaponBase::TraceWeapon(FHitResult& OutHit) const
{
	if (!OwningPawn || !WeaponData)
	{
		return false;
	}

	FVector ViewLocation = OwningPawn->GetActorLocation();
	FRotator ViewRotation = OwningPawn->GetActorRotation();

	if (AActor* AimTarget = AimTargetOverride.Get())
	{
		ViewLocation = GetActorLocation();
		ViewRotation = (AimTarget->GetActorLocation() - ViewLocation).Rotation();
	}
	else if (const APlayerController* PlayerController = Cast<APlayerController>(OwningPawn->GetController()))
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else if (const AController* OwningController = OwningPawn->GetController())
	{
		ViewRotation = OwningController->GetControlRotation();
	}

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * WeaponData->Range);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FWWeaponTrace), true, this);
	QueryParams.AddIgnoredActor(OwningPawn);

	return GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
}

FFWDamageResult AFWWeaponBase::ApplyHitDamage(const FHitResult& HitResult) const
{
	FFWDamageResult DamageResult;
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || !WeaponData || !HasAuthority())
	{
		return DamageResult;
	}

	UFWHealthComponent* HealthComponent = HitActor->FindComponentByClass<UFWHealthComponent>();
	if (!HealthComponent)
	{
		return DamageResult;
	}

	FFWDamageRequest DamageRequest;
	DamageRequest.InstigatorActor = OwningPawn;
	DamageRequest.DamageCauser = const_cast<AFWWeaponBase*>(this);
	DamageRequest.BaseDamage = WeaponData->BaseDamage;
	DamageRequest.DamageMultiplier = 1.0f;
	DamageRequest.DamageTag = FGameplayTag::RequestGameplayTag(TEXT("Damage.Weapon.Hitscan"));
	DamageRequest.HitResult = HitResult;

	if (const UFWHitZoneComponent* HitZone = UFWHitZoneComponent::FindBestHitZone(HitResult))
	{
		DamageRequest.HitZoneName = HitZone->HitZoneName;
		DamageRequest.DamageMultiplier = HitZone->DamageMultiplier;
		if (HitZone->bWeakPoint)
		{
			DamageRequest.DamageMultiplier *= WeaponData->WeakPointMultiplier;
		}
	}

	return HealthComponent->ApplyDamageRequest(DamageRequest);
}

void AFWWeaponBase::BroadcastWeaponEvent(FName EventTagName, const FHitResult& HitResult, float Magnitude) const
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
	Event.EventTag = FGameplayTag::RequestGameplayTag(EventTagName);
	Event.InstigatorActor = OwningPawn;
	Event.Target = HitResult.GetActor();
	Event.Magnitude = Magnitude;
	Event.WorldLocation = HitResult.bBlockingHit ? FVector(HitResult.ImpactPoint) : GetActorLocation();
	EventSubsystem->BroadcastGameplayEvent(Event);
}

void AFWWeaponBase::AttachToOwningPawn()
{
	if (!OwningPawn)
	{
		return;
	}

	if (const ACharacter* CharacterOwner = Cast<ACharacter>(OwningPawn))
	{
		if (USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh())
		{
			if (Mesh->GetSkeletalMeshAsset() && Mesh->DoesSocketExist(WeaponAttachSocketName))
			{
				AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
				return;
			}
		}
	}

	AttachToActor(OwningPawn, FAttachmentTransformRules::KeepRelativeTransform);
}
