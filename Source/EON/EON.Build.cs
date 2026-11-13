using UnrealBuildTool;

public class EON : ModuleRules
{
	public EON(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "UMG", "Slate", "SlateCore"
		});
	}
}
