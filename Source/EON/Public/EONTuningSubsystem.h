// EON — banc de réglages modifiable en jeu (équivalent du panneau du proto).
// Console : eon.feel <clé> <valeur> | eon.feel.preset <studio|sec|max> | eon.feel.list
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
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

	/** Réglages courants. */
	UFUNCTION(BlueprintPure, Category = "EON|Réglages")
	const FEONGameFeelRow& GetGameFeel() const { return Feel; }

	/** Applique une ligne de DT_GameFeel (studio, sec, max). */
	UFUNCTION(BlueprintCallable, Category = "EON|Réglages")
	bool ApplyPreset(FName Preset);

	/** Change un réglage par son nom (RunMul, Shake, Lookahead…). Insensible à la casse. */
	UFUNCTION(BlueprintCallable, Category = "EON|Réglages")
	bool SetValue(const FString& Key, const FString& Value);

	UFUNCTION(BlueprintPure, Category = "EON|Réglages")
	FString GetValueAsString(const FString& Key) const;

	/** Liste "clé = valeur" de tous les réglages. */
	UFUNCTION(BlueprintPure, Category = "EON|Réglages")
	TArray<FString> DescribeAll() const;

	UPROPERTY(BlueprintAssignable, Category = "EON|Réglages")
	FEONOnGameFeelChanged OnGameFeelChanged;

	/** Tables de données (chargées une fois). */
	UFUNCTION(BlueprintPure, Category = "EON|Données") UDataTable* GetGreffes() const { return Greffes; }
	UFUNCTION(BlueprintPure, Category = "EON|Données") UDataTable* GetEnemies() const { return Enemies; }
	UFUNCTION(BlueprintPure, Category = "EON|Données") UDataTable* GetSprites() const { return Sprites; }

	static UEONTuningSubsystem* Get(const UObject* WorldContext);

private:
	FProperty* FindFeelProperty(const FString& Key) const;

	UPROPERTY() FEONGameFeelRow Feel;
	UPROPERTY() TObjectPtr<UDataTable> GameFeelTable;
	UPROPERTY() TObjectPtr<UDataTable> Greffes;
	UPROPERTY() TObjectPtr<UDataTable> Enemies;
	UPROPERTY() TObjectPtr<UDataTable> Sprites;
};
