// EON — banc de réglages modifiable en jeu (équivalent du panneau du proto).
// Console : eon.feel <clé> <valeur> | eon.feel.preset <studio|sec|max> | eon.feel.list
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "EONTypes.h"
#include "EONTuningSubsystem.generated.h"

class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEONOnGameFeelChanged);

UCLASS()
class EON_API UEONTuningSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** stop(ms) du proto : fige tout le jeu pendant Ms millisecondes de temps réel (le plus long gagne).
	 *  Arrêt sur image des coups : rien si le joueur l'a coupé (Options > Jouabilité). */
	UFUNCTION(BlueprintCallable, Category = "EON|Réglages")
	void RequestHitstop(float Ms);

	/** Gel de mise en scène (QTE, ultime) : toujours appliqué, même sans arrêt sur image. */
	void RequestFreeze(float Ms);

	/** Réglages en vigueur : le banc (préréglage + menu F6) corrigé par les options du joueur (EONOptions::ApplyFeel). */
	UFUNCTION(BlueprintPure, Category = "EON|Réglages")
	const FEONGameFeelRow& GetGameFeel() const { return Effective; }

	/** Options du joueur changées : recalcule les réglages en vigueur et prévient le jeu. */
	void RefreshPlayerOptions();

	/** Applique une ligne de DT_GameFeel (studio, sec, max). */
	UFUNCTION(BlueprintCallable, Category = "EON|Réglages")
	bool ApplyPreset(FName Preset);

	/** Change un réglage par son nom (RunMul, Shake, Lookahead…). Insensible à la casse. */
	UFUNCTION(BlueprintCallable, Category = "EON|Réglages")
	bool SetValue(const FString& Key, const FString& Value);

	UFUNCTION(BlueprintPure, Category = "EON|Réglages")
	FString GetValueAsString(const FString& Key) const;

	/** Valeur d'un réglage dans un préréglage de DT_GameFeel (vide si inconnu). */
	FString GetPresetValueAsString(FName Preset, const FString& Key) const;

	/** Liste "clé = valeur" de tous les réglages. */
	UFUNCTION(BlueprintPure, Category = "EON|Réglages")
	TArray<FString> DescribeAll() const;

	UPROPERTY(BlueprintAssignable, Category = "EON|Réglages")
	FEONOnGameFeelChanged OnGameFeelChanged;

	/** Tables de données (chargées une fois). */
	UFUNCTION(BlueprintPure, Category = "EON|Données") UDataTable* GetGreffes() const { return Greffes; }
	UFUNCTION(BlueprintPure, Category = "EON|Données") UDataTable* GetEnemies() const { return Enemies; }

	static UEONTuningSubsystem* Get(const UObject* WorldContext);

private:
	FProperty* FindFeelProperty(const FString& Key) const;
	bool TickHitstop(float DeltaTime);
	void EndHitstop();
	/** Effective = Feel + options du joueur. */
	void RebuildEffective();

	double HitstopEnd = 0.0;
	FTSTicker::FDelegateHandle HitstopHandle;

	/** Banc du studio (DT_GameFeel, menu F6) ; le jeu lit Effective. */
	UPROPERTY() FEONGameFeelRow Feel;
	UPROPERTY() FEONGameFeelRow Effective;
	UPROPERTY() TObjectPtr<UDataTable> GameFeelTable;
	UPROPERTY() TObjectPtr<UDataTable> Greffes;
	UPROPERTY() TObjectPtr<UDataTable> Enemies;
};
