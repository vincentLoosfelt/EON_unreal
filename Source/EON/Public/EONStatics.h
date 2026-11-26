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
	/** Rampe toon 3 tons des celluloïds (Vincent, 23/11) : entrée = N·L × 0,5 + 0,5 (ombres portées incluses si
	 *  bRampIncludesShadow). Ombre = ShadowColor sous T1, ton moyen = MidColor de T1 à T2, lumière = 1 au-dessus de T2 ;
	 *  chaque passage adouci sur Soft. Reflet : rampe spéculaire à 0 sous HiT puis HiStrength (0 = pas de reflet). */
	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static bool ConfigureToonProfile3(UToonProfile* Profile, float T1, float T2, float Soft, FLinearColor ShadowColor, FLinearColor MidColor,
		float HiT, float HiStrength, float DiffuseIndirectScale, bool bRampIncludesShadow);

	UFUNCTION(BlueprintCallable, Category = "EON|Outils")
	static bool ConfigureToonProfile(UToonProfile* Profile, float Threshold, float Smooth, FLinearColor ShadowColor,
		float DiffuseIndirectScale = 0.35f, bool bRampIncludesShadow = true);

	/** Rendu hybride (Vincent, 23/11) : marque un élément animé comme « celluloïd » (CustomStencil = 1). Le post-process
	 *  M_PP_EON_Trait y trace le line art (brun, adaptatif) et porte l'ombre décalée du cel sur le décor.
	 *  Les collisions (formes) sont ignorées. Les effets translucides passent après le filtre (translucidité après DOF). */
	UFUNCTION(BlueprintCallable, Category = "EON|Rendu")
	static void MarkCel(AActor* Actor);

	/** Idem pour un seul composant (constructeurs, pièces créées à la volée).
	 *  bTrait = false : celluloïd sans line art (CustomStencil = 2 ; projectiles, Vincent 24/11). */
	UFUNCTION(BlueprintCallable, Category = "EON|Rendu")
	static void MarkCelComponent(UPrimitiveComponent* Component, bool bTrait = true);

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
	/** Éditeur : ajoute à la sortie de l'AnimGraph deux Transform (Modify) Bone (rotation remplacée, repère monde) :
	 *  UpperBone tourné de la variable UpperVar puis LowerBone de LowerVar (rotators), poids AlphaVar. Bras armé de Jira
	 *  tendu dans l'axe du tir (AEONCharacter::Tick calcule les rotations). */
	UFUNCTION(BlueprintCallable, Category = "EON|Editeur")
	static FString AddArmAim(UAnimBlueprint* AnimBlueprint, FName AlphaVar, FName UpperBone, FName UpperVar, FName LowerBone, FName LowerVar);
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
