// EON — jalon 8 : sauvegarde et déblocages (META, UNLOCKS, checkUnlocks, casUnlock, buyCass du proto).
#include "EONMeta.h"
#include "EON.h"
#include "EONCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	const TCHAR* EONMetaSlot = TEXT("EON_Meta");

	int32 InsRendus(const UEONSaveGame* M)
	{
		int32 N = 0;
		if (M) { for (const auto& KV : M->Crew) { if (KV.Value == TEXT("rendu")) { ++N; } } }
		return N;
	}

	// u.t(META.st) du proto, dans l'ordre de deblocages.json.
	bool TestUnlock(FName Id, const UEONSaveGame* M)
	{
		if (Id == TEXT("u_cobaye")) { return M->bCobUnlock; }
		if (Id == TEXT("u_laser")) { return M->MaxFloor >= 2; }
		if (Id == TEXT("u_multi")) { return M->Runs >= 3; }
		if (Id == TEXT("u_shield")) { return M->MaxFloor >= 3; }
		if (Id == TEXT("u_lame")) { return M->Kills >= 150; }
		if (Id == TEXT("u_berserk")) { return M->MaxMut >= 3; }
		if (Id == TEXT("u_shotgun")) { return M->BossFloors.Contains(2); }
		if (Id == TEXT("u_plaque")) { return InsRendus(M) >= 3; }
		if (Id == TEXT("u_revive")) { return M->Morts >= 5; }
		if (Id == TEXT("u_reg")) { return M->BossFloors.Contains(3); }
		if (Id == TEXT("u_dashc")) { return M->MaxFloor >= 4; }
		if (Id == TEXT("u_double")) { return M->BossCob >= 1; }
		return false;
	}

	FAutoConsoleCommandWithWorld CmdUnlockAll(TEXT("eon.toutdebloquer"), TEXT("EON : tout debloquer (persos, greffes, cassette, objets)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W) { if (UEONMetaSubsystem* S = UEONMetaSubsystem::Get(W)) { S->UnlockAll(); } }));

	FAutoConsoleCommandWithWorldAndArgs CmdObjet(TEXT("eon.objet"), TEXT("EON : donne l'objet ID a Jira (takeItem), ex. eon.objet c_shield"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* W)
		{
			if (AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(W, 0)); J && A.Num()) { J->TakeItem(FName(*A[0])); UE_LOG(LogEON, Display, TEXT("eon.objet %s"), *A[0]); }
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdBlesse(TEXT("eon.blesse"), TEXT("EON : un coup de N PV sur Jira (tests : bouclier, mort)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* W)
		{
			if (AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(W, 0)))
			{
				J->HurtPlayer(A.Num() ? FCString::Atoi(*A[0]) : 1, J->GetActorLocation() + FVector(100.f, 0.f, 0.f));
				UE_LOG(LogEON, Display, TEXT("eon.blesse : PV %d, bouclier %d/%d"), J->GetHP(), J->GetShield(), J->GetShieldMax());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdMeta(TEXT("eon.meta"), TEXT("EON : affiche la sauvegarde. eon.meta donnees N | plusdonnees N | floor N | kills N | runs N | effacer"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* W)
		{
			UEONMetaSubsystem* S = UEONMetaSubsystem::Get(W);
			UEONSaveGame* M = S ? S->M() : nullptr;
			if (!M) { return; }
			if (A.Num() >= 1 && A[0] == TEXT("effacer")) { S->Wipe(); }
			else if (A.Num() >= 2)
			{
				const int32 V = FCString::Atoi(*A[1]);
				if (A[0] == TEXT("donnees")) { M->Donnees = V; }
				else if (A[0] == TEXT("plusdonnees")) { M->Donnees += V; }
				else if (A[0] == TEXT("floor")) { M->MaxFloor = V; }
				else if (A[0] == TEXT("kills")) { M->Kills = V; }
				else if (A[0] == TEXT("runs")) { M->Runs = V; }
				else if (A[0] == TEXT("morts")) { M->Morts = V; }
				else if (A[0] == TEXT("mut")) { M->MaxMut = V; }
				S->Save();
			}
			FString Unl;
			for (const FName& U : M->Unl) { Unl += U.ToString() + TEXT(" "); }
			FString Cass;
			for (const auto& KV : M->Cass) { Cass += FString::Printf(TEXT("%s NV%d%s "), *KV.Key.ToString(), KV.Value.Lvl, *KV.Value.Br); }
			UE_LOG(LogEON, Display, TEXT("EON META : runs %d, morts %d, kills %d, maxFloor %d, maxMut %d, bossCob %d, donnees %d, cobUnlock %d | unl [%s] | cass [%s]"),
				M->Runs, M->Morts, M->Kills, M->MaxFloor, M->MaxMut, M->BossCob, M->Donnees, M->bCobUnlock ? 1 : 0, *Unl, *Cass);
		}));
}

UEONMetaSubsystem* UEONMetaSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UEONMetaSubsystem>() : nullptr;
}

const TArray<FEONUnlockDef>& UEONMetaSubsystem::Unlocks()
{
	// DONNEES/deblocages.json (ordre et textes du proto).
	static const TArray<FEONUnlockDef> L = {
		{ TEXT("u_cobaye"), TEXT("Perso"), TEXT("Le Cobaye"), TEXT("Vaincre un gardien"), NAME_None, NAME_None },
		{ TEXT("u_laser"), TEXT("Greffe"), TEXT("Laser"), TEXT("Atteindre l'étage 2"), TEXT("laser"), NAME_None },
		{ TEXT("u_multi"), TEXT("Objet"), TEXT("Double canon"), TEXT("Faire 3 runs"), NAME_None, TEXT("j_multi") },
		{ TEXT("u_shield"), TEXT("Cassette"), TEXT("Shield"), TEXT("Atteindre l'étage 3"), NAME_None, TEXT("c_shield") },
		{ TEXT("u_lame"), TEXT("Greffe"), TEXT("Lame"), TEXT("Éliminer 150 ennemis au total"), TEXT("lame"), NAME_None },
		{ TEXT("u_berserk"), TEXT("Objet"), TEXT("Catalyseur Berserk"), TEXT("Atteindre le stade 3 de mutation"), NAME_None, TEXT("e_berserk") },
		{ TEXT("u_shotgun"), TEXT("Greffe"), TEXT("Shotgun"), TEXT("Vaincre Mechakuve (étage 2)"), TEXT("shotgun"), NAME_None },
		{ TEXT("u_plaque"), TEXT("Objet"), TEXT("Plaque de blindage"), TEXT("Rendre 3 insignes à Ulysse"), NAME_None, TEXT("n_plaque") },
		{ TEXT("u_revive"), TEXT("Objet"), TEXT("Renaissance"), TEXT("Mourir 5 fois"), NAME_None, TEXT("p_revive") },
		{ TEXT("u_reg"), TEXT("Objet"), TEXT("Régulateur"), TEXT("Vaincre Le Chercheur (étage 3)"), NAME_None, TEXT("e_reg") },
		{ TEXT("u_dashc"), TEXT("Objet"), TEXT("Charge de dash +"), TEXT("Atteindre les serres (étage 4)"), NAME_None, TEXT("p_dashc") },
		{ TEXT("u_double"), TEXT("Objet"), TEXT("Double griffe"), TEXT("Vaincre un gardien avec le Cobaye"), NAME_None, TEXT("k_double") },
	};
	return L;
}

void UEONMetaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Meta = Cast<UEONSaveGame>(UGameplayStatics::LoadGameFromSlot(EONMetaSlot, 0));
	if (!Meta) { Meta = Cast<UEONSaveGame>(UGameplayStatics::CreateSaveGameObject(UEONSaveGame::StaticClass())); }
	UE_LOG(LogEON, Display, TEXT("EON META chargée : runs %d, donnees %d, %d deblocages"), Meta->Runs, Meta->Donnees, Meta->Unl.Num());
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UEONMetaSubsystem::Tick));
}

void UEONMetaSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	if (bDirty) { Save(); }
	Super::Deinitialize();
}

void UEONMetaSubsystem::Save()
{
	if (!Meta) { return; }
	// crewSync : les PV bonus se recalculent depuis l'équipage (3 insignes rendus = +1 PV, 2 au plus).
	Meta->HpBonus = FMath::Min(2, InsRendus(Meta) / 3);
	UGameplayStatics::SaveGameToSlot(Meta, EONMetaSlot, 0);
	bDirty = false;
}

bool UEONMetaSubsystem::HasProgress() const
{
	const UEONSaveGame* M = Meta;
	return M && (M->Runs || M->Unl.Num() || M->bUlysse || M->bCobUnlock || M->LogN || M->bNewtDone || M->bVieille || M->Donnees || M->Crew.Num() || M->Cass.Num());
}

void UEONMetaSubsystem::Wipe()
{
	Meta = Cast<UEONSaveGame>(UGameplayStatics::CreateSaveGameObject(UEONSaveGame::StaticClass()));
	ToastQ.Reset(); ToastCur = NAME_None; ToastLife = 0.f; RunUnl.Reset();
	Save();
	UE_LOG(LogEON, Display, TEXT("EON META : progression effacée"));
}

bool UEONMetaSubsystem::GreffeOk(FName Greffe) const
{
	for (const FEONUnlockDef& U : Unlocks()) { if (U.Greffe == Greffe) { return IsUnlocked(U.Id); } }
	return true;
}

