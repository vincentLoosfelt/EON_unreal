// EON — sons d'effets (placeholders CC0 de Kenney, /Game/EON/Audio/Placeholders, sources dans Audio_Sources/Placeholders).
// Jeu vu de dessus : sons joués en 2D, hauteur légèrement variée, et un écart minimal entre deux lectures du même son.
#pragma once

#include "CoreMinimal.h"

namespace EONSfx
{
	/** Joue /Game/EON/Audio/Placeholders/<Name>. Variante au hasard parmi <Name>_01.._<Variants> si Variants > 0.
	 *  MinGap : écart minimal (s) entre deux lectures de ce son (volées ennemies). */
	EON_API void Play(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.f, int32 Variants = 0, float PitchVar = 0.06f, float MinGap = 0.f);
}
