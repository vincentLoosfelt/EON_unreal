#include "EONPedestal.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONCombatDirector.h"
#include "EONItems.h"
#include "EONTileRoom.h"
#include "EONTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AEONPedestal::FOnTaken AEONPedestal::OnTaken;

AEONPedestal::AEONPedestal()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Socle"));
	RootComponent = Base;
	Base->SetStaticMesh(Cyl.Object);
	Base->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Base->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.3f));
	Gem = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Objet"));
	Gem->SetupAttachment(Base);
	Gem->SetStaticMesh(Cube.Object);
	Gem->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Gem->SetUsingAbsoluteScale(true);
	Gem->SetWorldScale3D(FVector(0.28f));
}

AEONPedestal* AEONPedestal::Spawn(UWorld* World, AEONTileRoom* Room, FName Item, const FVector2D& Px, int32 Price, int32 Slot)
{
	const FEONItemDef* Def = EONItems::Find(Item);
	if (!World || !Room || !Def) { return nullptr; }
	AEONPedestal* P = World->SpawnActor<AEONPedestal>(AEONPedestal::StaticClass(), Room->RoomPxToWorld(Px, 15.f), FRotator::ZeroRotator);
	if (!P) { return nullptr; }
	P->Item = Item;
	P->Price = Price;
	P->Slot = Slot;
	P->Px = Px;
	P->Room = Room;
	P->T = FMath::FRand() * 6.f;
	if (UMaterialInterface* Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_Flat.M_Flat")))
	{
		UMaterialInstanceDynamic* MB = UMaterialInstanceDynamic::Create(Flat, P);
		MB->SetVectorParameterValue(TEXT("Color"), FLinearColor(FColor(40, 36, 70)));
		P->Base->SetMaterial(0, MB);
		UMaterialInstanceDynamic* MG = UMaterialInstanceDynamic::Create(Flat, P);
		MG->SetVectorParameterValue(TEXT("Color"), EONItems::Color(Def->Rar) * 4.f);
		P->Gem->SetMaterial(0, MG);
	}
	return P;
}

void AEONPedestal::ClearAll(UWorld* World)
{
	for (TActorIterator<AEONPedestal> It(World); It; ++It) { It->Destroy(); }
}

AEONPedestal* AEONPedestal::Nearest(UWorld* World, const FVector2D& JiraPx, float RangePx)
{
	AEONPedestal* Best = nullptr;
	float BestD = RangePx;
	for (TActorIterator<AEONPedestal> It(World); It; ++It)
	{
		if (It->IsActorBeingDestroyed()) { continue; }
		const float D = FVector2D::Distance(JiraPx, It->Px);
		if (D < BestD) { BestD = D; Best = *It; }
	}
	return Best;
}

void AEONPedestal::Tick(float Dt)
{
	Super::Tick(Dt);
	T += Dt;
	Cool = FMath::Max(0.f, Cool - Dt);
	AEONTileRoom* Rm = Room.Get();
	AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Rm || !J) { return; }
	Gem->SetRelativeLocation(FVector(0.f, 0.f, 160.f + 25.f * FMath::Sin(T * 2.f)));
	Gem->AddLocalRotation(FRotator(0.f, 60.f * Dt, 0.f));
	if (J->IsFallingIntoVoid() || J->IsDead()) { return; }
	const FVector2D JP = Rm->WorldToRoomPx(J->GetActorLocation());
	if (FVector2D::Distance(JP, Px + FVector2D(0.f, 4.f)) >= 9.f) { return; }
	AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld());
	if (Price > 0)
	{
		// Chez le marchand on paie, et on peut racheter tant qu'il reste du scrap.
		if (Cool > 0.f) { return; }
		if (J->GetScrap() < Price)
		{
			Cool = 1.1f;
			if (D) { D->AddNum(GetActorLocation() + FVector(0, 0, 150.f), TEXT("Pas assez"), FColor(255, 61, 85), 1.f, -26.f); }
			return;
		}
		J->AddScrap(-Price);
		if (D) { D->AddNum(GetActorLocation() + FVector(0, 0, 150.f), FString::Printf(TEXT("-%d"), Price), FColor(255, 194, 61), 1.f, -30.f); }
		J->TakeItem(Item);
		OnTaken.Broadcast(this, true);
		Destroy();
		return;
	}
	J->TakeItem(Item);
	OnTaken.Broadcast(this, false);
	ClearAll(GetWorld());
}
