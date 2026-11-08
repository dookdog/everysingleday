using UnrealBuildTool;

public class LilyLeap : ModuleRules
{
	public LilyLeap(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Each .cpp keeps small file-local helpers (colour names etc.); unity builds would merge them into one file.
		bUseUnity = false;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "AudioMixer"
		});
	}
}
