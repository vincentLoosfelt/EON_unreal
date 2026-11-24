#include "EONFx.h"
#include "EONTypes.h"
#include "EONStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"

namespace
{
	UNiagaraSystem* Sys(int32 Which)
	{
		static const TCHAR* Paths[] = {
			TEXT("/Game/EON/VFX/NS_EON_Gerbe.NS_EON_Gerbe"),
			TEXT("/Game/EON/VFX/NS_EON_GerbeDir.NS_EON_GerbeDir"),
			TEXT("/Game/EON/VFX/NS_EON_Anneau.NS_EON_Anneau") };
		static TStrongObjectPtr<UNiagaraSystem> Cache[3];
		if (!Cache[Which].IsValid()) { Cache[Which].Reset(LoadObject<UNiagaraSystem>(nullptr, Paths[Which])); }
		return Cache[Which].Get();
	}

	UNiagaraComponent* Start(const UObject* Ctx, int32 Which, const FVector& At)
	{
		UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		UNiagaraSystem* S = Sys(Which);
		if (!W || !S || W->GetNetMode() == NM_DedicatedServer) { return nullptr; }
		return UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, S, At, FRotator::ZeroRotator, FVector::OneVector, true, false);
	}

	void Common(UNiagaraComponent* C, int32 N, const FLinearColor& Col, float Life, float SizePx, float Drag)
	{
		C->SetVariableInt(TEXT("User.Count"), N);
		C->SetVariableLinearColor(TEXT("User.Color"), Col);
		C->SetVariableFloat(TEXT("User.Life"), Life);
		const float Sz = EON::PxToCm(SizePx);
		C->SetVariableVec2(TEXT("User.Size"), FVector2D(Sz, Sz));
		C->SetVariableFloat(TEXT("User.Drag"), Drag);
		C->Activate(true);
	}
}

FLinearColor EONFx::Hex(const TCHAR* H)
{
	return FLinearColor(FColor::FromHex(H));
}

void EONFx::Gerbe(const UObject* Ctx, const FVector& At, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float SizePx, float Drag, float RadiusPx)
{
	if (N <= 0) { return; }
	if (UNiagaraComponent* C = Start(Ctx, 0, At))
	{
		C->SetVariableFloat(TEXT("User.Speed"), EON::PxToCm(SpeedPx));
		C->SetVariableFloat(TEXT("User.Radius"), EON::PxToCm(RadiusPx));
		Common(C, N, Col, Life, SizePx, Drag);
	}
}

void EONFx::GerbeDir(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float SizePx, float Drag)
{
	if (N <= 0) { return; }
	const FVector D = Dir.GetSafeNormal();
	if (D.IsNearlyZero()) { Gerbe(Ctx, At, N, Col, SpeedPx, Life, SizePx, Drag); return; }
	if (UNiagaraComponent* C = Start(Ctx, 1, At))
	{
		C->SetVariableFloat(TEXT("User.Speed"), EON::PxToCm(SpeedPx));
		C->SetVariableFloat(TEXT("User.Radius"), 0.f);
		// Un peu de hauteur : à plat, la moitié du cône rentrerait dans le sol.
		C->SetVariableVec3(TEXT("User.Dir"), (D + FVector(0.f, 0.f, 0.35f)).GetSafeNormal());
		Common(C, N, Col, Life, SizePx, Drag);
	}
}

void EONFx::Gerbe2(const UObject* Ctx, const FVector& At, int32 N, int32 Every, const FLinearColor& A, const FLinearColor& B, float SpeedPx, float Life, float SizePx, float Drag, float RadiusPx)
{
	const int32 Nb = Every > 0 ? (N + Every - 1) / Every : 0;
	Gerbe(Ctx, At, N - Nb, A, SpeedPx, Life, SizePx, Drag, RadiusPx);
	Gerbe(Ctx, At, Nb, B, SpeedPx, Life, SizePx, Drag, RadiusPx);
}

void EONFx::GerbeDir2(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, int32 Every, const FLinearColor& A, const FLinearColor& B, float SpeedPx, float Life, float SizePx, float Drag)
{
	const int32 Nb = Every > 0 ? (N + Every - 1) / Every : 0;
	GerbeDir(Ctx, At, Dir, N - Nb, A, SpeedPx, Life, SizePx, Drag);
	GerbeDir(Ctx, At, Dir, Nb, B, SpeedPx, Life, SizePx, Drag);
}

void EONFx::Anneau(const UObject* Ctx, const FVector& At, float RadiusPx, float Life, const FLinearColor& Col)
{
	if (UNiagaraComponent* C = Start(Ctx, 2, At + FVector(0.f, 0.f, 3.f)))
	{
		const float D = EON::PxToCm(RadiusPx) * 2.f;
		C->SetVariableLinearColor(TEXT("User.Color"), Col);
		C->SetVariableFloat(TEXT("User.Life"), Life);
		C->SetVariableVec2(TEXT("User.Size"), FVector2D(D, D));
		C->Activate(true);
	}
}

