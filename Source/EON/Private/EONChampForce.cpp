#include "EONChampForce.h"
#include "EONProjectile.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AEONChampForce::AEONChampForce()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Centre")));
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> C(TEXT("/Game/EON/Materials/MPC_EON_Champ.MPC_EON_Champ"));
	if (C.Succeeded()) { Collection = C.Object; }
}

void AEONChampForce::Impact(const FVector& P)
{
	if (!Collection) { return; }
	const FVector C = GetActorLocation();
	const FVector S = C + (P - C).GetSafeNormal() * Radius;
	const FName Slot(*FString::Printf(TEXT("Impact%d"), NextSlot));
	NextSlot = (NextSlot + 1) % 4;
	UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, Slot, FLinearColor(S.X, S.Y, S.Z, GetWorld()->GetTimeSeconds()));
}

void AEONChampForce::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Collection) { return; }
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TEXT("Now"), GetWorld()->GetTimeSeconds());
	if (const APawn* J = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TEXT("Jira"), FLinearColor(J->GetActorLocation()));
	}

	// Balles qui traversent la sphère (dans un sens ou dans l'autre)
	const FVector C = GetActorLocation();
	for (TActorIterator<AEONProjectile> It(GetWorld()); It; ++It)
	{
		const FVector P = It->GetActorLocation();
		const bool bIn = FVector::Dist(P, C) < Radius;
		if (bool* Was = Inside.Find(*It))
		{
			if (*Was != bIn && P.Z > MinZ) { Impact(P); }
			*Was = bIn;
		}
		else { Inside.Add(*It, bIn); }
	}
	for (auto It = Inside.CreateIterator(); It; ++It) { if (!It->Key.IsValid()) { It.RemoveCurrent(); } }

	// Impulsion au hasard sur la calotte
	NextAmbient -= DeltaSeconds;
	if (NextAmbient <= 0.f)
	{
		NextAmbient = FMath::FRandRange(AmbientMin, AmbientMax);
		FVector D = FMath::VRand();
		D.Z = FMath::Abs(D.Z) + 0.6f;
		Impact(C + D.GetSafeNormal() * Radius);
	}
}
