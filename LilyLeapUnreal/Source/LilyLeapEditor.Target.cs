using UnrealBuildTool;

public class LilyLeapEditorTarget : TargetRules
{
	public LilyLeapEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("LilyLeap");
	}
}