// ---------------------------------------------------------------- Effets en maillage (24/11, effets « ABS »)
#include "EONFxPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	AEONFxPiece* SpawnPiece(const UObject* Ctx, const FVector& At, float Life)
	{
		UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		if (!W || W->GetNetMode() == NM_DedicatedServer) { return nullptr; }
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEONFxPiece* F = W->SpawnActor<AEONFxPiece>(AEONFxPiece::StaticClass(), At, FRotator::ZeroRotator, P);
		if (F) { F->Life = FMath::Max(0.01f, Life); }
		return F;
	}
}

UMaterialInstanceDynamic* EONFx::Glow(UObject* Outer, EForme Forme, const FLinearColor& Col, float Intensite)
{
	static TStrongObjectPtr<UMaterialInterface> Base;
	if (!Base.IsValid()) { Base.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/M_EON_Glow.M_EON_Glow"))); }
	if (!Base.IsValid()) { return nullptr; }
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base.Get(), Outer);
	M->SetScalarParameterValue(TEXT("Forme"), (float)(uint8)Forme);
	M->SetVectorParameterValue(TEXT("Color"), Col);
	M->SetScalarParameterValue(TEXT("Intensite"), Intensite);
	return M;
}

void EONFx::Lumiere(const UObject* Ctx, const FVector& At, const FLinearColor& Col, float Intensity, float RadiusPx, float Life)
{
	if (AEONFxPiece* F = SpawnPiece(Ctx, At, Life)) { F->AddLight(Col, Intensity, EON::PxToCm(RadiusPx)); }
}

void EONFx::Eclair(const UObject* Ctx, const FVector& At, const FVector& Dir, const FLinearColor& Col, float Life, float Scale, float LightRadiusPx)
{
	AEONFxPiece* F = SpawnPiece(Ctx, At, Life);
	if (!F) { return; }
	const FVector D = Dir.GetSafeNormal2D();
	const float Yaw = D.Rotation().Yaw;
	const float S = EON::PxToCm(1.f) * Scale;
	// Halo 10 px couleur du tir, trait blanc de 6 px dans l'axe, cœur blanc de 4 px (draw du proto, l. 3428).
	F->AddPlane(Glow(F, EForme::Rond, Col, 2.5f), FVector::ZeroVector, Yaw, 14.f * S, 14.f * S);
	F->AddPlane(Glow(F, EForme::Uni, FLinearColor::White, 2.f), D * 3.f * S, Yaw, 6.f * S, 1.6f * S);
	F->AddPlane(Glow(F, EForme::Rond, FLinearColor::White, 4.f), FVector::ZeroVector, Yaw, 6.f * S, 6.f * S);
	if (LightRadiusPx > 0.f) { F->AddLight(Col, 6000.f, EON::PxToCm(LightRadiusPx)); }
}

void EONFx::Fantome(const UObject* Ctx, USkeletalMeshComponent* Src, const FLinearColor& Col, float Alpha, float Life)
{
	if (!Src) { return; }
	AEONFxPiece* F = SpawnPiece(Ctx, Src->GetComponentLocation(), Life);
	if (!F) { return; }
	F->Opacity0 = Alpha;
	UMaterialInstanceDynamic* M = Glow(F, EForme::Uni, Col, 1.f);
	if (M) { M->SetScalarParameterValue(TEXT("Opacite"), Alpha); }
	F->AddGhost(Src, M);
}

void EONFx::Douille(const UObject* Ctx, const FVector& At, const FVector& Vel, float FloorZ)
{
	AEONFxPiece* F = SpawnPiece(Ctx, At, 10.f);
	if (!F) { return; }
	F->bShell = true;
	F->ShellVel = Vel;
	F->FloorZ = FloorZ;
	static TStrongObjectPtr<UStaticMesh> Cyl;
	static TStrongObjectPtr<UMaterialInterface> Flat;
	if (!Cyl.IsValid()) { Cyl.Reset(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))); }
	// Celluloïd (Vincent, 24/11) : même matériau toon que le décor (M_EON_BG, teinte BC1) et trait du post-process.
	if (!Flat.IsValid()) { Flat.Reset(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/EON/Materials/BG/MI_BG_pipe.MI_BG_pipe"))); }
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(F);
	C->SetStaticMesh(Cyl.Get());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetupAttachment(F->GetRootComponent());
	// Cylindre moteur : 100 cm de diamètre, 100 cm de haut, axe Z. Douille couchée : 2 × 1 px (12,5 × 6,25 cm).
	C->SetRelativeLocationAndRotation(FVector(0.f, 0.f, EON::PxToCm(0.5f)), FRotator(90.f, 0.f, 0.f));
	C->SetRelativeScale3D(FVector(EON::PxToCm(1.f) / 100.f, EON::PxToCm(1.f) / 100.f, EON::PxToCm(2.f) / 100.f));
	if (Flat.IsValid())
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Flat.Get(), F);
		M->SetVectorParameterValue(TEXT("BC1"), Hex(TEXT("ffc23d")));
		C->SetMaterial(0, M);
	}
	C->RegisterComponent();
	UEONStatics::MarkCelComponent(C);
	AEONFxPiece::Shells.RemoveAll([](const TWeakObjectPtr<AEONFxPiece>& P) { return !P.IsValid(); });
	AEONFxPiece::Shells.Add(F);
	while (AEONFxPiece::Shells.Num() > 80) { if (AEONFxPiece* Old = AEONFxPiece::Shells[0].Get()) { Old->Destroy(); } AEONFxPiece::Shells.RemoveAt(0); }
}

