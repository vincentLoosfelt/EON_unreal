// EON — jalon 8 : la méta, ce qui reste entre les runs (05, sections 6, 7 et 10).
// UEONSaveGame = l'objet META du proto (un seul emplacement « EON_Meta »).
// UEONMetaSubsystem le charge au lancement, le sauve dès qu'il change, vérifie les 12 déblocages toutes les 0,5 s
// (checkUnlocks) et tient la file des encarts « DÉBLOQUÉ » (3,2 s chacun) et la liste des déblocages de la run.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "EONMeta.generated.h"

/** Un déblocage (DONNEES/deblocages.json, UNLOCKS du proto). */
struct EON_API FEONUnlockDef
{
	FName Id;
	FString Kind;     // Perso, Greffe, Cassette, Objet
	FString Name;
	FString Cond;
	FName Greffe;     // greffe verrouillée (LOCK_G)
	FName Item;       // objet verrouillé (LOCK_IT)
};

/** Niveau d'une cassette : lvl 0 à 3, branche A ou B au NV3. */
USTRUCT()
struct EON_API FEONCassette
{
	GENERATED_BODY()
	UPROPERTY() int32 Lvl = 0;
	UPROPERTY() FString Br;
};

UCLASS()
class EON_API UEONSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() FName Chara = TEXT("jira");
	// st : stats cumulées
	UPROPERTY() int32 Runs = 0;
	UPROPERTY() int32 Morts = 0;
	UPROPERTY() int32 Kills = 0;
	UPROPERTY() int32 MaxFloor = 0;
	UPROPERTY() int32 MaxMut = 0;
	UPROPERTY() TArray<int32> BossFloors;
	UPROPERTY() int32 BossCob = 0;
	// unl, donnees, cass
	UPROPERTY() TArray<FName> Unl;
	UPROPERTY() int32 Donnees = 0;
	UPROPERTY() TMap<FName, FEONCassette> Cass;
	// Quêtes (Ulysse, équipage, logs, gamine, vieille dame, Cobaye) et aides déjà vues
	UPROPERTY() bool bUlysse = false;
	UPROPERTY() TMap<FString, FString> Crew;      // membre -> « poche » ou « rendu »
	UPROPERTY() int32 HpBonus = 0;
	UPROPERTY() int32 LogN = 0;
	UPROPERTY() bool bNewtDone = false;
	UPROPERTY() FString NewtEnd;
	UPROPERTY() bool bVieille = false;
	UPROPERTY() bool bCobUnlock = false;
	UPROPERTY() bool bMetCobaye = false;
	UPROPERTY() bool bDockSeen = false;
	UPROPERTY() bool bDonHint = false;
};

UCLASS()
class EON_API UEONMetaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static UEONMetaSubsystem* Get(const UObject* WorldContext);
	static const TArray<FEONUnlockDef>& Unlocks();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UEONSaveGame* M() const { return Meta; }
	void Save();
	/** hasProgress() : runs, déblocages, Ulysse, Cobaye, logs, gamine, vieille dame, Données, équipage, cassettes. */
	bool HasProgress() const;
	/** Nouvelle partie : tout est effacé (resetMeta). */
	void Wipe();

	// --- Déblocages
	bool IsUnlocked(FName UnlockId) const { return Meta && Meta->Unl.Contains(UnlockId); }
	/** greffeOk(k) : une greffe sans verrou, ou dont le déblocage est obtenu. */
	bool GreffeOk(FName Greffe) const;
	/** unlOk(it) : objet sans verrou, ou débloqué. */
	bool ItemOk(FName Item) const;
	void CheckUnlocks();
	/** Commande de test « tout débloquer » (b-unl du proto). */
	void UnlockAll();
	/** Encart « DÉBLOQUÉ » en cours (nullptr = aucun) et son temps de vie (3,2 s → 0). */
	const FEONUnlockDef* GetToast(float& OutLife) const;
	/** Déblocages obtenus pendant la run (écran de mort). */
	const TArray<FName>& GetRunUnlocks() const { return RunUnl; }

	// --- Stats (META.st)
	void OnRunStart() { RunUnl.Reset(); }
	void OnKill() { if (Meta) { ++Meta->Kills; bDirty = true; } }
	void OnFloor(int32 FloorN);
	void OnMut(int32 Stade);
	/** checkDeath sans Renaissance : runs++ et morts++, sauvegarde, vérification immédiate. */
	void OnDeath();
	/** Gardien vaincu : boss[étage], bossCob, puis cobUnlock. Vrai si le Cobaye vient d'être débloqué. */
	bool OnBossDefeated(int32 FloorN);

	// --- Données et cassettes
	int32 GetDonnees() const { return Meta ? Meta->Donnees : 0; }
	void AddDonnees(int32 N) { if (Meta && N > 0) { Meta->Donnees += N; bDirty = true; } }
	int32 CasLvl(FName K) const;
	FString CasBr(FName K) const;
	/** casUnlock(k) : NV1 trouvée en run, acquise pour toujours. */
	void CasUnlock(FName K);
	/** buyCass : retire le coût et installe le niveau (et la branche au NV3). Faux si pas assez de Données. */
	bool BuyCass(FName K, int32 Lvl, const FString& Br, int32 Cost);

	/** Écran titre déjà passé dans cette session (il ne revient pas à chaque nouvelle run). */
	bool bTitleDone = false;

private:
	bool Tick(float Dt);
	UPROPERTY() TObjectPtr<UEONSaveGame> Meta;
	FTSTicker::FDelegateHandle TickHandle;
	float CheckT = 0.f;
	bool bDirty = false;
	TArray<FName> ToastQ;
	FName ToastCur;
	float ToastLife = 0.f;
	TArray<FName> RunUnl;
};