bool UEONMetaSubsystem::ItemOk(FName Item) const
{
	for (const FEONUnlockDef& U : Unlocks()) { if (U.Item == Item) { return IsUnlocked(U.Id); } }
	return true;
}

void UEONMetaSubsystem::CheckUnlocks()
{
	if (!Meta) { return; }
	bool bCh = false;
	for (const FEONUnlockDef& U : Unlocks())
	{
		if (!Meta->Unl.Contains(U.Id) && TestUnlock(U.Id, Meta))
		{
			Meta->Unl.Add(U.Id);
			bCh = true;
			if (U.Id == TEXT("u_cobaye")) { Meta->bCobUnlock = true; }
			ToastQ.Add(U.Id);
			RunUnl.Add(U.Id);
			UE_LOG(LogEON, Display, TEXT("EON DEBLOQUE %s : %s"), *U.Kind, *U.Name);
		}
	}
	if (bCh) { Save(); }   // son « item » du proto : pas encore de fichier (MANQUANTS.md, sons)
}

void UEONMetaSubsystem::UnlockAll()
{
	if (!Meta) { return; }
	for (const FEONUnlockDef& U : Unlocks()) { Meta->Unl.AddUnique(U.Id); }
	Meta->bCobUnlock = true;
	Save();
	UE_LOG(LogEON, Display, TEXT("EON META : tout est debloque"));
}

const FEONUnlockDef* UEONMetaSubsystem::GetToast(float& OutLife) const
{
	OutLife = ToastLife;
	if (ToastCur.IsNone()) { return nullptr; }
	for (const FEONUnlockDef& U : Unlocks()) { if (U.Id == ToastCur) { return &U; } }
	return nullptr;
}

void UEONMetaSubsystem::OnFloor(int32 FloorN)
{
	if (Meta && FloorN > Meta->MaxFloor) { Meta->MaxFloor = FloorN; }
	Save();
}

void UEONMetaSubsystem::OnMut(int32 Stade)
{
	if (Meta && Stade > Meta->MaxMut) { Meta->MaxMut = Stade; bDirty = true; }
}

void UEONMetaSubsystem::OnDeath()
{
	if (!Meta) { return; }
	++Meta->Runs;
	++Meta->Morts;
	Save();
	CheckUnlocks();
}

bool UEONMetaSubsystem::OnBossDefeated(int32 FloorN)
{
	if (!Meta) { return false; }
	Meta->BossFloors.AddUnique(FloorN);
	if (Meta->Chara == TEXT("cobaye")) { ++Meta->BossCob; }
	const bool bNew = !Meta->bCobUnlock;
	Meta->bCobUnlock = true;
	Save();
	return bNew;
}

int32 UEONMetaSubsystem::CasLvl(FName K) const
{
	const FEONCassette* C = Meta ? Meta->Cass.Find(K) : nullptr;
	return C ? C->Lvl : 0;
}

FString UEONMetaSubsystem::CasBr(FName K) const
{
	const FEONCassette* C = Meta ? Meta->Cass.Find(K) : nullptr;
	return C ? C->Br : FString();
}

void UEONMetaSubsystem::CasUnlock(FName K)
{
	if (Meta && CasLvl(K) < 1) { FEONCassette C; C.Lvl = 1; Meta->Cass.Add(K, C); Save(); }
}

bool UEONMetaSubsystem::BuyCass(FName K, int32 Lvl, const FString& Br, int32 Cost)
{
	if (!Meta || Meta->Donnees < Cost) { return false; }
	Meta->Donnees -= Cost;
	FEONCassette C;
	C.Lvl = Lvl;
	C.Br = Br.IsEmpty() ? CasBr(K) : Br;
	Meta->Cass.Add(K, C);
	Save();
	return true;
}

bool UEONMetaSubsystem::Tick(float Dt)
{
	Dt = FMath::Min(Dt, 0.1f);
	// Vérification toutes les 0,5 s.
	CheckT -= Dt;
	if (CheckT <= 0.f)
	{
		CheckT = 0.5f;
		CheckUnlocks();
		if (bDirty) { Save(); }
	}
	// Encart « DÉBLOQUÉ » : un à la fois, 3,2 s.
	if (!ToastCur.IsNone())
	{
		ToastLife -= Dt;
		if (ToastLife <= 0.f) { ToastCur = NAME_None; }
	}
	if (ToastCur.IsNone() && ToastQ.Num())
	{
		ToastCur = ToastQ[0];
		ToastQ.RemoveAt(0);
		ToastLife = 3.2f;
	}
	return true;
}
