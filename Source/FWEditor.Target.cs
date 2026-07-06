using UnrealBuildTool;
using System.Collections.Generic;

public class FWEditorTarget : TargetRules
{
	public FWEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		CppStandard = CppStandardVersion.Cpp20;
		WindowsPlatform.bWriteSarif = false;
		ExtraModuleNames.Add("FW");
	}
}
