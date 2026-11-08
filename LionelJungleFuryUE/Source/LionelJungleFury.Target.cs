using UnrealBuildTool;
using System.Collections.Generic;

public class LionelJungleFuryTarget : TargetRules
{
	public LionelJungleFuryTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("LionelJungleFury");
	}
}
