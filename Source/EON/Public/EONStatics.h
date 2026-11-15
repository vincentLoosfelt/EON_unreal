// EON — fonctions utilitaires pour le C++ et les Blueprints (échelle proto, réglages).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EONTypes.h"
class UToonProfile;
class UMaterialInstanceConstant;
class USkeleton;
class UAnimBlueprint;
class UAnimSequence;
class UBlueprint;
#include "EONStatics.generated.h"

UCLASS()
class EON_API UEONStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** px proto -> cm Unreal (1 px = 6,25 cm). */
	UFUNCTION(BlueprintPure, Category = "EON|Échelle", meta = (CompactNodeTitle = "px→cm"))
	static float PxToCm(float Px) { return EON::PxToCm(Px); }

	/** cm Unreal -> px proto. */
	UFUNCTION(BlueprintPure, Category = "EON|Échelle", meta = (CompactNodeTitle = "cm→px"))
	static float CmToPx(float Cm) { return EON::CmToPx(Cm); }

	/** Position proto (x, y en px) -> position Unreal au sol (Z = 0). x proto = X, y proto (vers le bas) = Y. */
	UFUNCTION(BlueprintPure, Category = "EON|Échelle")
	static FVector ProtoToWorld(FVector2D ProtoPx) { return FVector(EON::PxToCm(ProtoPx.X), EON::PxToCm(ProtoPx.Y), 0.f); }

	UFUNCTION(BlueprintPure, Category = "EON|Échelle")
	static FVector2D WorldToProto(FVector World) { return FVector2D(EON::CmToPx(World.X), EON::CmToPx(World.Y)); }

	/** Angle proto (radians, 0 = droite, + = vers le bas de l'écran) -> direction Unreal au sol. */
	UFUNCTION(BlueprintPure, Category = "EON|Échelle")
	static FVector ProtoAngleToDir(float Radians) { return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f); }

	/** Réglages du banc en cours. */
	UFUNCTION(BlueprintPure, Category = "EON|Réglages", meta = (WorldContext = "WorldContext"))
	static FEONGameFeelRow GetGameFeel(const UObject* WorldContext);

	/** Outil éditeur : active ou coupe la collision d'une section de static mesh (ex. dôme de la COM).
	 *  Passe aussi le mesh en « collision complexe = simple » pour suivre la forme exacte. */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static bool SetMeshSectionCollision(UStaticMesh* Mesh, int32 LodIndex, int32 SectionIndex, bool bEnable, bool bUseComplexAsSimple = true);

	/** Outil éditeur : rampe toon à deux tons (BG.shadergraph d'Unity) dans un ToonProfile.
	 *  Entrée de la rampe = N·L × 0,5 + 0,5 (comme Unity). Ombre = ShadowColor (par canal : teinte d'ombre Unity), éclairé = 1, transition
	 *  de Threshold à Threshold + Smooth. Spéculaire coupé (Unity n'en a pas). */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static bool ConfigureToonProfile(UToonProfile* Profile, float Threshold, float Smooth, FLinearColor ShadowColor,
		float DiffuseIndirectScale = 0.35f, bool bRampIncludesShadow = true);

	/** Outil éditeur : profil toon propre à une instance (ToonProfileOverride n'est pas exposé à Python). */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static bool SetInstanceToonProfile(UMaterialInstanceConstant* Instance, UToonProfile* Profile);

	/** Outil éditeur : modes de retarget de translation d'un squelette partagé par des corps de proportions différentes.
	 *  Tous les os en Skeleton, sauf AnimationBones (Animation) et ScaledBones (AnimationScaled). Renvoie le nb d'os réglés. */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static int32 SetTranslationRetargeting(USkeleton* Skeleton, const TArray<FName>& AnimationBones, const TArray<FName>& ScaledBones);
	/** Outil éditeur : insère juste avant la sortie de l'AnimGraph d'un Animation Blueprint une couche haut du corps
	 *  (Layered Blend per Bone depuis SpineBone, rotations en espace maillage) jouant Pose en boucle, pondérée par la
	 *  variable float AlphaVar (créée si absente). Idempotent (nœud marqué « EON_TIR »). Renvoie un compte rendu. */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static FString AddUpperBodyLayer(UAnimBlueprint* AnimBlueprint, UAnimSequence* Pose, FName SpineBone, FName AlphaVar);
	/** Remplit une structure de propriétés d'animation (S_CharacterPropertiesForAnimation du Game Animation Sample)
	 *  depuis un AEONCharacter, champ par champ selon leur nom. Appelée par BP_Jira (interface BPI_SandboxCharacter_Pawn). */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "EON|Anim", meta = (CustomStructureParam = "Props", DefaultToSelf = "Character"))
	static void FillAnimProps(AActor* Character, UPARAM(ref) int32& Props);
	DECLARE_FUNCTION(execFillAnimProps)
	{
		P_GET_OBJECT(AActor, Character);
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		void* Addr = Stack.MostRecentPropertyAddress;
		FStructProperty* SP = CastField<FStructProperty>(Stack.MostRecentProperty);
		P_FINISH;
		P_NATIVE_BEGIN;
		if (SP && Addr) { FillAnimPropsImpl(Character, SP->Struct, Addr); }
		P_NATIVE_END;
	}
	static void FillAnimPropsImpl(AActor* Character, const UScriptStruct* Struct, void* Data);

	/** Outil éditeur : fait implémenter Interface à Blueprint et écrit la fonction FuncName : variable locale du type de
	 *  sa sortie, remplie par FillAnimProps(self), renvoyée. Idempotent. Renvoie un compte rendu. */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static FString ImplementAnimPropsInterface(UBlueprint* Blueprint, UClass* Interface, FName FuncName);
};
