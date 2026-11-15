using UnrealBuildTool;

public class EON : ModuleRules
{
	public EON(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Chaque .cpp a ses constantes locales (ColRed, R...) : pas de regroupement Unity Build.
		bUseUnity = false;
		PublicIncludePaths.Add(ModuleDirectory + "/Public");
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "UMG", "Slate", "SlateCore", "AssetRegistry", "AnimGraphRuntime", "ProceduralMeshComponent"
		});
		// Outils éditeur (insertion de nœuds dans un Animation Blueprint : UEONStatics::AddUpperBodyLayer)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "AnimGraph", "Kismet" });
		}
	}
}
