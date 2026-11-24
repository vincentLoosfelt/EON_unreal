#include "EONProjectile.h"
#include "EONStatics.h"
#include "EONFx.h"
#include "EONTrainingDummy.h"
#include "EONEnemy.h"
#include "EONTypes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

int32 AEONProjectile::StatShots = 0;
int32 AEONProjectile::StatHits = 0;
int32 AEONProjectile::StatWalls = 0;

AEONProjectile::AEONProjectile()
{
	PrimaryActorTick.bCanEverTick = true;   // traînée (JIR-V13)

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->InitSphereRadius(EON::PxToCm(1.f));
	// Bloque murs, décor et créatures ; ignore les autres projectiles (WorldDynamic).
	Sphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	Sphere->SetGenerateOverlapEvents(false);
	RootComponent = Sphere;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Sphere);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	UEONStatics::MarkCelComponent(Visual, false);   // projectile : pas de trait (Vincent, 24/11)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded()) { Visual->SetStaticMesh(SphereMesh.Object); }
	// Trait allongé dans le sens du tir : 0,5 m de long, 0,1 m de large.
	Visual->SetRelativeScale3D(FVector(0.5f, 0.1f, 0.1f));

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Sphere;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	// Chaque contact passe par OnBounce : on y décide (dégâts, perforation, ricochet ou fin de course).
	Movement->bShouldBounce = true;
	Movement->Bounciness = 1.f;
	Movement->Friction = 0.f;
	Movement->BounceVelocityStopSimulatingThreshold = 0.f;
	Movement->InitialSpeed = 0.f; // la vitesse vient de Setup()
	// Setup() donne une vitesse monde : sans ça, elle est tournée une 2e fois par l'orientation de la balle (angle doublé).
	Movement->bInitialVelocityInLocalSpace = false;
	Movement->MaxSpeed = 0.f;     // 0 = pas de plafond
}

void AEONProjectile::Setup(const FVector& InVelocityCm, float InLifeSeconds, float InDamage, float InRadiusPx, const FLinearColor& InColor)
{
	Movement->Velocity = InVelocityCm;
	ShotDir = InVelocityCm.GetSafeNormal2D();
	SpeedCm = InVelocityCm.Size2D();
	LifeSeconds = InLifeSeconds;
	Damage = InDamage;
	Color = InColor;
	Sphere->SetSphereRadius(EON::PxToCm(InRadiusPx));
	SizePx = InRadiusPx;
}

void AEONProjectile::BeginPlay()
{
	Super::BeginPlay();
	++StatShots;

	if (AActor* Shooter = GetInstigator())
	{
		Sphere->IgnoreActorWhenMoving(Shooter, true);
	}
	Movement->OnProjectileStop.AddDynamic(this, &AEONProjectile::OnStop);
	Movement->OnProjectileBounce.AddDynamic(this, &AEONProjectile::OnBounce);
	SetLifeSpan(FMath::Max(0.01f, LifeSeconds));

	if (UMaterialInterface* Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_Flat.M_Flat")))
	{
		Mid = UMaterialInstanceDynamic::Create(Flat, this);
		Mid->SetVectorParameterValue(TEXT("Color"), Color);
		Visual->SetMaterial(0, Mid);
	}
	// JIR-V13 : traînée du proto (trait de min(6, v × 0,02) + taille px derrière la balle, épaisseur = taille) et halo.
	// Plans à plat qui suivent la balle ; la traînée grandit depuis le canon, jamais plus que le chemin parcouru.
	StartLoc = GetActorLocation();
	const float S = FMath::Max(1.f, SizePx);
	TrailMaxCm = EON::PxToCm(FMath::Min(6.f, EON::CmToPx(SpeedCm) * 0.02f) + S);
	TrailWCm = EON::PxToCm(S);
	static TStrongObjectPtr<UStaticMesh> Plane;
	if (!Plane.IsValid()) { Plane.Reset(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"))); }
	auto MakePlane = [this](UMaterialInstanceDynamic* M) {
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Plane.Get());
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(Sphere);
		C->SetMaterial(0, M);
		C->RegisterComponent();
		return C;
	};
	Trail = MakePlane(EONFx::Glow(this, EONFx::EForme::Trainee, Color, 2.5f));
	Halo = MakePlane(EONFx::Glow(this, EONFx::EForme::Rond, Color, 1.2f));
	const float Hd = EON::PxToCm(5.f * S);
	Halo->SetRelativeScale3D(FVector(Hd / 100.f, Hd / 100.f, 1.f));
	Trail->SetRelativeScale3D(FVector(0.001f, TrailWCm / 100.f, 1.f));
}

void AEONProjectile::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Trail) { return; }
	const float Len = FMath::Min(TrailMaxCm, FVector::Dist2D(GetActorLocation(), StartLoc));
	Trail->SetRelativeScale3D(FVector(FMath::Max(0.001f, Len / 100.f), TrailWCm / 100.f, 1.f));
	Trail->SetRelativeLocation(FVector(-Len * 0.5f, 0.f, 0.f));
}

