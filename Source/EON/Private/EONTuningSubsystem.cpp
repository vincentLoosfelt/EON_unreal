#include "EONTuningSubsystem.h"
#include "EON.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* GameFeelPath = TEXT("/Game/EON/Data/DT_GameFeel.DT_GameFeel");
	const TCHAR* GreffesPath = TEXT("/Game/EON/Data/DT_Greffes.DT_Greffes");
	const TCHAR* EnemiesPath = TEXT("/Game/EON/Data/DT_Ennemis.DT_Ennemis");
	const TCHAR* SpritesPath = TEXT("/Game/EON/Data/DT_Sprites.DT_Sprites");

	UEONTuningSubsystem* FirstTuning(UWorld* World)
	{
		if (World && World->GetGameInstance())
		{
			return World->GetGameInstance()->GetSubsystem<UEONTuningSubsystem>();
		}
		return nullptr;
	}

	void Print(const FString& Msg)
	{
		UE_LOG(LogEON, Display, TEXT("%s"), *Msg);
		if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan, Msg); }
	}

	FAutoConsoleCommandWithWorldAndArgs GEONCmdFeel(
		TEXT("eon.feel"),
		TEXT("eon.feel <clé> <valeur> : change un réglage du banc (ex. eon.feel Shake 2)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UEONTuningSubsystem* T = FirstTuning(World);
			if (!T) { return; }
			if (Args.Num() == 1) { Print(Args[0] + TEXT(" = ") + T->GetValueAsString(Args[0])); return; }
			if (Args.Num() < 2) { Print(TEXT("eon.feel <clé> <valeur>")); return; }
			Print(T->SetValue(Args[0], Args[1])
				? FString::Printf(TEXT("%s = %s"), *Args[0], *T->GetValueAsString(Args[0]))
				: FString::Printf(TEXT("Réglage inconnu : %s"), *Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs GEONCmdFeelPreset(
		TEXT("eon.feel.preset"),
		TEXT("eon.feel.preset <studio|sec|max> : charge un préréglage du banc"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UEONTuningSubsystem* T = FirstTuning(World);
			if (!T || Args.Num() < 1) { return; }
			Print(T->ApplyPreset(FName(*Args[0]))
				? FString::Printf(TEXT("Préréglage %s appliqué"), *Args[0])
				: FString::Printf(TEXT("Préréglage inconnu : %s"), *Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs GEONCmdFeelList(
		TEXT("eon.feel.list"),
		TEXT("eon.feel.list : liste les réglages du banc"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UEONTuningSubsystem* T = FirstTuning(World))
			{
				for (const FString& L : T->DescribeAll()) { UE_LOG(LogEON, Display, TEXT("%s"), *L); }
				Print(TEXT("Réglages listés dans le log (Output Log)"));
			}
		}));
}

void UEONTuningSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GameFeelTable = LoadObject<UDataTable>(nullptr, GameFeelPath);
	Greffes = LoadObject<UDataTable>(nullptr, GreffesPath);
	Enemies = LoadObject<UDataTable>(nullptr, EnemiesPath);
	Sprites = LoadObject<UDataTable>(nullptr, SpritesPath);

	if (!ApplyPreset(TEXT("studio")))
	{
		UE_LOG(LogEON, Warning, TEXT("DT_GameFeel introuvable : réglages par défaut du code"));
	}
	UE_LOG(LogEON, Display, TEXT("EON_TUNING greffes=%d ennemis=%d sprites=%d"),
		Greffes ? Greffes->GetRowMap().Num() : -1,
		Enemies ? Enemies->GetRowMap().Num() : -1,
		Sprites ? Sprites->GetRowMap().Num() : -1);
}

void UEONTuningSubsystem::Deinitialize()
{
	EndHitstop();
	Super::Deinitialize();
}

void UEONTuningSubsystem::RequestHitstop(float Ms)
{
	if (Ms <= 0.f) { return; }
	UWorld* World = GetWorld();
	if (!World) { return; }
	HitstopEnd = FMath::Max(HitstopEnd, FPlatformTime::Seconds() + Ms / 1000.0);
	if (!HitstopHandle.IsValid())
	{
		// Le ticker tourne en temps réel : il n'est pas ralenti par la dilatation qu'il pilote.
		UGameplayStatics::SetGlobalTimeDilation(World, 0.0001f);
		HitstopHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UEONTuningSubsystem::TickHitstop));
	}
}

bool UEONTuningSubsystem::TickHitstop(float)
{
	if (FPlatformTime::Seconds() < HitstopEnd) { return true; }
	HitstopHandle.Reset();
	if (UWorld* World = GetWorld()) { UGameplayStatics::SetGlobalTimeDilation(World, 1.f); }
	return false;
}

void UEONTuningSubsystem::EndHitstop()
{
	if (HitstopHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HitstopHandle);
		HitstopHandle.Reset();
		if (UWorld* World = GetWorld()) { UGameplayStatics::SetGlobalTimeDilation(World, 1.f); }
	}
}

UEONTuningSubsystem* UEONTuningSubsystem::Get(const UObject* WorldContext)
{
	if (const UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContext))
	{
		return GI->GetSubsystem<UEONTuningSubsystem>();
	}
	return nullptr;
}

