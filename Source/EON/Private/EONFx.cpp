#include "EONFx.h"
#include "EONTypes.h"
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

// eon.fx [anneau|gerbe|dir] : effet de test sur Jira (vérifier le rendu des systèmes Niagara).
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
static FAutoConsoleCommandWithWorldAndArgs GEONCmdFx(TEXT("eon.fx"), TEXT("eon.fx [anneau|gerbe|dir] : effet de test sur Jira"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* W)
	{
		const APawn* P = W ? UGameplayStatics::GetPlayerPawn(W, 0) : nullptr;
		if (!P) { return; }
		const FVector Feet = P->GetActorLocation() - FVector(0.f, 0.f, P->GetSimpleCollisionHalfHeight());
		const FString K = Args.Num() ? Args[0] : TEXT("anneau");
		if (K == TEXT("gerbe")) { EONFx::Gerbe2(P, Feet + FVector(0.f, 0.f, 30.f), 34, 3, EONFx::Hex(TEXT("ff9a3d")), FLinearColor::White, 180.f, 0.5f, 1.25f, 5.f); }
		else if (K == TEXT("dir")) { EONFx::GerbeDir2(P, Feet + FVector(0.f, 0.f, 30.f), P->GetActorForwardVector(), 18, 3, EONFx::Hex(TEXT("e0405a")), FLinearColor::White, 170.f, 0.6f, 1.3f, 5.f); }
		else { EONFx::Anneau(P, Feet, 60.f, 1.5f, EONFx::Hex(TEXT("ff3d8b"))); }
	}));
