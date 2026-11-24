#include "EONFxPiece.h"
#include "EONStatics.h"
#include "EONTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

TArray<TWeakObjectPtr<AEONFxPiece>> AEONFxPiece::Shells;

AEONFxPiece::AEONFxPiece()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetCanBeDamaged(false);
}

UStaticMeshComponent* AEONFxPiece::AddPlane(UMaterialInstanceDynamic* Mid, const FVector& Loc, float Yaw, float SizeXCm, float SizeYCm)
{
	static TStrongObjectPtr<UStaticMesh> Plane;
	if (!Plane.IsValid()) { Plane.Reset(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"))); }
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Plane.Get());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetGenerateOverlapEvents(false);
	C->SetupAttachment(RootComponent);
	C->SetRelativeLocationAndRotation(Loc, FRotator(0.f, Yaw, 0.f));
	C->SetRelativeScale3D(FVector(SizeXCm / 100.f, SizeYCm / 100.f, 1.f));
	C->SetMaterial(0, Mid);
	C->RegisterComponent();
	if (Mid) { Mids.AddUnique(Mid); }
	return C;
}

UPointLightComponent* AEONFxPiece::AddLight(const FLinearColor& Col, float Intensity, float RadiusCm)
{
	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(RootComponent);
	Light->SetLightColor(Col);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(RadiusCm);
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	Light0 = Intensity;
	return Light;
}

UPoseableMeshComponent* AEONFxPiece::AddGhost(USkeletalMeshComponent* Src, UMaterialInstanceDynamic* Mid)
{
	if (!Src || !Src->GetSkeletalMeshAsset()) { return nullptr; }
	UPoseableMeshComponent* G = NewObject<UPoseableMeshComponent>(this);
	G->SetSkinnedAssetAndUpdate(Src->GetSkeletalMeshAsset());
	G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	G->SetCastShadow(false);
	G->SetupAttachment(RootComponent);
	G->RegisterComponent();
	G->SetWorldTransform(Src->GetComponentTransform());
	G->CopyPoseFromSkeletalComponent(Src);
	for (int32 I = 0; I < G->GetNumMaterials(); ++I) { G->SetMaterial(I, Mid); }
	if (Mid) { Mids.AddUnique(Mid); }
	return G;
}

void AEONFxPiece::TickShell(float Dt)
{
	// Proto : vz -= 260 px/s² ; au sol, rebond à 40 % si |vz| > 30 px/s (vitesse au sol ÷ 2), sinon repos.
	if (!bRest)
	{
		ShellVel.Z -= EON::PxToCm(260.f) * Dt;
		FVector P = GetActorLocation() + ShellVel * Dt;
		if (P.Z <= FloorZ)
		{
			P.Z = FloorZ;
			if (FMath::Abs(ShellVel.Z) > EON::PxToCm(30.f))
			{
				ShellVel.Z = -ShellVel.Z * 0.4f;
				ShellVel.X *= 0.5f;
				ShellVel.Y *= 0.5f;
			}
			else
			{
				bRest = true;
				// Posée sur le vide ou dans un mur : elle disparaît (proto). Ici : rien sous elle à 40 cm.
				FHitResult H;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(EONShell), false, this);
				if (!GetWorld()->LineTraceSingleByChannel(H, P + FVector(0, 0, 10.f), P - FVector(0, 0, 40.f), ECC_Visibility, Q)) { Destroy(); return; }
			}
		}
		SetActorLocation(P);
		AddActorLocalRotation(FRotator(0.f, 900.f * Dt, 0.f));
	}
	else
	{
		RestLife -= Dt;
		if (RestLife <= 0.f) { Destroy(); return; }
		if (RestLife < 1.f) { SetActorScale3D(FVector(FMath::Max(0.05f, RestLife))); }
	}
}

void AEONFxPiece::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bShell) { TickShell(Dt); return; }
	Age += Dt;
	if (Age >= Life) { Destroy(); return; }
	const float K = FMath::Clamp(1.f - Age / Life, 0.f, 1.f);
	for (UMaterialInstanceDynamic* M : Mids)
	{
		if (!M) { continue; }
		if (!bNoFade) { M->SetScalarParameterValue(TEXT("Opacite"), Opacity0 * K); }
		if (bFill) { M->SetScalarParameterValue(TEXT("Remplissage"), Age / Life); }
	}
	if (Light) { Light->SetIntensity(Light0 * K * K); }
}
