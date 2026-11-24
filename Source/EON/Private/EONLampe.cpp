#include "EONLampe.h"

#include "Components/ChildActorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EONFloorManager.h"
#include "EONTypes.h"
#include "EONStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AEONLampe::AEONLampe()
{
	PrimaryActorTick.bCanEverTick = true;
	Racine = CreateDefaultSubobject<USceneComponent>(TEXT("Racine"));
	RootComponent = Racine;
	Lumiere = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lumiere"));
	Lumiere->SetupAttachment(Racine);
	Lumiere->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	Lumiere->SetMobility(EComponentMobility::Movable);
	Lumiere->SetCastShadows(false);
}

void AEONLampe::LireReglages()
{
	// Réglages de la lampe posée par l'éditeur de salles (tags du ChildActorComponent de la salle).
	const UChildActorComponent* CA = GetParentComponent();
	if (!CA) { return; }
	for (const FName& Tag : CA->ComponentTags)
	{
		const FString S = Tag.ToString();
		if (S.StartsWith(TEXT("eonlum=")))
		{
			TArray<FString> P;
			S.Mid(7).ParseIntoArray(P, TEXT(";"), false);
			if (P.Num() > 0)
			{
				Mode = P[0] == TEXT("clignote") ? EEONLampeMode::Clignote : P[0] == TEXT("vacille") ? EEONLampeMode::Vacille : EEONLampeMode::Fixe;
			}
			if (P.Num() > 1 && P[1].Len() == 7) { Couleur = FLinearColor(FColor::FromHex(P[1])); }
			if (P.Num() > 2) { RayonPx = FMath::Max(1.f, FCString::Atof(*P[2])); }
			if (P.Num() > 3) { Intensite = FMath::Max(0.f, FCString::Atof(*P[3])); }
		}
		else if (S.StartsWith(TEXT("eonpos=")) && Phase < 0.f)
		{
			// Proto : bl = colonne de tuile × 0,37.
			Phase = FCString::Atoi(*S.Mid(7)) * 0.37f;
		}
	}
}

void AEONLampe::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Lumiere->SetLightColor(Couleur);
	Lumiere->SetAttenuationRadius(EON::PxToCm(RayonPx));
	Lumiere->SetIntensity(Intensite * IntensiteUnreal);
}

void AEONLampe::BeginPlay()
{
	Super::BeginPlay();
	LireReglages();
	if (Phase < 0.f) { Phase = FMath::Abs(GetActorLocation().X) / EON::CmPerTile * 0.37f; }
	const FVector L = Lumiere->GetComponentLocation();
	PosPx = FVector2D(L.X, L.Y) / EON::CmPerPx;
	Lumiere->SetLightColor(Couleur);
	Lumiere->SetAttenuationRadius(EON::PxToCm(RayonPx));
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents(Meshes);
	for (UStaticMeshComponent* M : Meshes)
	{
		if (!M->ComponentHasTag(TEXT("neon"))) { continue; }
		for (int32 I = 0; I < M->GetNumMaterials(); ++I)
		{
			if (UMaterialInstanceDynamic* D = M->CreateDynamicMaterialInstance(I))
			{
				D->SetVectorParameterValue(TEXT("Couleur"), Couleur);
				Neons.Add(D);
			}
		}
	}
	Appliquer(1.f);
}

void AEONLampe::Appliquer(float F)
{
	if (F == Dernier) { return; }
	Dernier = F;
	const FEONGameFeelRow Feel = UEONStatics::GetGameFeel(this);   // LampeMul, NeonMul : DT_GameFeel (Vincent, 24/11)
	Lumiere->SetIntensity(Intensite * F * IntensiteUnreal * Feel.LampeMul);
	for (UMaterialInstanceDynamic* D : Neons) { D->SetScalarParameterValue(TEXT("Emission"), EmissionNeon * Feel.NeonMul * F); }
}

void AEONLampe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UWorld* W = GetWorld();
	const float T = W ? W->GetTimeSeconds() : 0.f;
	float F = 1.f;
	if (Mode == EEONLampeMode::Clignote && FMath::FloorToInt(T * 2.2f + Phase) % 3 == 0) { F *= 0.1f; }
	if (Mode == EEONLampeMode::Vacille && FMath::Sin(T * 37.f + PosPx.X) * FMath::Sin(T * 23.f + PosPx.Y) > 0.55f) { F *= 0.12f; }
	// Irruption : l'étage assombrit la salle (vacillement puis noir) ; les lampes suivent.
	if (const AEONFloorManager* FM = AEONFloorManager::Get(this))
	{
		if (FM->GetExposureBias() < 0.f) { F = 0.f; }
	}
	// Proto : lampe éteinte = trait sombre (#14122a), le néon n'émet presque plus.
	Appliquer(F);
}
