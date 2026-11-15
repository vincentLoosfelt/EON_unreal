#include "EONDoorComponent.h"
#include "EONTileGrid.h"

UEONDoorComponent::UEONDoorComponent()
{
	ArrowSize = 2.5f;
	ArrowLength = 120.f;
	bHiddenInGame = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyLook();
}

uint8 UEONDoorComponent::DirFromYaw(float Yaw)
{
	const float R = FMath::DegreesToRadians(Yaw);
	const float C = FMath::Cos(R), S = FMath::Sin(R);
	if (FMath::Abs(C) >= FMath::Abs(S)) { return C > 0.f ? EONDoor::E : EONDoor::W; }
	return S > 0.f ? EONDoor::S : EONDoor::N;
}

void UEONDoorComponent::ApplyLook()
{
	// Cyan = sortie de la salle ; orange = porte intérieure.
	ArrowColor = bExterieure ? FColor(62, 242, 224) : FColor(255, 150, 40);
}

void UEONDoorComponent::OnRegister()
{
	ApplyLook();
	Super::OnRegister();
}

#if WITH_EDITOR
void UEONDoorComponent::PostEditChangeProperty(FPropertyChangedEvent& E)
{
	ApplyLook();
	MarkRenderStateDirty();
	Super::PostEditChangeProperty(E);
}
#endif
