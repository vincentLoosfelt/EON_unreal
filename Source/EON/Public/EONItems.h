// EON — jalon 6 : les 36 objets (ITEMS du proto), générés depuis DONNEES/items.json par gen_items.py. Ne pas éditer à la main.
#pragma once

#include "CoreMinimal.h"

class AEONCharacter;

struct EON_API FEONItemDef
{
	FName Id;
	FName Cat;          // DISQUETTE, PERK, GREFFE, CASSETTE, ECHO
	TCHAR Rar;          // C, R, E
	FString Name;
	FString Desc;
	TArray<FName> Gr;   // ne sort que si une de ces greffes est active
	bool bOnce = false; // unique par run
	FName Greffe;       // objets GREFFE
};

namespace EONItems
{
	EON_API const TArray<FEONItemDef>& All();
	EON_API const FEONItemDef* Find(FName Id);
	/** Rareté : poids (60/30/10), couleur, nom. */
	EON_API float Weight(TCHAR Rar);
	EON_API FLinearColor Color(TCHAR Rar);
	EON_API FString RarName(TCHAR Rar);
	/** Prix en scrap chez le marchand (ITEM_PRICE) : C 8, R 16, E 28, +6 pour une greffe. */
	EON_API int32 Price(const FEONItemDef& It);
	/** rollItems(n) / rollLevel(n) : tirage pondéré sans remise, avec les filtres du proto. */
	EON_API TArray<FName> Roll(int32 N, const AEONCharacter* J, bool bLevel);
}
