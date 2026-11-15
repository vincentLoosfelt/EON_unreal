#include "EONTrainingDummy.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Les tirs partent du bras (17 px de haut) : la cible est centrée à cette hauteur.
	const float CenterZ = EON::PxToCm(EON::ArmHeightPx);
	const float HitRadius = EON::PxToCm(7.f); // r du proto
	const FLinearColor Lilac(0.69f, 0.66f, 0.87f, 1.f);
}

AEONTrainingDummy::AEONTrainingDummy()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(Root);
	Capsule->InitCapsuleSize(HitRadius, HitRadius);
	Capsule->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));
	Capsule->SetCollisionProfileName(TEXT("Pawn")); // bloque les tirs et Jira

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded()) { Body->SetStaticMesh(CubeMesh.Object); }
	Body->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f)); // 12 px
	Body->SetRelativeLocation(FVector(0.f, 0.f, CenterZ));

	Post = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CylMesh.Succeeded()) { Post->SetStaticMesh(CylMesh.Object); }
	Post->SetRelativeScale3D(FVector(0.1f, 0.1f, CenterZ / 100.f));
	Post->SetRelativeLocation(FVector(0.f, 0.f, CenterZ * 0.5f));
}

void AEONTrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_Flat.M_Flat")))
	{
		BodyMid = UMaterialInstanceDynamic::Create(Flat, this);
		BodyMid->SetVectorParameterValue(TEXT("Color"), Lilac);
		Body->SetMaterial(0, BodyMid);
		Post->SetMaterial(0, Flat);
	}
}

FVector AEONTrainingDummy::GetAimPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, CenterZ);
}

void AEONTrainingDummy::ApplyShot(float Damage, FVector Dir, float KnockMul, FVector /*ImpactPoint*/)
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	const FEONGameFeelRow F = T ? T->GetGameFeel() : FEONGameFeelRow();

	// e.wob += dx*6*knock*kmul ; e.flash = hitFlash ? 0.07 : 0 ; e.dmgAcc += dmg ; e.dmgT = 1.2
	WobCm += EON::PxToCm(Dir.X * 6.f * F.Knock * KnockMul);
	FlashT = F.HitFlash ? 0.07f : 0.f;
	DmgAcc += Damage;
	DmgT = 1.2f;

	if (F.DmgNums)
	{
		FNum N;
		N.Loc = GetActorLocation() + FVector(FMath::FRandRange(-0.5f, 0.5f) * EON::PxToCm(6.f), 0.f, CenterZ + HitRadius + EON::PxToCm(8.f));
		N.VelZ = EON::PxToCm(38.f);
		N.Life = 0.6f;
		N.Text = FString::SanitizeFloat(FMath::RoundToFloat(Damage * 10.f) / 10.f);
		N.Col = Damage >= 3.f ? FColor(255, 194, 61) : FColor::White;
		Nums.Add(N);
	}

	// P.ult += dmg*0.8 : la jauge d'ultime arrive au jalon 4.
	if (Damage >= 3.f)
	{
		if (UEONTuningSubsystem* Tun = UEONTuningSubsystem::Get(this)) { Tun->RequestHitstop(F.Hitstop * 0.3f); }
	}
}

void AEONTrainingDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// e.wob += -e.wob*dt*10 ; e.dmgT -= dt ; si dmgT <= 0, dmgAcc = 0
	WobCm += -WobCm * FMath::Min(1.f, DeltaSeconds * 10.f);
	FlashT -= DeltaSeconds;
	DmgT -= DeltaSeconds;
	if (DmgT <= 0.f) { DmgAcc = 0.f; }

	Body->SetRelativeLocation(FVector(WobCm, 0.f, CenterZ));
	if (BodyMid)
	{
		BodyMid->SetVectorParameterValue(TEXT("Color"), FlashT > 0.f ? FLinearColor::White : Lilac);
	}

#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World) { return; }

	for (int32 i = Nums.Num() - 1; i >= 0; --i)
	{
		FNum& N = Nums[i];
		N.Life -= DeltaSeconds;
		if (N.Life <= 0.f) { Nums.RemoveAtSwap(i); continue; }
		N.Loc.Z += N.VelZ * DeltaSeconds;
		DrawDebugString(World, N.Loc, N.Text, nullptr, N.Col, 0.f, false, 1.4f);
	}
	if (DmgAcc > 0.f)
	{
		const FVector P = GetActorLocation() + FVector(0.f, 0.f, CenterZ + HitRadius + EON::PxToCm(14.f));
		DrawDebugString(World, P, FString::Printf(TEXT("DPS %.1f"), FMath::RoundToFloat(DmgAcc * 10.f) / 10.f), nullptr, FColor(143, 138, 189), 0.f, false, 1.2f);
	}
#endif
}
