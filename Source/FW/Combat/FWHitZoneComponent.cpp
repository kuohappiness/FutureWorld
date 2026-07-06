#include "FWHitZoneComponent.h"

UFWHitZoneComponent::UFWHitZoneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UFWHitZoneComponent* UFWHitZoneComponent::FindHitZoneFromComponent(const UPrimitiveComponent* HitComponent)
{
	const USceneComponent* CurrentComponent = HitComponent;
	while (CurrentComponent)
	{
		if (UFWHitZoneComponent* HitZone = const_cast<UFWHitZoneComponent*>(Cast<UFWHitZoneComponent>(CurrentComponent)))
		{
			return HitZone;
		}

		CurrentComponent = CurrentComponent->GetAttachParent();
	}

	return nullptr;
}

UFWHitZoneComponent* UFWHitZoneComponent::FindBestHitZone(const FHitResult& HitResult)
{
	if (UFWHitZoneComponent* HitZone = FindHitZoneFromComponent(HitResult.GetComponent()))
	{
		return HitZone;
	}

	if (AActor* HitActor = HitResult.GetActor())
	{
		return HitActor->FindComponentByClass<UFWHitZoneComponent>();
	}

	return nullptr;
}
