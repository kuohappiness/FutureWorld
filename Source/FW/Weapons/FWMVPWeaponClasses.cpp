#include "FWMVPWeaponClasses.h"

#include "FWWeaponData.h"

void AFWMVPWeaponBase::BeginPlay()
{
	if (!WeaponData && !DefaultWeaponDataPath.IsNone())
	{
		WeaponData = LoadObject<UFWWeaponData>(nullptr, *DefaultWeaponDataPath.ToString());
	}

	Super::BeginPlay();
}

void AFWMVPWeaponBase::SetDefaultWeaponDataPath(FName AssetPath)
{
	DefaultWeaponDataPath = AssetPath;
	if (!DefaultWeaponDataPath.IsNone())
	{
		WeaponData = LoadObject<UFWWeaponData>(nullptr, *DefaultWeaponDataPath.ToString());
	}
}

AFWPistolWeapon::AFWPistolWeapon()
{
	SetDefaultWeaponDataPath(TEXT("/Game/FW/Weapons/Data/DA_Weapon_Pistol.DA_Weapon_Pistol"));
}

AFWRifleWeapon::AFWRifleWeapon()
{
	SetDefaultWeaponDataPath(TEXT("/Game/FW/Weapons/Data/DA_Weapon_Rifle.DA_Weapon_Rifle"));
}

AFWSniperWeapon::AFWSniperWeapon()
{
	SetDefaultWeaponDataPath(TEXT("/Game/FW/Weapons/Data/DA_Weapon_Sniper.DA_Weapon_Sniper"));
}
