#include "EONMembre.h"
#include "EONFx.h"
#include "EONStatics.h"
#include "AnimationRuntime.h"
#include "Components/BoxComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

AEONMembre::AEONMembre()
{
	PrimaryActorTick.bCanEverTick = true;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Boite"));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetHiddenInGame(true);
	RootComponent = Box;
}

FVector AEONMembre::RefToWorld(const USkeletalMeshComponent* Src, FName Bone, const FVector& Ref)
{
	if (!Src || !Src->GetSkeletalMeshAsset()) { return FVector::ZeroVector; }
	const int32 Bi = Src->GetBoneIndex(Bone);
	if (Bi == INDEX_NONE) { return Src->GetComponentTransform().TransformPosition(Ref); }
	const FTransform RefCs = FAnimationRuntime::GetComponentSpaceTransformRefPose(Src->GetSkeletalMeshAsset()->GetRefSkeleton(), Bi);
	return Src->GetBoneTransform(Bi).TransformPosition(RefCs.InverseTransformPosition(Ref));
}

void AEONMembre::Init(USkeletalMeshComponent* Src, const TArray<UMaterialInstanceDynamic*>& SrcMids, int32 K, const FEONMembreDef& Def,
	const FVector& ShotDir, const FLinearColor& InSang)
{
	Sang = InSang;
	const FName Bone(Def.Bone);
	const FVector Jw = RefToWorld(Src, Bone, Def.J);
	const FVector Ew = RefToWorld(Src, Bone, Def.J + Def.Axe * Def.L);
	const FVector X = (Ew - Jw).GetSafeNormal();
	const float S = Src->GetComponentScale().X;
	RayonCm = Def.R * S;
	// Boîte le long du membre (un peu plus mince que lui : il repose sur sa chair, pas sur le vide autour).
	Box->SetBoxExtent(FVector(FMath::Max(2.f, (Ew - Jw).Size() * 0.5f), RayonCm * 0.7f, RayonCm * 0.7f), false);
	SetActorLocationAndRotation((Jw + Ew) * 0.5f, FRotationMatrix::MakeFromX(X).Rotator());
	Pose = NewObject<UPoseableMeshComponent>(this);
	Pose->SetSkinnedAssetAndUpdate(Src->GetSkeletalMeshAsset());
	Pose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pose->SetupAttachment(Box);
	Pose->RegisterComponent();
	Pose->SetWorldTransform(Src->GetComponentTransform());
	Pose->CopyPoseFromSkeletalComponent(Src);
	for (int32 I = 0; I < Pose->GetNumMaterials() && I < SrcMids.Num(); ++I)
	{
		UMaterialInstanceDynamic* From = SrcMids[I];
		if (!From) { continue; }
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(From->Parent, this);
		M->CopyParameterOverrides(From);
		M->SetScalarParameterValue(TEXT("Morceau"), (float)(K + 1));
		M->SetVectorParameterValue(TEXT("FlashCouleur"), FLinearColor(1.f, 1.f, 1.f, 0.f));
		Pose->SetMaterial(I, M);
		Mids.Add(M);
	}
	UEONStatics::MarkCelComponent(Pose);   // celluloïd
	// Rigid body : ne heurte que le décor statique (sol, murs), pas les persos ni les balles.
	Box->SetCollisionObjectType(ECC_PhysicsBody);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetSimulatePhysics(true);
	Box->SetMassOverrideInKg(NAME_None, 4.f, true);
	Box->SetAngularDamping(0.6f);
	Box->SetLinearDamping(0.2f);
	const FVector D = ShotDir.GetSafeNormal2D();
	// Poussée modérée (W90 : à 180–320 cm/s les morceaux sortaient de la dalle) : ils retombent près du corps.
	Box->AddImpulse(D * FMath::FRandRange(50.f, 110.f) + FVector(0.f, 0.f, FMath::FRandRange(130.f, 210.f)) + X * 40.f, NAME_None, true);
	Box->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-0.5f, 0.5f)) * 500.f, NAME_None, true);
	// Giclée qui sort du moignon du morceau, à l'opposé du bout.
	EONFx::Giclee(this, Jw, -X, FMath::Max(40.f, RayonCm * 4.f), Sang, 0.3f, RayonCm);
}

void AEONMembre::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;
	UWorld* W = GetWorld();
	// Tache au sol quand le morceau touche terre.
	if (!bTache && Age > 0.15f && W)
	{
		FHitResult H;
		const FVector P = GetActorLocation();
		FCollisionQueryParams Q(SCENE_QUERY_STAT(EONMembreSol), false, this);
		if (W->LineTraceSingleByChannel(H, P, P - FVector(0.f, 0.f, RayonCm + 15.f), ECC_Visibility, Q) && H.ImpactNormal.Z > 0.7f)
		{
			bTache = true;
			EONFx::TacheSol(this, H.ImpactPoint, FMath::FRandRange(0.f, 360.f), RayonCm * 5.f, Sang, 0.f, Life - Age);
		}
	}
	// Dernière seconde : physique coupée, le morceau s'enfonce dans le sol.
	if (Age > Life - 1.f)
	{
		if (Box->IsSimulatingPhysics()) { Box->SetSimulatePhysics(false); Box->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
		AddActorWorldOffset(FVector(0.f, 0.f, -RayonCm * 3.f * Dt));
	}
	if (Age >= Life || GetActorLocation().Z < -5000.f) { Destroy(); }
}
