// EON — jalon 6 : table des objets (générée depuis DONNEES/items.json) et tirages.
#include "EONItems.h"
#include "EONCharacter.h"

namespace EONItems
{
	const TArray<FEONItemDef>& All()
	{
		static const TArray<FEONItemDef> Items = {
		{ TEXT("d_hp"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Disquette HP"), TEXT("+1 PV max et soigne 1 PV"), {  }, false, TEXT("") },
		{ TEXT("d_spd"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Disquette Vitesse"), TEXT("+8% vitesse de deplacement"), {  }, false, TEXT("") },
		{ TEXT("d_en"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Disquette Energie"), TEXT("+15 energie max"), {  }, false, TEXT("") },
		{ TEXT("d_dmg"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Disquette Degats"), TEXT("+15% degats du Cobra"), {  }, false, TEXT("") },
		{ TEXT("d_cd"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Disquette Recharge"), TEXT("-12% recharge du dash et de l onde"), {  }, false, TEXT("") },
		{ TEXT("p_rate"), TEXT("PERK"), TEXT('C'), TEXT("Vitesse de tir +"), TEXT("+12% cadence du mode actif"), {  }, false, TEXT("") },
		{ TEXT("p_thick"), TEXT("PERK"), TEXT('C'), TEXT("Epaisseur du tir +"), TEXT("Projectiles plus gros, lame plus large"), {  }, false, TEXT("") },
		{ TEXT("p_range"), TEXT("PERK"), TEXT('R'), TEXT("Portee +"), TEXT("+20% portee du mode actif"), {  }, false, TEXT("") },
		{ TEXT("p_charge"), TEXT("PERK"), TEXT('R'), TEXT("Reduction de charge"), TEXT("-20% temps de charge rafale et laser"), {  }, false, TEXT("") },
		{ TEXT("p_dashd"), TEXT("PERK"), TEXT('R'), TEXT("Dash elargi"), TEXT("+25% distance de dash"), {  }, false, TEXT("") },
		{ TEXT("p_dashc"), TEXT("PERK"), TEXT('E'), TEXT("Charge de dash +"), TEXT("+1 charge de dash"), {  }, false, TEXT("") },
		{ TEXT("p_revive"), TEXT("PERK"), TEXT('E'), TEXT("Renaissance"), TEXT("Ressuscite une fois a 50% de vie"), {  }, false, TEXT("") },
		{ TEXT("c_shield"), TEXT("CASSETTE"), TEXT('R'), TEXT("Shield"), TEXT("Absorbe 1 coup. Gardee entre les runs, ameliorable au Kibo"), {  }, true, TEXT("") },
		{ TEXT("d_don"), TEXT("DISQUETTE"), TEXT('R'), TEXT("Disquette Donnees"), TEXT("+30% Donnees recuperees"), {  }, false, TEXT("") },
		{ TEXT("g_rafale"), TEXT("GREFFE"), TEXT('R'), TEXT("Greffe Rafale"), TEXT("Maintenir pour charger, relacher pour la salve"), {  }, false, TEXT("rafale") },
		{ TEXT("g_laser"), TEXT("GREFFE"), TEXT('R'), TEXT("Laser"), TEXT("Rayon charge qui transperce tout sur une ligne"), {  }, false, TEXT("laser") },
		{ TEXT("g_lame"), TEXT("GREFFE"), TEXT('R'), TEXT("Lame"), TEXT("Arc de melee. Detruit les tirs ennemis"), {  }, false, TEXT("lame") },
		{ TEXT("g_shotgun"), TEXT("GREFFE"), TEXT('R'), TEXT("Shotgun"), TEXT("Cone court et puissant. Recompense le risque"), {  }, false, TEXT("shotgun") },
		{ TEXT("g_standard"), TEXT("GREFFE"), TEXT('C'), TEXT("Greffe Standard"), TEXT("Tir rapide et precis, econome"), {  }, false, TEXT("standard") },
		{ TEXT("e_berserk"), TEXT("ECHO"), TEXT('E'), TEXT("Catalyseur Berserk"), TEXT("La mutation augmente tes degats : +35% par stade"), {  }, true, TEXT("") },
		{ TEXT("e_stab"), TEXT("ECHO"), TEXT('R'), TEXT("Stabilisateur"), TEXT("Les stades de mutation durent 2x plus longtemps"), {  }, true, TEXT("") },
		{ TEXT("e_reg"), TEXT("ECHO"), TEXT('E'), TEXT("Regulateur"), TEXT("La mutation reste bloquee au stade 1"), {  }, true, TEXT("") },
		{ TEXT("n_kit"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Kit de soin"), TEXT("Soigne 3 PV"), {  }, false, TEXT("") },
		{ TEXT("n_aimant"), TEXT("DISQUETTE"), TEXT('C'), TEXT("Aimant"), TEXT("Ramasse le butin de plus loin"), {  }, false, TEXT("") },
		{ TEXT("n_plaque"), TEXT("DISQUETTE"), TEXT('R'), TEXT("Plaque de blindage"), TEXT("+2 PV max, -6% vitesse"), {  }, false, TEXT("") },
		{ TEXT("n_adn"), TEXT("DISQUETTE"), TEXT('R'), TEXT("Catalyseur ADN"), TEXT("+25% d ADN gagne"), {  }, false, TEXT("") },
		{ TEXT("n_scrap"), TEXT("DISQUETTE"), TEXT('R'), TEXT("Recuperateur"), TEXT("Les ennemis lachent plus de scrap"), {  }, false, TEXT("") },
		{ TEXT("j_pierce"), TEXT("PERK"), TEXT('R'), TEXT("Balles perforantes"), TEXT("Chaque tir traverse 1 ennemi de plus"), { TEXT("standard"), TEXT("rafale"), TEXT("shotgun") }, false, TEXT("") },
		{ TEXT("j_bounce"), TEXT("PERK"), TEXT('R'), TEXT("Ricochet"), TEXT("Les tirs rebondissent une fois sur les murs"), { TEXT("standard"), TEXT("rafale"), TEXT("shotgun") }, false, TEXT("") },
		{ TEXT("j_multi"), TEXT("PERK"), TEXT('E'), TEXT("Double canon"), TEXT("Un projectile de plus, cote a cote"), { TEXT("standard"), TEXT("rafale"), TEXT("shotgun") }, false, TEXT("") },
		{ TEXT("k_long"), TEXT("PERK"), TEXT('C'), TEXT("Bras long"), TEXT("+25% portee de la griffe"), { TEXT("griffe") }, false, TEXT("") },
		{ TEXT("k_prise"), TEXT("PERK"), TEXT('C'), TEXT("Prise"), TEXT("La griffe ramene les cibles plus fort"), { TEXT("griffe") }, false, TEXT("") },
		{ TEXT("k_fauche"), TEXT("PERK"), TEXT('R'), TEXT("Fauche"), TEXT("Toutes les 3 frappes, un grand arc au corps a corps"), { TEXT("griffe") }, true, TEXT("") },
		{ TEXT("k_rayon"), TEXT("PERK"), TEXT('R'), TEXT("Rayon latent"), TEXT("Toutes les 4 frappes, la griffe libere un rayon"), { TEXT("griffe") }, true, TEXT("") },
		{ TEXT("k_vorace"), TEXT("PERK"), TEXT('R'), TEXT("Vorace"), TEXT("Mute, un kill rend 1 PV une fois sur 4"), { TEXT("griffe") }, true, TEXT("") },
		{ TEXT("k_double"), TEXT("PERK"), TEXT('E'), TEXT("Double griffe"), TEXT("La griffe frappe deux fois"), { TEXT("griffe") }, true, TEXT("") },
		};
		return Items;
	}

	const FEONItemDef* Find(FName Id)
	{
		for (const FEONItemDef& It : All()) { if (It.Id == Id) { return &It; } }
		return nullptr;
	}

	float Weight(TCHAR Rar) { return Rar == 'E' ? 10.f : Rar == 'R' ? 30.f : 60.f; }
	FLinearColor Color(TCHAR Rar) { return FLinearColor(Rar == 'E' ? FColor(255, 194, 61) : Rar == 'R' ? FColor(62, 242, 224) : FColor(207, 211, 255)); }
	FString RarName(TCHAR Rar) { return Rar == 'E' ? TEXT("Epique") : Rar == 'R' ? TEXT("Rare") : TEXT("Commun"); }
	int32 Price(const FEONItemDef& It) { return (It.Rar == 'E' ? 28 : It.Rar == 'R' ? 16 : 8) + (It.Cat == TEXT("GREFFE") ? 6 : 0); }

	TArray<FName> Roll(int32 N, const AEONCharacter* J, bool bLevel)
	{
		const FName G = J ? J->GetGreffeId() : FName(TEXT("standard"));
		TArray<const FEONItemDef*> Pool;
		for (const FEONItemDef& It : All())
		{
			if (bLevel && It.Cat != TEXT("PERK") && It.Cat != TEXT("DISQUETTE")) { continue; }
			// LOCK_G : Jira a déjà ses 5 greffes en main, les objets Greffe ne sortent pas (voir MANQUANTS.md).
			if (It.Cat == TEXT("GREFFE")) { continue; }
			if (It.Gr.Num() && !It.Gr.Contains(G)) { continue; }
			if (It.bOnce && J && J->HasItem(It.Id)) { continue; }
			if (It.Id == TEXT("c_shield") && J && J->GetShieldMax() > 0) { continue; }
			Pool.Add(&It);
		}
		TArray<FName> Out;
		while (Out.Num() < N && Pool.Num())
		{
			float Tot = 0.f;
			for (const FEONItemDef* It : Pool) { Tot += Weight(It->Rar); }
			float R = FMath::FRand() * Tot;
			int32 K = 0;
			for (; K < Pool.Num(); ++K) { R -= Weight(Pool[K]->Rar); if (R <= 0.f) { break; } }
			K = FMath::Min(K, Pool.Num() - 1);
			Out.Add(Pool[K]->Id);
			Pool.RemoveAt(K);
		}
		return Out;
	}
}