bool UEONTuningSubsystem::ApplyPreset(FName Preset)
{
	if (!GameFeelTable) { return false; }
	const FEONGameFeelRow* Row = GameFeelTable->FindRow<FEONGameFeelRow>(Preset, TEXT("EON"), false);
	if (!Row) { return false; }
	// Les triches (menu F7) restent actives quand on change de préréglage.
	const FEONGameFeelRow Old = Feel;
	Feel = *Row;
	Feel.God = Old.God; Feel.InfEnergy = Old.InfEnergy; Feel.InfDash = Old.InfDash; Feel.OneShot = Old.OneShot;
	OnGameFeelChanged.Broadcast();
	return true;
}

FString UEONTuningSubsystem::GetPresetValueAsString(FName Preset, const FString& Key) const
{
	const FEONGameFeelRow* Row = GameFeelTable ? GameFeelTable->FindRow<FEONGameFeelRow>(Preset, TEXT("EON"), false) : nullptr;
	FProperty* Prop = FindFeelProperty(Key);
	if (!Row || !Prop) { return FString(); }
	FString Out;
	Prop->ExportTextItem_Direct(Out, Prop->ContainerPtrToValuePtr<void>(Row), nullptr, nullptr, PPF_None);
	return Out;
}

FProperty* UEONTuningSubsystem::FindFeelProperty(const FString& Key) const
{
	for (TFieldIterator<FProperty> It(FEONGameFeelRow::StaticStruct()); It; ++It)
	{
		if (It->GetName().Equals(Key, ESearchCase::IgnoreCase)) { return *It; }
	}
	return nullptr;
}

bool UEONTuningSubsystem::SetValue(const FString& Key, const FString& Value)
{
	FProperty* Prop = FindFeelProperty(Key);
	if (!Prop) { return false; }
	void* Ptr = Prop->ContainerPtrToValuePtr<void>(&Feel);
	if (!Prop->ImportText_Direct(*Value, Ptr, nullptr, PPF_None)) { return false; }
	OnGameFeelChanged.Broadcast();
	return true;
}

FString UEONTuningSubsystem::GetValueAsString(const FString& Key) const
{
	const FProperty* Prop = FindFeelProperty(Key);
	if (!Prop) { return TEXT("?"); }
	FString Out;
	Prop->ExportTextItem_Direct(Out, Prop->ContainerPtrToValuePtr<void>(&Feel), nullptr, nullptr, PPF_None);
	return Out;
}

TArray<FString> UEONTuningSubsystem::DescribeAll() const
{
	TArray<FString> Lines;
	for (TFieldIterator<FProperty> It(FEONGameFeelRow::StaticStruct()); It; ++It)
	{
		Lines.Add(It->GetName() + TEXT(" = ") + GetValueAsString(It->GetName()));
	}
	return Lines;
}
