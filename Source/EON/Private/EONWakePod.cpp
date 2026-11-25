#include "EONWakePod.h"
#include "EON.h"
#include "EONSfx.h"
#include "EONTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Repère de Cuve_1 (cm, socle posé au sol) : tube de verre rayon 70, de 130 à 316.
	constexpr float GlassZ0 = 130.f, GlassZ1 = 316.f, GlassR = 70.f;
	// Emplacement Light_red du corps (voyant des cuves scellées).
	constexpr int32 SlotRed = 4;
	float Smooth(float X) { X = FMath::Clamp(X, 0.f, 1.f); return X * X * (3.f - 2.f * X); }
}

AEONWakePod::AEONWakePod()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MCorps(TEXT("/Game/EON/Unity/import/fbx/Cuve_Reveil/Cuve_Reveil_Corps.Cuve_Reveil_Corps"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MVerre(TEXT("/Game/EON/Unity/import/fbx/Cuve_Reveil/Cuve_Reveil_Verre.Cuve_Reveil_Verre"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MCyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MSph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MLiq(TEXT("/Game/EON/Materials/Cuves/MI_EON_Liquide.MI_EON_Liquide"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MSil(TEXT("/Game/EON/Materials/Cuves/MI_EON_Silhouette.MI_EON_Silhouette"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MVer(TEXT("/Game/EON/Materials/Cuves/MI_EON_Verre.MI_EON_Verre"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MVap(TEXT("/Game/EON/Materials/Cuves/MI_EON_Vapeur.MI_EON_Vapeur"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MRed(TEXT("/Game/EON/Unity/import/fbx/Light_red.Light_red"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MOff(TEXT("/Game/EON/Unity/import/fbx/Metal_dark_grey.Metal_dark_grey"));
	SphereMesh = MSph.Object; CubeMesh = MCube.Object;
	VerreMat = MVer.Object; VapeurMat = MVap.Object; RougeMat = MRed.Object; EteintMat = MOff.Object;

	auto Make = [this](const TCHAR* Name, UStaticMesh* M, UMaterialInterface* Mat)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetStaticMesh(M);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		if (Mat) { C->SetMaterial(0, Mat); }
		return C;
	};
	Corps = Make(TEXT("Corps"), MCorps.Object, nullptr);
	Corps->SetCastShadow(true);
	Verre = Make(TEXT("Verre"), MVerre.Object, nullptr);
	Liquide = Make(TEXT("Liquide"), MCyl.Object, MLiq.Object);
	Liquide->SetTranslucentSortPriority(-1);   // derrière le verre
	// Silhouette d'expérience (cuves scellées) : tête, torse, jambes, dans le liquide.
	Tete = Make(TEXT("Tete"), MSph.Object, MSil.Object);
	Tete->SetRelativeLocation(FVector(0.f, 0.f, 168.f + GlassZ0 - 20.f));
	Tete->SetRelativeScale3D(FVector(0.26f));
	Torse = Make(TEXT("Torse"), MCyl.Object, MSil.Object);
	Torse->SetRelativeLocation(FVector(0.f, 0.f, 118.f + GlassZ0 - 20.f));
	Torse->SetRelativeScale3D(FVector(0.42f, 0.28f, 0.7f));
	Jambes = Make(TEXT("Jambes"), MCyl.Object, MSil.Object);
	Jambes->SetRelativeLocation(FVector(0.f, 0.f, 50.f + GlassZ0 - 20.f));
	Jambes->SetRelativeScale3D(FVector(0.3f, 0.2f, 0.75f));
	// Flaque verte au pied des cuves brisées (proto : lueur verte sous la cuve).
	Flaque = Make(TEXT("Flaque"), MCyl.Object, MLiq.Object);
	Flaque->SetRelativeLocation(FVector(0.f, 120.f, 0.6f));
	Flaque->SetRelativeScale3D(FVector(2.3f, 1.5f, 0.01f));

	Lueur = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lueur"));
	Lueur->SetupAttachment(Root);
	Lueur->SetRelativeLocation(FVector(0.f, 90.f, 120.f));
	Lueur->SetIntensityUnits(ELightUnits::Unitless);
	Lueur->SetAttenuationRadius(EON::PxToCm(36.f) * 2.f);
	Lueur->SetCastShadows(false);
	Voyant = CreateDefaultSubobject<UPointLightComponent>(TEXT("Voyant"));
	Voyant->SetupAttachment(Root);
	Voyant->SetRelativeLocation(FVector(0.f, 60.f, GlassZ1 + 50.f));
	Voyant->SetIntensityUnits(ELightUnits::Unitless);
	Voyant->SetAttenuationRadius(EON::PxToCm(12.f) * 2.f);
	Voyant->SetLightColor(FLinearColor(FColor(255, 61, 85)));
	Voyant->SetCastShadows(false);
}

void AEONWakePod::BeginPlay()
{
	Super::BeginPlay();
	LiquideMID = Liquide->CreateDynamicMaterialInstance(0);
	FlaqueMID = Flaque->CreateDynamicMaterialInstance(0);
	if (FlaqueMID) { FlaqueMID->SetScalarParameterValue(TEXT("Opacite"), 0.22f); FlaqueMID->SetScalarParameterValue(TEXT("Emission"), 1.2f); }
	Phase = FMath::FRandRange(0.f, 6.f);
	for (int32 I = 0; I < 48; ++I)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetupAttachment(Root);
		C->SetStaticMesh(SphereMesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetVisibility(false);
		C->RegisterComponent();
		FPuff P; P.C = C; P.M = C->CreateDynamicMaterialInstance(0, VapeurMat);
		Puffs.Add(P);
	}
}

void AEONWakePod::SetLight(const FColor& C, float K)
{
	Lueur->SetLightColor(FLinearColor(C));
	Lueur->SetIntensity(9000.f * K);
	Lueur->SetVisibility(K > 0.f);
}

void AEONWakePod::Init(EEONPodState InState)
{
	State = InState;
	const bool bSealed = State == EEONPodState::Sealed;
	Tete->SetVisibility(bSealed); Torse->SetVisibility(bSealed); Jambes->SetVisibility(bSealed);
	Flaque->SetVisibility(State == EEONPodState::Broken);
	Verre->SetVisibility(State != EEONPodState::Broken);
	Voyant->SetVisibility(bSealed);
	for (UStaticMeshComponent* E : Eclats) { if (E) { E->DestroyComponent(); } }
	Eclats.Reset();
	if (State == EEONPodState::Broken)
	{
		// Tube éclaté : des dents de verre restent sur le bord bas et le bord haut, quelques éclats au sol devant.
		FRandomStream R(GetUniqueID());
		for (int32 I = 0; I < 16; ++I)
		{
			const bool bTop = I >= 9, bFloor = I >= 13;
			const float A = R.FRandRange(0.f, 2.f * PI);
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetupAttachment(Root);
			C->SetStaticMesh(CubeMesh);
			C->SetMaterial(0, VerreMat);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			if (bFloor)
			{
				C->SetRelativeLocation(FVector(R.FRandRange(-90.f, 90.f), R.FRandRange(95.f, 160.f), 1.f));
				C->SetRelativeRotation(FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f));
				C->SetRelativeScale3D(FVector(R.FRandRange(0.08f, 0.2f), R.FRandRange(0.05f, 0.14f), 0.01f));
			}
			else
			{
				const float H = R.FRandRange(14.f, 42.f);
				const float Z = bTop ? GlassZ1 - H * 0.5f : GlassZ0 + H * 0.5f;
				C->SetRelativeLocation(FVector(FMath::Cos(A) * GlassR, FMath::Sin(A) * GlassR, Z));
				C->SetRelativeRotation(FRotator(R.FRandRange(-12.f, 12.f), FMath::RadiansToDegrees(A) + 90.f, R.FRandRange(-20.f, 20.f)));
				C->SetRelativeScale3D(FVector(R.FRandRange(0.18f, 0.4f), 0.015f, H / 100.f));
			}
			C->RegisterComponent();
			Eclats.Add(C);
		}
	}
	SetDrain(State == EEONPodState::Sealed || State == EEONPodState::Jira ? 0.f : 1.f);
	SetOpen(State == EEONPodState::Open ? 1.f : 0.f);
	// Lueurs du proto (L(...)) : scellée #3a8a70 0,6, brisée 0,18, Jira #7affd0 0,6 ; ouverte : faible.
	switch (State)
	{
	case EEONPodState::Sealed: SetLight(FColor(58, 138, 112), 0.6f); break;
	case EEONPodState::Broken: SetLight(FColor(58, 138, 112), 0.18f); break;
	case EEONPodState::Jira: SetLight(FColor(122, 255, 208), 0.6f); break;
	default: SetLight(FColor(62, 242, 224), 0.25f); break;
	}
}

void AEONWakePod::SetDrain(float D)
{
	// Liquide : cylindre (100 × 100, pivot au centre) qui remplit le tube ; il baisse avec la vidange.
	const float Z0 = GlassZ0 + 2.f, H = (GlassZ1 - GlassZ0 - 4.f) * (1.f - D);
	Liquide->SetVisibility(H > 1.f);
	Liquide->SetRelativeLocation(FVector(0.f, 0.f, Z0 + H * 0.5f));
	Liquide->SetRelativeScale3D(FVector((GlassR - 4.f) / 50.f, (GlassR - 4.f) / 50.f, FMath::Max(H, 0.1f) / 100.f));
	if (LiquideMID) { LiquideMID->SetScalarParameterValue(TEXT("Opacite"), State == EEONPodState::Jira ? 0.45f : 0.6f); }
}

void AEONWakePod::SetOpen(float K)
{
	// Le tube descend autour du socle (son bas passe sous le sol) : la cuve s'ouvre.
	Verre->SetRelativeLocation(FVector(0.f, 0.f, -(GlassZ1 - GlassZ0 + 10.f) * K));
}

void AEONWakePod::StartWake()
{
	if (State != EEONPodState::Jira) { return; }
	WakeT = 0.f;
}

void AEONWakePod::EmitPuff(bool bBurst)
{
	for (FPuff& P : Puffs)
	{
		if (P.Life > 0.f) { continue; }
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const float R = bBurst ? FMath::FRandRange(20.f, 60.f) : GlassR + FMath::FRandRange(-10.f, 10.f);
		P.C->SetRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, GlassZ0 + FMath::FRandRange(0.f, bBurst ? 120.f : 40.f)));
		P.V = FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * FMath::FRandRange(20.f, bBurst ? 160.f : 60.f) + FVector(0.f, 0.f, FMath::FRandRange(60.f, 140.f));
		P.Max = P.Life = FMath::FRandRange(0.7f, 1.1f);
		P.S0 = FMath::FRandRange(0.1f, 0.18f); P.S1 = P.S0 * FMath::FRandRange(4.f, 6.f);
		P.C->SetVisibility(true);
		return;
	}
}

void AEONWakePod::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (State == EEONPodState::Sealed)
	{
		// Le spécimen flotte doucement ; le voyant rouge clignote (proto : floor(time × 1,5 + x) % 2).
		const float Bob = FMath::Sin(Time * 0.9f + Phase) * 3.f;
		const float Z = GlassZ0 - 20.f;
		Tete->SetRelativeLocation(FVector(0.f, 0.f, Z + 168.f + Bob));
		Torse->SetRelativeLocation(FVector(0.f, 0.f, Z + 118.f + Bob));
		Jambes->SetRelativeLocation(FVector(0.f, 0.f, Z + 50.f + Bob));
		const bool bOn = FMath::FloorToInt(Time * 1.5f + Phase) % 2 == 1;
		if (bOn != bRedOn)
		{
			bRedOn = bOn;
			Corps->SetMaterial(SlotRed, bOn ? RougeMat.Get() : EteintMat.Get());
			Voyant->SetIntensity(bOn ? 2500.f : 0.f);
		}
	}
	if (WakeT >= 0.f)
	{
		const float T0 = WakeT;
		WakeT += DeltaSeconds;
		if (T0 < 0.6f && WakeT >= 0.6f) { EONSfx::Event(this, TEXT("hisslong")); }
		SetDrain(Smooth((WakeT - 0.6f) / 1.0f));
		if (T0 < 1.7f && WakeT >= 1.7f) { EONSfx::Event(this, TEXT("door"), 0.8f); }
		if (WakeT > 1.7f)
		{
			// Ouverture en 0,45 s (proto : open += dt × 2,2), freinée en fin de course ; la vapeur s'échappe.
			const float K = FMath::Clamp((WakeT - 1.7f) / 0.45f, 0.f, 1.f);
			SetOpen(1.f - FMath::Pow(1.f - K, 3.f));
			for (PuffAcc += DeltaSeconds * 45.f; PuffAcc >= 1.f; PuffAcc -= 1.f) { EmitPuff(false); }
		}
		if (WakeT >= 2.6f)
		{
			EONSfx::Event(this, TEXT("lift"));
			for (int32 I = 0; I < 20; ++I) { EmitPuff(true); }
			WakeT = -1.f;
			State = EEONPodState::Open;
			SetLight(FColor(62, 242, 224), 0.25f);
		}
	}
	for (FPuff& P : Puffs)
	{
		if (P.Life <= 0.f) { continue; }
		P.Life -= DeltaSeconds;
		if (P.Life <= 0.f) { P.C->SetVisibility(false); continue; }
		const float A = 1.f - P.Life / P.Max;
		P.V *= FMath::Max(0.f, 1.f - DeltaSeconds * 1.5f);
		P.C->AddRelativeLocation(P.V * DeltaSeconds);
		P.C->SetRelativeScale3D(FVector(FMath::Lerp(P.S0, P.S1, Smooth(A))));
		if (P.M) { P.M->SetScalarParameterValue(TEXT("Opacite"), 0.14f * (1.f - A) * FMath::Min(1.f, A * 6.f)); }
	}
}
