#include "EONNpc.h"
#include "EONStatics.h"
#include "EONTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AEONNpc::AEONNpc()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Corps"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(Cyl.Object);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tete"));
	Head->SetupAttachment(Root);
	Head->SetStaticMesh(Sph.Object);
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UEONStatics::MarkCelComponent(Body);
	UEONStatics::MarkCelComponent(Head);
	BaseMaterial = Mat.Object;
}

void AEONNpc::Setup(float HeightPx, float WidthPx, const FLinearColor& BodyCol, const FLinearColor& HeadCol)
{
	// Corps : 70 % de la hauteur, tête : le reste (diamètre = 30 % de la hauteur). Pieds au niveau du sol de l'acteur.
	const float Hc = EON::PxToCm(HeightPx), Wc = EON::PxToCm(WidthPx);
	const float BodyH = Hc * 0.7f, HeadD = Hc * 0.3f;
	Body->SetRelativeScale3D(FVector(Wc / 100.f, Wc / 100.f, BodyH / 100.f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, BodyH * 0.5f));
	Head->SetRelativeScale3D(FVector(HeadD / 100.f));
	Head->SetRelativeLocation(FVector(0.f, 0.f, BodyH + HeadD * 0.5f));
	BodyBase = Body->GetRelativeLocation();
	HeadBase = Head->GetRelativeLocation();
	BodyC = BodyCol;
	HeadC = HeadCol;
	if (BaseMaterial)
	{
		BodyMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		HeadMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Body->SetMaterial(0, BodyMID);
		Head->SetMaterial(0, HeadMID);
	}
	ApplyColors(false);
}

void AEONNpc::SetDown(bool bDown)
{
	// Root est la racine de l'acteur : on tourne l'acteur, sans toucher à sa position (couché, il se relève de 3 px).
	if (bDown == bIsDown) { return; }
	bIsDown = bDown;
	SetActorRotation(bDown ? FRotator(0.f, 0.f, 90.f) : FRotator::ZeroRotator);
	AddActorWorldOffset(FVector(0.f, 0.f, EON::PxToCm(bDown ? 3.f : -3.f)));
}

void AEONNpc::ApplyColors(bool bWhite)
{
	if (BodyMID) { BodyMID->SetVectorParameterValue(TEXT("BC1"), bWhite ? FLinearColor::White : BodyC); BodyMID->SetVectorParameterValue(TEXT("EmisAdd"), bWhite ? FLinearColor(2.f, 2.f, 2.f) : FLinearColor::Black); }
	if (HeadMID) { HeadMID->SetVectorParameterValue(TEXT("BC1"), bWhite ? FLinearColor::White : HeadC); HeadMID->SetVectorParameterValue(TEXT("EmisAdd"), bWhite ? FLinearColor(2.f, 2.f, 2.f) : FLinearColor::Black); }
}

void AEONNpc::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FlashT -= DeltaSeconds;
	const bool bOn = FlashT > 0.f;
	if (bOn != bFlashOn) { bFlashOn = bOn; ApplyColors(bOn); }
	Age += DeltaSeconds;
	const float Dx = ShakePx > 0.f ? EON::PxToCm(FMath::RoundToFloat(FMath::Sin(Age * 30.f) * ShakePx * 2.f) * 0.5f) : 0.f;
	Body->SetRelativeLocation(BodyBase + FVector(Dx, 0.f, 0.f));
	Head->SetRelativeLocation(HeadBase + FVector(Dx, 0.f, 0.f));
}
