#include "EONRoomBounds.h"
#include "EONTypes.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AEONRoomBounds::AEONRoomBounds()
{
	PrimaryActorTick.bCanEverTick = false;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->ShapeColor = FColor(62, 242, 224);
	Box->SetLineThickness(4.f);
	RootComponent = Box;
}

void AEONRoomBounds::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Le pivot est le centre de la salle, au niveau du sol.
	Box->SetBoxExtent(FVector(SizeTiles.X * EON::CmPerTile * 0.5f, SizeTiles.Y * EON::CmPerTile * 0.5f, 300.f));
	Box->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
}

FBox2D AEONRoomBounds::GetRect2D() const
{
	const FVector C = GetActorLocation();
	const FVector2D Half(SizeTiles.X * EON::CmPerTile * 0.5f, SizeTiles.Y * EON::CmPerTile * 0.5f);
	return FBox2D(FVector2D(C.X, C.Y) - Half, FVector2D(C.X, C.Y) + Half);
}

AEONRoomBounds* AEONRoomBounds::FindRoomAt(const UObject* WorldContext, FVector Location)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) { return nullptr; }
	const FVector2D P(Location.X, Location.Y);
	for (TActorIterator<AEONRoomBounds> It(World); It; ++It)
	{
		if (It->GetRect2D().IsInside(P)) { return *It; }
	}
	return nullptr;
}
