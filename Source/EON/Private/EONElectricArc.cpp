#include "EONElectricArc.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Mathf.PerlinNoise (Unity) renvoie ~[0, 1]
	float Perlin01(float X, float Y) { return FMath::PerlinNoise2D(FVector2D(X, Y)) * 0.5f + 0.5f; }

	// widthCurve du LineRenderer Unity : effilé aux deux bouts
	float WidthCurve(float T)
	{
		static const float K[6][2] = {{0.f, 0.1f}, {0.1f, 0.8f}, {0.3f, 1.f}, {0.7f, 1.f}, {0.9f, 0.8f}, {1.f, 0.1f}};
		for (int32 i = 1; i < 6; ++i)
		{
			if (T <= K[i][0]) { return FMath::Lerp(K[i - 1][1], K[i][1], (T - K[i - 1][0]) / (K[i][0] - K[i - 1][0])); }
		}
		return 0.1f;
	}

	// Couleur HDR -> sommet : teinte normalisée en RGB, intensité (max × alpha) / 4 dans l'alpha ; M_EON_Arc remultiplie.
	FColor ToVertex(const FLinearColor& C)
	{
		const float M = FMath::Max3(C.R, C.G, C.B);
		if (M <= 0.f) { return FColor(0, 0, 0, 0); }
		auto B = [](float V) { return uint8(FMath::Clamp(V, 0.f, 1.f) * 255.f); };
		return FColor(B(C.R / M), B(C.G / M), B(C.B / M), B(M * C.A / 4.f));
	}
}

AEONElectricArc::AEONElectricArc()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> M(TEXT("/Game/EON/Materials/MI_EON_Arc.MI_EON_Arc"));
	if (M.Succeeded()) { Material = M.Object; }
}

void AEONElectricArc::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(FVector::ZeroVector);   // les sommets sont calculés en monde
	SetActorRotation(FRotator::ZeroRotator);
	SetActorScale3D(FVector::OneVector);
	CurStart = StartCandidates.Num() ? StartCandidates[FMath::RandRange(0, StartCandidates.Num() - 1)] : StartPoint;
	CurEnd = EndCandidates.Num() ? EndCandidates[FMath::RandRange(0, EndCandidates.Num() - 1)] : EndPoint;
	SeedX = FMath::FRandRange(0.f, 1000.f);
	SeedY = FMath::FRandRange(0.f, 1000.f);
	SeedZ = FMath::FRandRange(0.f, 1000.f);
	NextReposition = FMath::FRandRange(RepositionMin, RepositionMax);
	Branches.Reset();
	for (int32 i = 0; i < SecondaryCount; ++i)
	{
		Branches.Add({FMath::FRandRange(0.1f, 0.5f - SecondarySpread), FMath::FRandRange(0.5f + SecondarySpread, 0.9f), FMath::FRandRange(0.f, 1000.f)});
	}
}

void AEONElectricArc::Generate(TArray<FVector>& Out, const FVector& A, const FVector& B, float SeedOffset, int32 Count) const
{
	Out.SetNum(Count);
	const FVector Dir = B - A;
	const float Len = Dir.Size();
	const FVector Fwd = Dir.GetSafeNormal();
	FVector Right = FVector::CrossProduct(Fwd, FVector::UpVector).GetSafeNormal();
	if (Right.IsNearlyZero()) { Right = FVector::CrossProduct(Fwd, FVector::ForwardVector).GetSafeNormal(); }
	const FVector Up = FVector::CrossProduct(Right, Fwd).GetSafeNormal();
	const float T0 = Time * AnimationSpeed;
	const float S = SeedX + SeedOffset;
	for (int32 i = 0; i < Count; ++i)
	{
		const float T = float(i) / float(Count - 1);
		const float Env = FMath::Pow(FMath::Max(0.f, FMath::Sin(T * PI)), 1.5f);   // sin(pi) < 0 de peu -> NaN sans le Max
		const float Scale = NoiseIntensity * Len * 0.15f * Env;
		const float Nx = Perlin01(T * NoiseFrequency + S + T0, SeedY) - 0.5f;
		const float Ny = Perlin01(T * NoiseFrequency + SeedY + T0 * 1.3f, SeedZ) - 0.5f;
		float Sx = Perlin01(T * NoiseFrequency * 2.f + S + T0 * 0.7f, SeedZ + 200.f) - 0.5f;
		float Sy = Perlin01(T * NoiseFrequency * 2.f + SeedZ + T0 * 0.9f, S + 200.f) - 0.5f;
		Sx = FMath::RoundToFloat(Sx * 4.f) / 4.f;
		Sy = FMath::RoundToFloat(Sy * 4.f) / 4.f;
		Out[i] = FMath::Lerp(A, B, T) + (Right * (Nx * 0.5f + Sx * 0.5f) + Up * (Ny * 0.5f + Sy * 0.5f)) * Scale * 2.f;
	}
}

