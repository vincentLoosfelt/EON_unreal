#include "EONSpriteComponent.h"
#include "EON.h"
#include "EONTypes.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* MaterialPath = TEXT("/Game/EON/Materials/M_PixelSprite.M_PixelSprite");
	const TCHAR* SpritesPath = TEXT("/Game/EON/Data/DT_Sprites.DT_Sprites");

	/** Orientation par défaut : face à la caméra plongeante (yaw -90, plongée 55°). */
	void DefaultCamAxes(FVector& Fwd, FVector& Up)
	{
		const FRotator R(-55.f, -90.f, 0.f);
		Fwd = R.Vector();
		Up = FRotationMatrix(R).GetUnitAxis(EAxis::Z);
	}
}

UEONSpriteComponent::UEONSpriteComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	bTickInEditor = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	CastShadow = false;
	bReceivesDecals = false;
	SetCanEverAffectNavigation(false);
}

void UEONSpriteComponent::OnRegister()
{
	if (!bFootCaptured)
	{
		FootLocal = GetRelativeLocation();
		bFootCaptured = true;
	}
	if (!GetStaticMesh())
	{
		SetStaticMesh(LoadObject<UStaticMesh>(nullptr, PlanePath));
	}
	Super::OnRegister();
	RefreshSprite();
}

void UEONSpriteComponent::RefreshSprite()
{
	int32 FW = 16, FH = 16;
	Frames = 1;
	if (const UDataTable* DT = LoadObject<UDataTable>(nullptr, SpritesPath))
	{
		if (const FEONSpriteRow* Row = DT->FindRow<FEONSpriteRow>(SpriteName, TEXT("EONSprite"), false))
		{
			FW = Row->FW; FH = Row->FH; Frames = FMath::Max(1, Row->N);
		}
	}
	SizeCm = FVector2D(EON::PxToCm(FW), EON::PxToCm(FH)) * DisplayScale;
	// Le plan de base fait 100 × 100 cm.
	SetRelativeScale3D(FVector(SizeCm.X / 100.f, SizeCm.Y / 100.f, 1.f));

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
	if (Base)
	{
		if (!Mid || Mid->Parent != Base)
		{
			Mid = UMaterialInstanceDynamic::Create(Base, this);
			SetMaterial(0, Mid);
		}
		const FString TexPath = FString::Printf(TEXT("/Game/EON/Placeholders/T_%s.T_%s"), *SpriteName.ToString(), *SpriteName.ToString());
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *TexPath, nullptr, LOAD_NoWarn))
		{
			Mid->SetTextureParameterValue(TEXT("Tex"), Tex);
		}
		else if (!SpriteName.IsNone())
		{
			UE_LOG(LogEON, Warning, TEXT("Sprite placeholder introuvable : %s"), *TexPath);
		}
	}
	ApplyParams();

	FVector Fwd, Up;
	DefaultCamAxes(Fwd, Up);
	FaceCamera(Fwd, Up);
}

void UEONSpriteComponent::ApplyParams()
{
	if (!Mid) { return; }
	Mid->SetScalarParameterValue(TEXT("Frames"), (float)Frames);
	Mid->SetScalarParameterValue(TEXT("Frame"), (float)FMath::Clamp(Frame, 0, Frames - 1));
	Mid->SetScalarParameterValue(TEXT("FlipX"), bFlipX ? 1.f : 0.f);
	Mid->SetVectorParameterValue(TEXT("Tint"), Tint);
}

void UEONSpriteComponent::FaceCamera(const FVector& CamForward, const FVector& CamUp)
{
	// Plan : normale locale +Z vers la caméra, haut de l'image = haut de l'écran.
	const FVector N = -CamForward;
	// Sur le plan de base, le V de la texture suit l'axe local Y : Y = bas de l'écran.
	const FMatrix M = FRotationMatrix::MakeFromZY(N, -CamUp);
	SetWorldRotation(M.Rotator());

	// Le bas du sprite est aux pieds : on remonte le centre de H/2 selon le haut de l'écran.
	if (const USceneComponent* Parent = GetAttachParent())
	{
		const FVector Feet = Parent->GetComponentTransform().TransformPosition(FootLocal);
		SetWorldLocation(Feet + CamUp * (SizeCm.Y * 0.5f));
	}
}

void UEONSpriteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AnimFps > 0.f && Frames > 1)
	{
		AnimTime += DeltaTime;
		Frame = FMath::FloorToInt(AnimTime * AnimFps) % Frames;
	}
	ApplyParams();

	FVector Fwd, Up;
	DefaultCamAxes(Fwd, Up);
	if (const UWorld* W = GetWorld())
	{
		if (const APlayerController* PC = W->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				const FRotator R = PC->PlayerCameraManager->GetCameraRotation();
				Fwd = R.Vector();
				Up = FRotationMatrix(R).GetUnitAxis(EAxis::Z);
			}
		}
	}
	FaceCamera(Fwd, Up);
}

#if WITH_EDITOR
void UEONSpriteComponent::PostEditChangeProperty(FPropertyChangedEvent& E)
{
	Super::PostEditChangeProperty(E);
	RefreshSprite();
}
#endif