AEONFxPiece* EONFx::Sol(const UObject* Ctx, EForme Forme, const FVector& At, float Yaw, float SizeXPx, float SizeYPx, const FLinearColor& Col, float Life,
	float Clignote, bool bFill, float Angle, float Graine, float Intensite)
{
	AEONFxPiece* F = SpawnPiece(Ctx, At + FVector(0.f, 0.f, 2.f), Life);
	if (!F) { return nullptr; }
	UMaterialInstanceDynamic* M = Glow(F, Forme, Col, Intensite);
	if (M)
	{
		M->SetScalarParameterValue(TEXT("Clignote"), Clignote);
		M->SetScalarParameterValue(TEXT("Angle"), Angle);
		M->SetScalarParameterValue(TEXT("Graine"), Graine);
		M->SetScalarParameterValue(TEXT("Remplissage"), bFill ? 0.f : 1.f);
	}
	F->bFill = bFill;
	F->AddPlane(M, FVector::ZeroVector, Yaw, EON::PxToCm(SizeXPx), EON::PxToCm(SizeYPx));
	return F;
}

// eon.fx [anneau|gerbe|dir] : effet de test sur Jira (vérifier le rendu des systèmes Niagara).
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
static FAutoConsoleCommandWithWorldAndArgs GEONCmdFx(TEXT("eon.fx"), TEXT("eon.fx [anneau|gerbe|dir|eclair|lumiere|flaque|organique|cercle|cone|ligne] : effet de test sur Jira"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
	{
		const APawn* P = W ? UGameplayStatics::GetPlayerPawn(W, 0) : nullptr;
		if (!P) { return; }
		const FVector Feet = P->GetActorLocation() - FVector(0.f, 0.f, P->GetSimpleCollisionHalfHeight());
		const FString K = Args.Num() ? Args[0] : TEXT("anneau");
		if (K == TEXT("gerbe")) { EONFx::Gerbe2(P, Feet + FVector(0.f, 0.f, 30.f), 34, 3, EONFx::Hex(TEXT("ff9a3d")), FLinearColor::White, 180.f, 0.5f, 1.25f, 5.f); }
		else if (K == TEXT("dir")) { EONFx::GerbeDir2(P, Feet + FVector(0.f, 0.f, 30.f), P->GetActorForwardVector(), 18, 3, EONFx::Hex(TEXT("e0405a")), FLinearColor::White, 170.f, 0.6f, 1.3f, 5.f); }
		else if (K == TEXT("eclair")) { EONFx::Eclair(P, Feet + FVector(0.f, 0.f, 90.f), P->GetActorForwardVector(), EONFx::Hex(TEXT("3ef2e0")), 1.5f); }
		else if (K == TEXT("lumiere")) { EONFx::Lumiere(P, Feet + FVector(0.f, 0.f, 60.f), EONFx::Hex(TEXT("ff7a1a")), 60000.f, 60.f, 1.f); }
		// Gardiens 2 et 3 (GAR-V18, GAR-V19) : effets prêts, branchés quand la Mechakuve et le Chercheur seront portés.
		else if (K == TEXT("flaque")) { EONFx::Sol(P, EONFx::EForme::Flaque, Feet, 0.f, 30.f, 30.f, EONFx::Hex(TEXT("7aff3a")), 4.5f, 0.f, false, 0.6f, FMath::FRand() * 10.f, 1.2f); }
		else if (K == TEXT("organique")) { EONFx::Sol(P, EONFx::EForme::Flaque, Feet, 0.f, 40.f, 40.f, EONFx::Hex(TEXT("c84a7a")), 6.f, 0.f, false, 0.6f, FMath::FRand() * 10.f, 1.2f); }
		else if (K == TEXT("cercle")) { EONFx::Sol(P, EONFx::EForme::Disque, Feet, 0.f, 48.f, 48.f, EONFx::Hex(TEXT("ff3d55")), 0.8f, 8.f, true); }
		else if (K == TEXT("cone")) { EONFx::Sol(P, EONFx::EForme::Cone, Feet, P->GetActorRotation().Yaw, 152.f, 152.f, EONFx::Hex(TEXT("ff3d55")), 0.55f, 0.f, true, 1.15f); }
		else if (K == TEXT("ligne")) { EONFx::Sol(P, EONFx::EForme::Ligne, Feet + P->GetActorForwardVector() * EON::PxToCm(60.f), P->GetActorRotation().Yaw, 120.f, 12.f, EONFx::Hex(TEXT("ff3d55")), 0.55f, 0.f, true); }
		else { EONFx::Anneau(P, Feet, 60.f, 1.5f, EONFx::Hex(TEXT("ff3d8b"))); }
	}));
