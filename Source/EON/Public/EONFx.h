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

class AEONFxPiece;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

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

	// ---- Effets en maillage (AEONFxPiece, matériau M_EON_Glow), ajoutés le 24/11 pour les effets « ABS ».
	/** Formes de M_EON_Glow (paramètre Forme). */
	enum class EForme : uint8 { Uni = 0, Rond = 1, Trainee = 2, Anneau = 3, Disque = 4, Cone = 5, Ligne = 6, Flaque = 7 };
	/** Matériau lumineux additif (Color × Intensite × forme × Opacite). */
	EON_API UMaterialInstanceDynamic* Glow(UObject* Outer, EForme Forme, const FLinearColor& Col, float Intensite = 1.f);
	/** Lumière ponctuelle qui décroît (carré de la vie restante). */
	EON_API void Lumiere(const UObject* Ctx, const FVector& At, const FLinearColor& Col, float Intensity, float RadiusPx, float Life);
	/** Flash de bouche du proto : halo carré de 10 px couleur Col (35 %), trait blanc de 6 px vers Dir, cœur blanc de 4 px.
	 *  LightRadiusPx > 0 : lumière en plus (tirs ennemis ; Jira a déjà la sienne, JIR-V1). */
	EON_API void Eclair(const UObject* Ctx, const FVector& At, const FVector& Dir, const FLinearColor& Col, float Life, float Scale = 1.f, float LightRadiusPx = 0.f);
	/** Fantôme : copie de la pose de Src, couleur Col additive à Alpha, qui s'efface sur Life. */
	EON_API void Fantome(const UObject* Ctx, USkeletalMeshComponent* Src, const FLinearColor& Col, float Alpha, float Life);
	/** Douille du proto : 2 × 1 px ambre #ffc23d, lancée à Vel (cm/s), tombe (260 px/s²), rebondit, reste 4 s. */
	EON_API void Douille(const UObject* Ctx, const FVector& At, const FVector& Vel, float FloorZ);
	/** Forme au sol (annonce, flaque) : centre At, Yaw (deg, axe +X de la forme), taille en px, durée, clignotement (Hz).
	 *  bFill : la partie pleine grandit de 0 à 1 sur la durée. Angle = demi-angle du cône (rad). */
	EON_API AEONFxPiece* Sol(const UObject* Ctx, EForme Forme, const FVector& At, float Yaw, float SizeXPx, float SizeYPx, const FLinearColor& Col, float Life,
		float Clignote = 0.f, bool bFill = false, float Angle = 0.6f, float Graine = 0.f, float Intensite = 1.5f);
}
