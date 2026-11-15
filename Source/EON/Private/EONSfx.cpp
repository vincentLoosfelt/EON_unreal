#include "EONSfx.h"
#include "EON.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"

namespace
{
	TMap<FName, TWeakObjectPtr<USoundBase>> GCache;
	TMap<FName, double> GLast;

	USoundBase* Find(const FString& Name)
	{
		const FName K(*Name);
		if (TWeakObjectPtr<USoundBase>* W = GCache.Find(K); W && W->IsValid()) { return W->Get(); }
		USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/EON/Audio/Placeholders/%s.%s"), *Name, *Name));
		if (!S) { UE_LOG(LogEON, Warning, TEXT("Son introuvable : %s"), *Name); }
		GCache.Add(K, S);
		return S;
	}
}

void EONSfx::Play(const UObject* WorldContext, const TCHAR* Name, float Volume, int32 Variants, float PitchVar, float MinGap)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld()) { return; }
	const FName Key(Name);
	const double Now = World->GetRealTimeSeconds();
	if (MinGap > 0.f)
	{
		if (const double* L = GLast.Find(Key); L && Now - *L >= 0.0 && Now - *L < MinGap) { return; }
		GLast.Add(Key, Now);
	}
	const FString Full = Variants > 0 ? FString::Printf(TEXT("%s_%02d"), Name, FMath::RandRange(1, Variants)) : FString(Name);
	if (USoundBase* S = Find(Full))
	{
		UGameplayStatics::PlaySound2D(WorldContext, S, Volume, 1.f + FMath::FRandRange(-PitchVar, PitchVar));
	}
}
