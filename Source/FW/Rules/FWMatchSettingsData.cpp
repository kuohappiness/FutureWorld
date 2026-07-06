#include "FWMatchSettingsData.h"

#include "FWMatchRuleSet.h"

void UFWMatchSettingsData::CopyFromRuleSet(const UFWMatchRuleSet* RuleSet)
{
	if (!RuleSet)
	{
		return;
	}

	AIEnemyCount = RuleSet->AIEnemyCount;
	PlayerLives = RuleSet->PlayerLives;
	bRespawnEnabled = RuleSet->bRespawnEnabled;
	bCoreVehicleEnabled = RuleSet->bCoreVehicleEnabled;
	CoreVehiclePenalty = RuleSet->CoreVehiclePenalty;
}

void UFWMatchSettingsData::ApplyToRuleSet(UFWMatchRuleSet* RuleSet) const
{
	if (!RuleSet)
	{
		return;
	}

	RuleSet->AIEnemyCount = AIEnemyCount;
	RuleSet->PlayerLives = PlayerLives;
	RuleSet->bRespawnEnabled = bRespawnEnabled;
	RuleSet->bCoreVehicleEnabled = bCoreVehicleEnabled;
	RuleSet->CoreVehiclePenalty = CoreVehiclePenalty;
}

UFWMatchRuleSet* UFWMatchSettingsData::CreateRuntimeRuleSet(UObject* Outer) const
{
	UObject* RuleSetOuter = Outer ? Outer : GetTransientPackage();
	UFWMatchRuleSet* RuntimeRuleSet = NewObject<UFWMatchRuleSet>(RuleSetOuter);
	ApplyToRuleSet(RuntimeRuleSet);
	RuntimeRuleSet->ContentId = TEXT("RuntimeMatchSettings");
	RuntimeRuleSet->DisplayName = FText::FromString(TEXT("Runtime Match Settings"));
	return RuntimeRuleSet;
}
