#include "EONSlidingDoor.h"
#include "EONEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarPortesForce(TEXT("eon.portes.force"), -1,
		TEXT("Portes coulissantes : -1 auto (fermées s'il reste un ennemi dans la salle), 0 forcées fermées, 1 forcées ouvertes"));
}

AEONSlidingDoor::AEONSlidingDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	auto Part = [this](const TCHAR* Name, const TCHAR* Path)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		ConstructorHelpers::FObjectFinder<UStaticMesh> M(Path);
		if (M.Succeeded())
		{
			C->SetStaticMesh(M.Object);
		}
		C->SetCollisionProfileName(TEXT("BlockAll"));
		return C;
	};
	Cadre = Part(TEXT("Cadre"), TEXT("/Game/EON/Unity/import/fbx/enviro/porte_pieces/porte_Cadre.porte_Cadre"));
	SetRootComponent(Cadre);
	VantailG = Part(TEXT("VantailG"), TEXT("/Game/EON/Unity/import/fbx/enviro/porte_pieces/porte_VantailG.porte_VantailG"));
	VantailD = Part(TEXT("VantailD"), TEXT("/Game/EON/Unity/import/fbx/enviro/porte_pieces/porte_VantailD.porte_VantailD"));
	VantailG->SetupAttachment(Cadre);
	VantailD->SetupAttachment(Cadre);
	VantailG->SetMobility(EComponentMobility::Movable);
	VantailD->SetMobility(EComponentMobility::Movable);
}

void AEONSlidingDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyOpening();   // ouverte par défaut, y compris dans l'éditeur
}

void AEONSlidingDoor::BeginPlay()
{
	Super::BeginPlay();
	ApplyOpening();
}

void AEONSlidingDoor::ApplyOpening()
{
	// Les vantaux sont modelés en position fermée : on les décale vers l'extérieur (gauche -X, droit +X).
	const float D = Opening * OpenDistance;
	VantailG->SetRelativeLocation(FVector(-D, 0.f, 0.f));
	VantailD->SetRelativeLocation(FVector(D, 0.f, 0.f));
}

bool AEONSlidingDoor::EnemyInRoom() const
{
	const FVector P = GetActorLocation();
	for (TActorIterator<AEONEnemy> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist2D(It->GetActorLocation(), P) < RoomRadius)
		{
			return true;
		}
	}
	return false;
}

void AEONSlidingDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	NextCheck -= DeltaSeconds;
	if (NextCheck <= 0.f)
	{
		NextCheck = CheckInterval;
		const int32 Force = CVarPortesForce.GetValueOnGameThread();
		bWantOpen = Force >= 0 ? Force == 1 : ForceState >= 0 ? ForceState == 1 : !EnemyInRoom();
	}
	const float Target = bWantOpen ? 1.f : 0.f;
	if (Opening != Target && OpenDistance > 0.f)
	{
		Opening = FMath::FInterpConstantTo(Opening, Target, DeltaSeconds, OpenSpeed / OpenDistance);
		ApplyOpening();
	}
}