void AEONElectricArc::AddRibbon(const TArray<FVector>& P, float Width, const FLinearColor& Color, const FVector& Eye,
	TArray<FVector>& V, TArray<int32>& I, TArray<FColor>& C) const
{
	const int32 Base = V.Num();
	const FColor Col = ToVertex(Color);
	const int32 N = P.Num();
	for (int32 i = 0; i < N; ++i)
	{
		const FVector Tan = (P[FMath::Min(i + 1, N - 1)] - P[FMath::Max(i - 1, 0)]).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Tan, (Eye - P[i]).GetSafeNormal()).GetSafeNormal();
		const float W = Width * 0.5f * WidthCurve(float(i) / float(N - 1));
		V.Add(P[i] - Side * W);
		V.Add(P[i] + Side * W);
		C.Add(Col);
		C.Add(Col);
		if (i > 0)
		{
			const int32 a = Base + (i - 1) * 2;
			I.Append({a, a + 2, a + 1, a + 1, a + 2, a + 3});
		}
	}
}

void AEONElectricArc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	// Extrémités replacées au hasard toutes les 2-3 s (rayon autour du point d'origine)
	NextReposition -= DeltaSeconds;
	if (NextReposition <= 0.f)
	{
		NextReposition = FMath::FRandRange(RepositionMin, RepositionMax);
		CurStart = StartCandidates.Num() ? StartCandidates[FMath::RandRange(0, StartCandidates.Num() - 1)] : StartPoint + FMath::VRand() * FMath::FRand() * StartRadius;
		CurEnd = EndCandidates.Num() ? EndCandidates[FMath::RandRange(0, EndCandidates.Num() - 1)] : EndPoint + FMath::VRand() * FMath::FRand() * EndRadius;
	}

	// Scintillement (deux couches de Perlin)
	FlickerT += DeltaSeconds * FlickerSpeed;
	const float N1 = Perlin01(FlickerT, 0.f);
	const float N2 = FMath::Square(Perlin01(FlickerT * 3.1f, 50.f));
	Flicker = FMath::Lerp(FlickerMin, 1.f, N1 * 0.6f + N2 * 0.4f);

	// Remodelage : graines décalées
	ReshapeT += DeltaSeconds;
	if (ReshapeT >= ReshapeInterval)
	{
		ReshapeT = 0.f;
		SeedX += FMath::FRandRange(10.f, 50.f);
		SeedY += FMath::FRandRange(10.f, 50.f);
		SeedZ += FMath::FRandRange(10.f, 50.f);
	}

	FVector Eye(0.f, 0.f, 10000.f);
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0)) { Eye = Cam->GetCameraLocation(); }

	TArray<FVector> V;
	TArray<int32> I;
	TArray<FColor> C;
	TArray<FVector> P;
	Generate(P, CurStart, CurEnd, 0.f, Segments);
	AddRibbon(P, Thickness * 3.f * Flicker, GlowColor * Flicker, Eye, V, I, C);   // halo (Unity : épaisseur ×3)
	AddRibbon(P, Thickness * Flicker, ArcColor * Flicker, Eye, V, I, C);          // cœur
	const FVector Perp = FVector::CrossProduct((CurEnd - CurStart).GetSafeNormal(), FVector::UpVector).GetSafeNormal();
	for (const FBranch& Br : Branches)
	{
		const float Nv = Perlin01(Time * AnimationSpeed * 0.5f + Br.Seed, Br.Seed * 0.7f) - 0.5f;
		const FVector Off = Perp * Nv * NoiseIntensity * 0.3f * 100.f;   // Unity en m -> cm
		Generate(P, FMath::Lerp(CurStart, CurEnd, Br.StartT) + Off, FMath::Lerp(CurStart, CurEnd, Br.EndT) - Off * 0.5f, Br.Seed, Segments);
		AddRibbon(P, Thickness * 0.4f * Flicker, ArcColor * (Flicker * 0.6f), Eye, V, I, C);
	}
	Mesh->CreateMeshSection(0, V, I, TArray<FVector>(), TArray<FVector2D>(), C, TArray<FProcMeshTangent>(), false);
	if (Material) { Mesh->SetMaterial(0, Material); }
}
