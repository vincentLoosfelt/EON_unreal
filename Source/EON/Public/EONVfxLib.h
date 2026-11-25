// EON — Outils éditeur pour fabriquer les effets Niagara par script (Python) : les émetteurs « Lightweight »
// (stateless) de Niagara 5.8 ne sont pas exposés à Python. Ces fonctions lisent et écrivent leurs réglages
// par réflexion (chemin de propriété + texte), et lient un réglage à un paramètre utilisateur du système.
// Utilisé par Saved/Scripts/eon_vfx_build.py. Sans effet hors éditeur.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EONVfxLib.generated.h"

class UNiagaraSystem;

UCLASS()
class EON_API UEONVfxLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Émetteur Lightweight n° Index du système (nul si l'émetteur est standard). */
	UFUNCTION(BlueprintCallable, Category = "EON|VFX")
	static UObject* GetStatelessEmitter(UNiagaraSystem* System, int32 Index);

	/** Toutes les propriétés de l'objet en texte, sous-objets instanciés compris (modules, rendus). */
	UFUNCTION(BlueprintCallable, Category = "EON|VFX")
	static FString DumpObject(UObject* Object);

	/** Écrit Value (format texte d'Unreal) au chemin Path, ex. « Modules[2].DragDistribution.Mode ». */
	UFUNCTION(BlueprintCallable, Category = "EON|VFX")
	static bool SetProp(UObject* Object, const FString& Path, const FString& Value);

	/** Lie la distribution au chemin Path au paramètre utilisateur User.<UserName> (Type : float, int, color, vec2, vec3), et l'ajoute au système. */
	UFUNCTION(BlueprintCallable, Category = "EON|VFX")
	static bool BindUser(UNiagaraSystem* System, UObject* Object, const FString& Path, const FString& UserName, const FString& Type);

	/** Remplace les rendus de l'émetteur Lightweight par un rendu de maillage (Mesh, matériau Material) : fumée en sphères (24/11). */
	UFUNCTION(BlueprintCallable, Category = "EON|VFX")
	static bool UseMeshRenderer(UObject* Emitter, class UStaticMesh* Mesh, class UMaterialInterface* Material);

	/** Après les réglages : reconstruit l'émetteur et recompile le système. */
	UFUNCTION(BlueprintCallable, Category = "EON|VFX")
	static void FinishSystem(UNiagaraSystem* System, UObject* Emitter);
};
