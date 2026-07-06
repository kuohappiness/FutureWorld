#include "FWWeaponLoadout.h"

#include "FWWeaponData.h"

TArray<FName> UFWWeaponLoadout::GetSelectedContentIds() const
{
	TArray<FName> ContentIds;
	if (Pistol)
	{
		ContentIds.Add(Pistol->ContentId);
	}
	if (Rifle)
	{
		ContentIds.Add(Rifle->ContentId);
	}
	if (Sniper)
	{
		ContentIds.Add(Sniper->ContentId);
	}

	return ContentIds;
}
