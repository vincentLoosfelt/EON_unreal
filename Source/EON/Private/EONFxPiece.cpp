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
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

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

UPointLightComponent* AEONFxPiece::AddLight(const FLinearColor& Col, float Intensity, float RadiusCm, bool bShadows)
{
	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(RootComponent);
	Light->SetLightColor(Col);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(RadiusCm);
	Light->SetCastShadows(bShadows);
	if (bShadows) { Light->SetSourceRadius(0.f); Light->SetSoftSourceRadius(0.f); }
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

UStaticMeshComponent* AEONFxPiece::AddMesh(UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Scale)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetGenerateOverlapEvents(false);
	C->SetupAttachment(RootComponent);
	C->SetRelativeScale3D(Scale);
	C->SetMaterial(0, Mid);
	C->RegisterComponent();
	if (Mid) { Mids.AddUnique(Mid); }
	Grow = C; Grow0 = Grow1 = Scale;
	return C;
}


void AEONFxPiece::TickShell(float Dt)
{
	// Proto : vz -= 260 px/s² ; au sol, rebond à 40 % si |vz| > 30 px/s (vitesse au sol ÷ 2), sinon repos.
	if (!bRest)
	{
		ShellVel.Z -= EON::PxToCm(260.f) * Dt;
		FVector P = ShellPos + ShellVel * Dt;
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
		ShellPos = P;
		ShellYaw = FMath::Fmod(ShellYaw + 900.f * Dt, 360.f);
	}
	else
	{
		RestLife -= Dt;
		if (RestLife <= 0.f) { Destroy(); return; }
	}
	// Affichage par bonds de 1/12 s (position, rotation et effacement final).
	ShellAcc += Dt;
	if (ShellAcc < 1.f / 12.f) { return; }
	ShellAcc = FMath::Fmod(ShellAcc, 1.f / 12.f);
	SetActorLocationAndRotation(ShellPos, FRotator(0.f, ShellYaw, 0.f));
	if (bRest && RestLife < 1.f) { SetActorScale3D(FVector(FMath::Max(0.05f, RestLife))); }
}

void AEONFxPiece::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bShell) { TickShell(Dt); return; }
	if (Delay > 0.f)
	{
		Delay -= Dt;
		if (Delay > 0.f) { return; }
		SetActorHiddenInGame(false);
	}
	Age += Dt;
	if (Age >= Life)
	{
		if (OnEnd) { TFunction<void(AEONFxPiece*)> F = MoveTemp(OnEnd); OnEnd = nullptr; F(this); }
		Destroy(); return;
	}
	const float VAge = bPas12 ? FMath::FloorToFloat(Age * 12.f) / 12.f : Age;
	const float K = FMath::Clamp(1.f - VAge / Life, 0.f, 1.f);
	if (Grow)
	{
		const float A = FMath::Clamp(VAge / FMath::Max(GrowT, 0.01f), 0.f, 1.f);
		Grow->SetRelativeScale3D(FMath::Lerp(Grow0, Grow1, A));
	}
	const float Img = bFlip ? FMath::Min((float)(FlipN - 1), FMath::FloorToFloat(Age / Life * FlipN)) : -1.f;
	if (FacePlane) { UpdateFace(); }
	for (UMaterialInstanceDynamic* M : Mids)
	{
		if (!M) { continue; }
		if (Img >= 0.f) { M->SetScalarParameterValue(TEXT("Image"), Img); }
		if (bTacheSol) { M->SetScalarParameterValue(TEXT("Opacite"), FMath::Clamp(Life - Age, 0.f, 1.f)); }
		if (!bNoFade) { M->SetScalarParameterValue(TEXT("Opacite"), Opacity0 * K); }
		if (bFill) { M->SetScalarParameterValue(TEXT("Remplissage"), Age / Life); }
	}
	// Boule de charge : la lueur croît avec la boule ; sinon elle décroît (carré de la vie restante).
	if (Light) { Light->SetIntensity(Grow ? Light0 * FMath::Square(FMath::Clamp(VAge / FMath::Max(GrowT, 0.01f), 0.f, 1.f)) : Light0 * K * K); }
}

void AEONFxPiece::UpdateFace()
{
	// Plan face à la caméra ; son +X (sens de l'image) suit le sens du tir vu à l'écran.
	FVector N(0.f, 0.f, 1.f);
	if (const APlayerCameraManager* Pcm = UGameplayStatics::GetPlayerCameraManager(this, 0)) { N = -Pcm->GetCameraRotation().Vector(); }
	FVector X = FaceDir - N * FVector::DotProduct(FaceDir, N);
	if (!X.Normalize()) { X = FVector::CrossProduct(N, FVector::UpVector).GetSafeNormal(); }
	const FMatrix R = FRotationMatrix::MakeFromXZ(X, N);
	FacePlane->SetWorldLocationAndRotation(GetActorLocation() + X * FaceAnchor * FaceSize + N * FaceLift, R.Rotator());
}
