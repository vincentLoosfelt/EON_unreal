// EON — menus de test en jeu.
// F6 : menu Tweak (banc de réglages du proto, §12 de 02_JOUEUR_ET_COMBAT) pour calibrer Jira, le tir, la caméra et les ennemis.
// F7 : menu Triche (panneau DEV du proto) : invincible, énergie infinie, téléportation, ADN, scrap…
#pragma once

#include "CoreMinimal.h"

class APlayerController;

/** Une ligne prête à dessiner. */
struct FEONMenuLine
{
	FString Label;
	FString Value;
	FString Hint;           // valeur de référence (studio) quand elle diffère
	bool bSection = false;  // titre de rubrique
	bool bSelected = false;
	bool bChanged = false;  // différent du préréglage studio
	bool bOn = false;       // interrupteur allumé
};

enum class EEONDevMenu : uint8 { None, Tweak, Triche };

class EON_API FEONDevMenu
{
public:
	EEONDevMenu GetOpen() const { return Open; }
	bool IsOpen() const { return Open != EEONDevMenu::None; }

	/** Touches F6 / F7, navigation et actions. RealDt = temps réel (le menu marche jeu en pause). */
	void Tick(APlayerController* PC, float RealDt);

	/** Titre, lignes visibles (fenêtre défilante) et pied de page. */
	void Build(const APlayerController* PC, FString& Title, TArray<FEONMenuLine>& Lines, FString& Footer) const;

private:
	struct FItem
	{
		FString Label;
		FString Key;            // réglage du banc (FEONGameFeelRow) ; vide pour une action
		float Step = 0.f;       // 0 = interrupteur
		float Min = 0.f;
		int32 Action = -1;      // action du menu (voir .cpp)
		bool bSection = false;
	};
	void Rebuild();
	void Change(APlayerController* PC, const FItem& It, int32 Dir, bool bEnter);
	void Activate(APlayerController* PC, int32 Action, int32 Dir, bool bEnter);
	FString ActionValue(const APlayerController* PC, int32 Action) const;
	void Export(APlayerController* PC);
	void Flash(const FString& Msg) { Message = Msg; MessageT = 4.f; }

	EEONDevMenu Open = EEONDevMenu::None;
	TArray<FItem> Items;
	int32 Sel[3] = { 0, 1, 1 };
	int32 PresetIdx = 0;
	int32 RoleIdx = 0;
	float HoldT = 0.f;
	float RepeatT = 0.f;
	FString Message;
	float MessageT = 0.f;
};
