using UnrealBuildTool;
using System.Collections.Generic;

public class FWTarget : TargetRules
{
	public FWTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		CppStandard = CppStandardVersion.Cpp20;
		WindowsPlatform.bWriteSarif = false;
		ExtraModuleNames.Add("FW");
	}
}
