using UnrealBuildTool;

public class LionelJungleFury : ModuleRules
{
	public LionelJungleFury(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "ProceduralMeshComponent"
		});
	}
}
