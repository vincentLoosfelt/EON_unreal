// EON — fonctions utilitaires pour le C++ et les Blueprints (échelle proto, réglages).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EONTypes.h"
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
};
