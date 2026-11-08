using UnrealBuildTool;

public class LilyLeapTarget : TargetRules
{
	public LilyLeapTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("LilyLeap");
	}
}
