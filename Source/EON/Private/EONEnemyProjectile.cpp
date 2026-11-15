#include "EONEnemyProjectile.h"
#include "EONTileRoom.h"
#include "EONTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Sphère M_Flat sans collision ni ombre (placeholder des tirs et des œufs). */
	UStaticMeshComponent* MakeFlatSphere(AActor* Owner, UStaticMeshComponent* Visual)
	{
		Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Visual->SetCastShadow(false);
		Visual->SetGenerateOverlapEvents(false);
		Visual->SetCanEverAffectNavigation(false);
		return Visual;
	}

	UMaterialInstanceDynamic* MakeFlatMid(UObject* Outer, UStaticMeshComponent* Visual, const FLinearColor& Col)
	{
		UMaterialInterface* Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_Flat.M_Flat"));
		if (!Flat) { return nullptr; }
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Flat, Outer);
		M->SetVectorParameterValue(TEXT("Color"), Col);
		Visual->SetMaterial(0, M);
		return M;
	}

	/** La sphère moteur fait 100 cm de diamètre : échelle pour un rayon donné en px. */
	FORCEINLINE float SphereScaleForRadiusPx(float RPx) { return EON::PxToCm(RPx) * 2.f / 100.f; }
}

// ---------------------------------------------------------------- AEONEnemyProjectile

AEONEnemyProjectile::AEONEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;   // piloté par AEONCombatDirector
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	RootComponent = Visual;
	MakeFlatSphere(this, Visual);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded()) { Visual->SetStaticMesh(SphereMesh.Object); }
}

void AEONEnemyProjectile::Init(AEONTileRoom* InRoom, const FVector2D& InScreenPx, const FVector2D& InVelPx, float InLife, bool bInBig, float InHeightPx)
{
	Room = InRoom;
	Pos = InScreenPx;
	Vel = InVelPx;
	Life = InLife;
	bBig = bInBig;
	HeightPx = InHeightPx;
	// Couleurs du proto : magenta #ff3d8b (méduse), gros orange #e8923a (Ikiryo). Rayon d'affichage 2 / 3 px.
	const FLinearColor Col = bBig ? FLinearColor(FColor(232, 146, 58)) : FLinearColor(FColor(255, 61, 139));
	Mid = MakeFlatMid(this, Visual, Col * 4.f);
	Visual->SetWorldScale3D(FVector(SphereScaleForRadiusPx(bBig ? 3.f : 2.f)));
	Place();
}

void AEONEnemyProjectile::Place()
{
	if (const AEONTileRoom* Rm = Room.Get())
	{
		// y écran du proto = sol − hauteur : on redescend au sol et on remonte en Z.
		SetActorLocation(Rm->RoomPxToWorld(FVector2D(Pos.X, Pos.Y + HeightPx), EON::PxToCm(HeightPx)));
	}
}

bool AEONEnemyProjectile::Step(float Dt)
{
	const AEONTileRoom* Rm = Room.Get();
	if (!Rm) { return true; }
	Pos += Vel * Dt;
	Life -= Dt;
	if (Life <= 0.f || Rm->SolidAtPx(Pos)) { return true; }
	Place();
	return false;
}

// ---------------------------------------------------------------- AEONEnemyEgg

AEONEnemyEgg::AEONEnemyEgg()
{
	PrimaryActorTick.bCanEverTick = false;   // piloté par AEONCombatDirector
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	RootComponent = Visual;
	MakeFlatSphere(this, Visual);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded()) { Visual->SetStaticMesh(SphereMesh.Object); }
}

void AEONEnemyEgg::Init(AEONTileRoom* InRoom, const FVector2D& InFromScreenPx, const FVector2D& InToPx)
{
	Room = InRoom;
	From = InFromScreenPx;
	To = InToPx;
	T = 0.f;
	// Œuf vert pâle (#d8f06a, couleur de ponte du proto), un peu ovale.
	Mid = MakeFlatMid(this, Visual, FLinearColor(FColor(216, 240, 106)) * 2.f);
	const float S = SphereScaleForRadiusPx(3.f);
	Visual->SetWorldScale3D(FVector(S, S, S * 1.3f));
	bool bDummy = false;
	Step(0.f, bDummy);
}

bool AEONEnemyEgg::Step(float Dt, bool& bHatch)
{
	bHatch = false;
	const AEONTileRoom* Rm = Room.Get();
	if (!Rm) { return true; }
	T += Dt;
	const float K = FMath::Clamp(T / FlyTime, 0.f, 1.f);
	// Vol en cloche : part de la hauteur de ponte (10 px), monte de 14 px au sommet, se pose au sol.
	const FVector2D Ground(FMath::Lerp(From.X, To.X, K), FMath::Lerp(From.Y + 10.f, To.Y, K));
	const float Hpx = 10.f * (1.f - K) + FMath::Sin(K * PI) * 14.f;
	FVector Loc = Rm->RoomPxToWorld(Ground, EON::PxToCm(Hpx + 3.f));
	if (K >= 1.f)
	{
		// Au sol : tremble de plus en plus avant d'éclore
		const float Q = (T - FlyTime) / (HatchTime - FlyTime);
		Loc.X += EON::PxToCm(FMath::Sin(T * 30.f) * Q);
	}
	SetActorLocation(Loc);
	if (T < HatchTime) { return false; }
	// Éclosion seulement sur du sol (pas dans le vide ni un mur) ; sinon l'œuf est perdu.
	bHatch = Rm->TileAtPx(To) == EEONTile::Floor;
	return true;
}
