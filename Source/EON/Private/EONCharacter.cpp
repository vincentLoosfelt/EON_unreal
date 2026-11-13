#include "EONCharacter.h"
#include "EONSpriteComponent.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEONCharacter::AEONCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	const float Radius = EON::PxToCm(CollisionRadiusPx);
	const float HalfHeight = EON::PxToCm(HeightPx) * 0.5f;
	GetCapsuleComponent()->InitCapsuleSize(Radius, HalfHeight);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->MaxWalkSpeed = EON::PxToCm(WalkSpeedPx);
	// Le lissage de vitesse est fait à la main (k = min(1, dt×16), comme le proto) :
	// la CharacterMovement suit l'entrée lissée sans inertie propre.
	Move->MaxAcceleration = 100000.f;
	Move->BrakingDecelerationWalking = 100000.f;
	Move->GroundFriction = 8.f;
	Move->bConstrainToPlane = true;
	Move->SetPlaneConstraintNormal(FVector::UpVector);

	PlaceholderSprite = CreateDefaultSubobject<UEONSpriteComponent>(TEXT("PlaceholderSprite"));
	PlaceholderSprite->SetupAttachment(GetCapsuleComponent());
	PlaceholderSprite->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight));
	PlaceholderSprite->SpriteName = TEXT("jira_a0");
}

void AEONCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = EON::PxToCm(WalkSpeedPx);
}

void AEONCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	const float SpeedMul = T ? T->GetGameFeel().SpeedMul3D : 1.f;
	GetCharacterMovement()->MaxWalkSpeed = EON::PxToCm(WalkSpeedPx) * SpeedMul;

	const float K = FMath::Min(1.f, DeltaSeconds * 16.f);
	SmoothMove += (RawMove - SmoothMove) * K;
	// Écran : haut = -Y monde.
	AddMovementInput(FVector(SmoothMove.X, -SmoothMove.Y, 0.f), SmoothMove.Size());

	SetActorRotation(FRotator(0.f, FMath::RadiansToDegrees(AimAngle), 0.f));
	if (PlaceholderSprite)
	{
		PlaceholderSprite->bFlipX = FMath::Cos(AimAngle) < 0.f;
	}
}

FVector AEONCharacter::GetArmLocation() const
{
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	return Feet + FVector(0.f, 0.f, EON::PxToCm(EON::ArmHeightPx));
}
