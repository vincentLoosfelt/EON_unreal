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
		// Options : carte graphique et ray tracing matériel (GRHIAdapterName, GRHISupportsRayTracing).
		PrivateDependencyModuleNames.Add("RHI");
		// Outils éditeur (insertion de nœuds dans un Animation Blueprint : UEONStatics::AddUpperBodyLayer)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "AnimGraph", "Kismet" });
			// Émetteurs Lightweight de Niagara (EONVfxLib::UseMeshRenderer) : en-têtes « Internal » du plugin.
			PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Plugins/FX/Niagara/Source/Niagara/Internal"));
			PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Plugins/FX/Niagara/Source/NiagaraShader/Internal"));
			PrivateDependencyModuleNames.AddRange(new string[] { "NiagaraShader", "NiagaraCore", "RenderCore", "RHI" });
			// Éditeur de salles dans un onglet d'Unreal (EONSallesPanel.cpp) : navigateur intégré + serveur HTTP local.
			PrivateDependencyModuleNames.AddRange(new string[] { "WebBrowser", "HTTPServer", "Json", "WorkspaceMenuStructure" });
		}
	}
}
