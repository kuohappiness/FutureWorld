using UnrealBuildTool;

public class FW : ModuleRules
{
	public FW(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ApplicationCore",
			"UMG",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"GameplayTags",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"AssetRegistry",
				"UMGEditor"
			});
		}
	}
}
