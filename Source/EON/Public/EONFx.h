// EON — Effets de particules du proto, en Niagara (systèmes faits par Saved/Scripts/eon_vfx_build.py).
// Le proto n'a qu'un modèle de particule, part(x, y, vx, vy, vie, couleur, taille, frein) : un carré qui file en ligne
// droite, freiné, sans gravité. Chaque appel ici reprend une ligne du proto avec ses chiffres (px, px/s, s) ;
// la conversion en cm se fait dedans. En 3D, les éclats partent en demi-sphère vers le haut (le proto est à plat).
//   Gerbe    : NS_EON_Gerbe, toutes directions.
//   GerbeDir : NS_EON_GerbeDir, cône de ±63° autour de Dir (« atan2(dy,dx) + (rand-0,5)·2,2 » du proto).
//   Anneau   : NS_EON_Anneau, cercle à plat qui grandit de 0 à R en s'effaçant ({ring:true} du proto).
// Les vitesses du proto vont de « s » à « s + r » : on donne la vitesse max, le système tire entre 25 et 100 %.
#pragma once

#include "CoreMinimal.h"

namespace EONFx
{
	/** Couleur du proto (« #ff7a1a » ou « ff7a1a »). */
	EON_API FLinearColor Hex(const TCHAR* Hex);

	/** N éclats de couleur Col. SpeedPx = vitesse max (px/s), Life (s), SizePx (côté du carré, px), Drag (frein du proto), RadiusPx (dispersion du départ). */
	EON_API void Gerbe(const UObject* Ctx, const FVector& At, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float SizePx = 1.f, float Drag = 4.f, float RadiusPx = 0.f);

	/** Même chose dans un cône autour de Dir (direction dans le monde). */
	EON_API void GerbeDir(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, const FLinearColor& Col, float SpeedPx, float Life, float SizePx = 1.f, float Drag = 4.f);

	/** Gerbe à deux couleurs : « i%Every ? A : B » du proto (B une fois sur Every). */
	EON_API void Gerbe2(const UObject* Ctx, const FVector& At, int32 N, int32 Every, const FLinearColor& A, const FLinearColor& B, float SpeedPx, float Life, float SizePx = 1.f, float Drag = 4.f, float RadiusPx = 0.f);
	EON_API void GerbeDir2(const UObject* Ctx, const FVector& At, const FVector& Dir, int32 N, int32 Every, const FLinearColor& A, const FLinearColor& B, float SpeedPx, float Life, float SizePx = 1.f, float Drag = 4.f);

	/** Anneau de rayon final RadiusPx, à plat au sol à At. */
	EON_API void Anneau(const UObject* Ctx, const FVector& At, float RadiusPx, float Life, const FLinearColor& Col);
}
