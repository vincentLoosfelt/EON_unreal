// EON — sons d'effets.
// - Placeholders CC0 (Kenney) et sons du proto : /Game/EON/Audio/Placeholders, sources dans Audio_Sources/Placeholders.
// - Pack « Old-School Shonen SFX » (Helton Yan, CC BY 4.0, crédit obligatoire) : /Game/EON/Audio/OldSchoolShonen/<Famille>/SFX_Shonen_<Famille>_NN.
// Jeu vu de dessus : sons à plat (2D), sauf ceux joués depuis un ennemi, placés gauche / droite autour de Jira (réglage SonSpatial).
// Volume : Sound, Vol et les groupes VolJira, VolImpacts, VolEnnemis, VolMonde de DT_GameFeel (menu F6, rubrique Son).
#pragma once

#include "CoreMinimal.h"

class AActor;

namespace EONSfx
{
	/** Joue un son par son nom d'asset. Variante au hasard parmi <Name>_01.._<Variants> si Variants > 0.
	 *  Les noms SFX_Shonen_<Famille>_NN sont cherchés dans OldSchoolShonen/<Famille>, les autres dans Placeholders.
	 *  MinGap : écart minimal (s) entre deux lectures de ce son (volées ennemies). */
	EON_API void Play(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.f, int32 Variants = 0, float PitchVar = 0.06f, float MinGap = 0.f);

	/** Joue le son d'un évènement du jeu, par sa clé du proto (sfx('hit'), sfx('kill')…).
	 *  La table clé -> son est dans EONSfx.cpp : c'est le seul endroit à changer pour réattribuer un son.
	 *  VolMul multiplie le volume de la table. */
	EON_API void Event(const UObject* WorldContext, const TCHAR* Key, float VolMul = 1.f);

	/** Pas : à appeler à chaque tick. Joue l'évènement « step » selon la distance parcourue au sol
	 *  (un pas tous les 110 cm en marchant, 150 cm en courant). */
	EON_API void Steps(const AActor* Actor, float Dt, bool bOnGround, bool bRunning);
}
