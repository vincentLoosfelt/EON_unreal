#include "EONPickup.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONCombatDirector.h"
#include "EONMeta.h"
#include "EONUI.h"
#include "EONTileRoom.h"
#include "EONTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AEONPickup::AEONPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visuel"));
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Mesh->SetStaticMesh(Sph.Object);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetMobility(EComponentMobility::Movable);
}

AEONPickup* AEONPickup::Spawn(UWorld* World, AEONTileRoom* Room, EEONPickup Type, const FVector2D& Px, const FVector2D& VelPx, float V, float T0)
{
	if (!World || !Room) { return nullptr; }
	AEONPickup* P = World->SpawnActor<AEONPickup>(AEONPickup::StaticClass(), Room->RoomPxToWorld(Px, 20.f), FRotator::ZeroRotator);
	if (!P) { return nullptr; }
	P->Type = Type;
	P->Value = V;
	P->Room = Room;
	P->Px = Px;
	P->Vel = VelPx;
	P->T = T0;
	// Couleurs du proto : ADN vert, scrap ambre, énergie cyan, cœur rouge. Formes provisoires.
	const FLinearColor C = Type == EEONPickup::Adn ? FLinearColor(FColor(138, 240, 122)) : Type == EEONPickup::Scrap ? FLinearColor(FColor(255, 194, 61))
		: Type == EEONPickup::Energy ? FLinearColor(FColor(62, 242, 224)) : Type == EEONPickup::Donnee ? FLinearColor(FColor(154, 216, 255)) : FLinearColor(FColor(255, 61, 85));
	const float S = Type == EEONPickup::Adn ? 0.12f + 0.03f * V : 0.2f;
	P->Mesh->SetWorldScale3D(FVector(S));
	if (UMaterialInterface* Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_Flat.M_Flat")))
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Flat, P);
		M->SetVectorParameterValue(TEXT("Color"), C * 3.f);
		P->Mesh->SetMaterial(0, M);
	}
	return P;
}

void AEONPickup::DropADN(UWorld* World, AEONTileRoom* Room, const FVector2D& Px, int32 V)
{
	while (V > 0)
	{
		const int32 K = FMath::Min(3, V);
		V -= K;
		Spawn(World, Room, EEONPickup::Adn, Px, FVector2D(FMath::FRandRange(-45.f, 45.f), FMath::FRandRange(-45.f, 45.f)), K);
	}
}

bool AEONPickup::AnyAdn(UWorld* World)
{
	for (TActorIterator<AEONPickup> It(World); It; ++It) { if (It->Type == EEONPickup::Adn && !It->IsActorBeingDestroyed()) { return true; } }
	return false;
}

void AEONPickup::ClearAll(UWorld* World, bool bCreditAdn)
{
	AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	for (TActorIterator<AEONPickup> It(World); It; ++It)
	{
		if (bCreditAdn && J && It->Type == EEONPickup::Adn) { J->GainADN(It->Value); }
		It->Destroy();
	}
}

void AEONPickup::Tick(float Dt)
{
	Super::Tick(Dt);
	AEONTileRoom* Rm = Room.Get();
	AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Rm || !J) { Destroy(); return; }
	T += Dt;
	const FVector2D JP = Rm->WorldToRoomPx(J->GetActorLocation()) + FVector2D(0.f, -3.f);
	const FVector2D Dv = JP - Px;
	const float D = Dv.Size();
	const bool bFall = J->IsFallingIntoVoid();
	const bool bHeartFull = Type == EEONPickup::Heart && J->GetHP() >= J->GetMaxHP();
	if (Type == EEONPickup::Adn && T > 0.35f && !bFall && D > 0.1f)
	{
		// L'ADN est aspiré vers Jira où qu'elle soit dans la salle.
		const float K = FMath::Min(1.f, (T - 0.35f) * 3.f);
		Vel += Dv / D * 1800.f * K * Dt;
		Vel *= 1.f - FMath::Min(1.f, Dt * 3.f);
	}
	else if (D < J->GetMagnetPx() && T > 0.3f && !bFall && !bHeartFull)
	{
		// Aimant dirigé : la vitesse s'oriente vers Jira (sinon le ramassable tourne autour).
		const float Sp = 140.f + FMath::Max(0.f, 40.f - D) * 7.f, K = FMath::Min(1.f, Dt * 14.f);
		Vel += (Dv / FMath::Max(D, 0.1f) * Sp - Vel) * K;
	}
	else
	{
		Vel *= 1.f - FMath::Min(1.f, Dt * 4.f);
	}
	Px += Vel * Dt;
	// Un ramassable ordinaire immobile au-dessus du vide finit par tomber (pas l'ADN).
	if (Type != EEONPickup::Adn && Type != EEONPickup::Donnee && T > 0.8f && D >= 40.f && Vel.Size() < 10.f && Rm->PitAtPx(Px)) { Destroy(); return; }
	if (D < 8.f && T > 0.3f && !bHeartFull)
	{
		switch (Type)
		{
		case EEONPickup::Adn: J->GainADN(Value); break;
		case EEONPickup::Scrap: J->AddScrap(1); break;
		case EEONPickup::Energy: J->PickupEnergy(14.f); break;
		case EEONPickup::Heart: J->Heal(1); break;
		case EEONPickup::Donnee:
		{
			// +v × donM, au moins 1 ; le premier ramassage déclenche le dialogue « donnees ».
			const int32 Gn = FMath::Max(1, FMath::RoundToInt(Value * J->GetDonM()));
			if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this))
			{
				MS->AddDonnees(Gn);
				if (MS->M() && !MS->M()->bDonHint)
				{
					MS->M()->bDonHint = true;
					MS->Save();
					if (UEONUISubsystem* UI = UEONUISubsystem::Get(this)) { UI->Say(TEXT("donnees")); }
				}
			}
			if (AEONCombatDirector* Dir = AEONCombatDirector::Get(GetWorld()))
			{
				Dir->Donnees += Gn;
				Dir->AddNum(J->GetActorLocation() + FVector(0.f, 0.f, 160.f), FString::Printf(TEXT("+%d donnees"), Gn), FColor(154, 216, 255), 0.9f, -30.f);
			}
			break;
		}
		}
		Destroy();
		return;
	}
	// Petit flottement au-dessus du sol.
	SetActorLocation(Rm->RoomPxToWorld(Px, 18.f + 6.f * FMath::Sin(T * 5.f + Value)));
}
