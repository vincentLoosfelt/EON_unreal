// EON — arc électrique du HUB (ElectricArc.cs d'Unity, 8 instances dans Room_elevator) : relie une monture d'angle
// (Rampe electro) au C.O.M, champ magnétique qui fait circuler le HUB verticalement (Vincent, 15/11).
// Même algorithme qu'Unity : 24 segments, bruit de Perlin doux + cassures quantifiées, enveloppe sin^1.5, halo x3,
// 2 arcs secondaires, scintillement, graines décalées toutes les 0,03 s, extrémités replacées toutes les 2-3 s.
// Rendu : rubans face caméra (ProceduralMesh), matériau additif non éclairé M_EON_Arc (couleur = sommets).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONElectricArc.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class EON_API AEONElectricArc : public AActor
{
	GENERATED_BODY()

public:
	AEONElectricArc();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Arc") TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY(EditAnywhere, Category = "Arc") TObjectPtr<UMaterialInterface> Material;

	/** Extrémités, en monde (cm). */
	UPROPERTY(EditAnywhere, Category = "Arc") FVector StartPoint = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Arc") FVector EndPoint = FVector(100.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Arc") float StartRadius = 20.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float EndRadius = 20.f;
	/** Points des surfaces émissives (glissières, lumières du C.O.M) où les extrémités se promènent (Vincent, 15/11). Vide = rayon autour du point. */
	UPROPERTY(EditAnywhere, Category = "Arc") TArray<FVector> StartCandidates;
	UPROPERTY(EditAnywhere, Category = "Arc") TArray<FVector> EndCandidates;

	UPROPERTY(EditAnywhere, Category = "Arc") int32 Segments = 12;
	/** Épaisseur du cœur en cm (Unity 0,06 m). */
	UPROPERTY(EditAnywhere, Category = "Arc") float Thickness = 16.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float NoiseIntensity = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Arc") float NoiseFrequency = 3.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float AnimationSpeed = 12.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float ReshapeInterval = 0.06f;
	UPROPERTY(EditAnywhere, Category = "Arc") FLinearColor ArcColor = FLinearColor(0.f, 0.6346f, 4.f, 1.f);   // émission des rails (electro : 0 ; 0,79 ; 5)
	UPROPERTY(EditAnywhere, Category = "Arc") FLinearColor GlowColor = FLinearColor(0.f, 0.6346f, 4.f, 0.4f);
	UPROPERTY(EditAnywhere, Category = "Arc") float FlickerSpeed = 20.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float FlickerMin = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Arc") int32 SecondaryCount = 1;
	UPROPERTY(EditAnywhere, Category = "Arc") float SecondarySpread = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Arc") float RepositionMin = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Arc") float RepositionMax = 1.2f;

private:
	struct FBranch { float StartT, EndT, Seed; };
	TArray<FBranch> Branches;
	FVector CurStart = FVector::ZeroVector;
	FVector CurEnd = FVector::ZeroVector;
	float SeedX = 0.f, SeedY = 0.f, SeedZ = 0.f;
	float ReshapeT = 0.f, FlickerT = 0.f, Flicker = 1.f, NextReposition = 2.f, Time = 0.f;

	void Generate(TArray<FVector>& Out, const FVector& A, const FVector& B, float SeedOffset, int32 Count) const;
	void AddRibbon(const TArray<FVector>& P, float Width, const FLinearColor& Color, const FVector& Eye,
		TArray<FVector>& V, TArray<int32>& I, TArray<FColor>& C) const;
};