void AEONProjectile::OnBounce(const FHitResult& Hit, const FVector& ImpactVelocity)
{
	if (IsActorBeingDestroyed()) { return; }
	AActor* A = Hit.GetActor();
	if (AEONTrainingDummy* Dummy = Cast<AEONTrainingDummy>(A)) { Dummy->ApplyShot(Damage, ShotDir, 1.f, Hit.ImpactPoint); Destroy(); return; }
	if (AEONEnemy* Enemy = Cast<AEONEnemy>(A))
	{
		Enemy->ApplyHit(Damage, ShotDir, 1.f);
		++StatHits;
		if (PierceLeft > 0)
		{
			// Perforation : on traverse cet ennemi et on garde la trajectoire.
			--PierceLeft;
			Sphere->IgnoreActorWhenMoving(Enemy, true);
			Movement->Velocity = ShotDir * SpeedCm;
			return;
		}
		Destroy();
		return;
	}
	if (BounceLeft > 0)
	{
		// Ricochet : la vitesse réfléchie est gardée, à plat et à vitesse constante.
		--BounceLeft;
		// Ricochet : un point blanc immobile (0,15 s).
		EONFx::Gerbe(this, GetActorLocation(), 1, FLinearColor::White, 0.f, 0.15f, 1.f, 0.f);
		FVector V = Movement->Velocity; V.Z = 0.f;
		ShotDir = V.GetSafeNormal2D();
		Movement->Velocity = ShotDir * SpeedCm;
		return;
	}
	++StatWalls;
	WallSparks();
	if (StatWalls <= 40)
	{
		UE_LOG(LogTemp, Display, TEXT("EON balle arretee par %s / %s en %s (z %.0f, n %s)"), A ? *A->GetName() : TEXT("-"),
			Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("-"), *Hit.ImpactPoint.ToCompactString(), GetActorLocation().Z, *Hit.ImpactNormal.ToCompactString());
	}
	Destroy();
}

void AEONProjectile::WallSparks()
{
	// Mort au mur : 4 éclats couleur du tir, renvoyés vers l'arrière (-v × 0,2 ± 35 px/s), 0,2 s.
	EONFx::GerbeDir(this, GetActorLocation(), -ShotDir, 4, Color, EON::CmToPx(SpeedCm) * 0.2f + 35.f, 0.2f);
}

void AEONProjectile::OnStop(const FHitResult& Hit)
{
	if (AEONTrainingDummy* Dummy = Cast<AEONTrainingDummy>(Hit.GetActor()))
	{
		// Movement->Velocity est déjà remis à zéro quand OnProjectileStop part : on garde la direction du tir.
		Dummy->ApplyShot(Damage, ShotDir, 1.f, Hit.ImpactPoint);
	}
	else if (AEONEnemy* Enemy = Cast<AEONEnemy>(Hit.GetActor()))
	{
		// hitEnemy(e, b.dmg, dx, dy) : recul dans le sens du tir
		Enemy->ApplyHit(Damage, ShotDir, 1.f);
		++StatHits;
	}
	else
	{
		++StatWalls;
		WallSparks();
		if (StatWalls <= 25)
		{
			UE_LOG(LogTemp, Display, TEXT("EON balle arretee par %s / %s en %s (z depart %.0f)"), Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("-"),
				Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("-"), *Hit.ImpactPoint.ToCompactString(), GetActorLocation().Z);
		}
	}
	Destroy();
}
