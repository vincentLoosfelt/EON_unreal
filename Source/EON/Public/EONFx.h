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

	/** Arrêt sur image (Vincent, 24/11 : « 15 % comme le proto ») : pendant le gel global (0,0001), les effets EONFx
	 *  continuent à 15 % de leur vitesse. Appelé par UEONTuningSubsystem au début et à la fin du gel. */
	EON_API void SetHitstop(UWorld* W, bool bOn);

	/** Fumée toon façon Akira (Vincent, 24/11) : N sphères NS_EON_Fumee (shader 2 tons M_EON_FumeeToon, trait celluloïd)
	 *  qui montent, tourbillonnent, gonflent puis s'érodent. Scale : taille d'ensemble (1 = bouffée de ~1 m).
	 *  Animée à 12 i/s : le temps de l'effet avance par bonds de 1/12 s, comme les persos. */
	EON_API void Fumee(const UObject* Ctx, const FVector& At, int32 N, const FLinearColor& Col, float Life, float Scale = 1.f);

	/** Fumée façon Akira (Vincent, 25/11 : la fumée de l'explosion, sortie en effet à part, appelable par n'importe
	 *  quel effet) : gros nuage sombre NS_EON_FumeeExplo/FumeeExploLobes, masses qui s'excentrent depuis le centre,
	 *  dérivent vers le haut et s'érodent à bords nets, 12 i/s. RadiusCm : taille de la source (explosion : son rayon).
	 *  Life 5,2 s : le nuage a disparu vers 80 % de sa vie (~4 s). */
	EON_API void FumeeAkira(const UObject* Ctx, const FVector& At, float RadiusCm, const FLinearColor& Col = FLinearColor(0.027f, 0.019f, 0.030f), float Life = 5.2f);

	/** Explosion façon Akira (Vincent, 24/11) : « une petite boule qui grossit (1 à 50 cm en 500 ms) puis explose
	 *  drastiquement pour laisser un nuage de fumée qui se dissipe sur 4 secondes », avec un flash de lumière qui projette des ombres noires.
	 *  ChargeS > 0 : la boule de charge (Charge) d'abord, puis l'éclat ; 0 : l'éclat tout de suite (la charge a déjà été
	 *  montrée, ex. l'Onibi qui s'arme). Éclat = flash + boule de feu en amas (principe de la fumée, NS_EON_Feu)
	 *  + lumière + fumée toon sombre de 4 s. At = centre au sol, RadiusCm = rayon de la boule de feu, Col = couleur du bord. */
	EON_API void Explosion(const UObject* Ctx, const FVector& At, float RadiusCm, const FLinearColor& Col, bool bFumee = true, float ChargeS = 0.f);
	/** Boule de charge : petite boule blanche bordée de Col (M_EON_Dome, bulles) posée au sol en At, qui grossit
	 *  de 1 à 50 cm de diamètre sur Duree s (0,5 s voulu par Vincent), à 12 i/s. RadiusCm est gardé pour l'appel (non utilisé). */
	EON_API AEONFxPiece* Charge(const UObject* Ctx, const FVector& At, float RadiusCm, const FLinearColor& Col, float Duree);
	/** Flash (Vincent, 24/11 : « une lumière extrêmement forte qui génère des ombres noires ») : lumière ponctuelle
	 *  énorme en At, ombres nettes, 2 images à 12 i/s (pleine puis au quart). RadiusCm = portée. */
	EON_API void Flash(const UObject* Ctx, const FVector& At, const FLinearColor& Col, float RadiusCm);

	/** Anneau de rayon final RadiusPx, à plat au sol à At. */
	EON_API void Anneau(const UObject* Ctx, const FVector& At, float RadiusPx, float Life, const FLinearColor& Col);

	// ---- Effets en maillage (AEONFxPiece, matériau M_EON_Glow), ajoutés le 24/11 pour les effets « ABS ».
	/** Formes de M_EON_Glow (paramètre Forme). */
	enum class EForme : uint8 { Uni = 0, Rond = 1, Trainee = 2, Anneau = 3, Disque = 4, Cone = 5, Ligne = 6, Flaque = 7 };
	/** Matériau lumineux additif (Color × Intensite × forme × Opacite). */
	EON_API UMaterialInstanceDynamic* Glow(UObject* Outer, EForme Forme, const FLinearColor& Col, float Intensite = 1.f);
	/** Lumière ponctuelle qui décroît (carré de la vie restante). */
	EON_API void Lumiere(const UObject* Ctx, const FVector& At, const FLinearColor& Col, float Intensity, float RadiusPx, float Life);
	/** Flash des tirs (Vincent, 25/11) : une étoile dessinée de la planche P4 (2 à 4 branches, 2 à 6 branches ; jamais
	 *  deux fois la même de suite), face caméra, teintée de Col avec un cœur clair, tenue une image à 12 i/s.
	 *  SizePx : taille de l'étoile (px du proto). LightRadiusPx > 0 : lumière en plus (tirs ennemis ; Jira a déjà la
	 *  sienne, JIR-V1). */
	EON_API void FlashTir(const UObject* Ctx, const FVector& At, const FVector& Dir, const FLinearColor& Col, float SizePx = 14.f, float LightRadiusPx = 0.f);
	/** Fantôme : copie de la pose de Src, couleur Col additive à Alpha, qui s'efface sur Life. */
	EON_API AEONFxPiece* Fantome(const UObject* Ctx, USkeletalMeshComponent* Src, const FLinearColor& Col, float Alpha, float Life);
	/** Rayon lumineux de From à To (Vincent, 24/11 : armes de Jira hors debug) : ruban couleur Col de largeur WidthCm,
	 *  cœur blanc au tiers, tourné vers la caméra autour de son axe, qui s'efface sur Life. */
	EON_API AEONFxPiece* Rayon(const UObject* Ctx, const FVector& From, const FVector& To, float WidthCm, const FLinearColor& Col, float Life, float Intensite = 3.f);
	/** Douille du proto : 2 × 1 px ambre #ffc23d, lancée à Vel (cm/s), tombe (260 px/s²), rebondit, reste 4 s. */
	EON_API void Douille(const UObject* Ctx, const FVector& At, const FVector& Vel, float FloorZ);
	/** Forme au sol (annonce, flaque) : centre At, Yaw (deg, axe +X de la forme), taille en px, durée, clignotement (Hz).
	 *  bFill : la partie pleine grandit de 0 à 1 sur la durée. Angle = demi-angle du cône (rad). */
	EON_API AEONFxPiece* Sol(const UObject* Ctx, EForme Forme, const FVector& At, float Yaw, float SizeXPx, float SizeYPx, const FLinearColor& Col, float Life,
		float Clignote = 0.f, bool bFill = false, float Angle = 0.6f, float Graine = 0.f, float Intensite = 1.5f);
	/** Giclée dessinée (planche P2, façon Akira, Vincent 25/11) : 8 images jouées sur Life, plan tourné vers la caméra,
	 *  l'image part de At et file dans le sens Dir (monde) vu à l'écran. SizeCm = côté de l'image. LiftCm : vers la caméra. */
	/** Éclats d'impact dessinés (planche P1, Vincent 25/11 : à la place des carrés blancs) : 8 images sur Life, plan
	 *  tourné vers la caméra et centré sur At, axe +X de l'image dans le sens Dir vu à l'écran. */
	EON_API void Eclats(const UObject* Ctx, const FVector& At, const FVector& Dir, float SizeCm, const FLinearColor& Col, float Life = 0.25f, float LiftCm = 0.f);
	EON_API void Giclee(const UObject* Ctx, const FVector& At, const FVector& Dir, float SizeCm, const FLinearColor& Col, float Life = 0.3f, float LiftCm = 0.f);
	/** Tache de sang au sol (planche P3, 6 formes) : posée à plat en At (sol), axe +X = Yaw (sens du tir), après Delay s,
	 *  reste Life s et s'érode la dernière seconde. 100 au plus (les plus anciennes partent). */
	EON_API void TacheSol(const UObject* Ctx, const FVector& At, float Yaw, float SizeCm, const FLinearColor& Col, float Delay = 0.15f, float Life = 10.f);

	/** Gros coup (Vincent, 25/11 : morts d'élite, explosions, ultime) : écran bichrome (noir + Col) 2 images à 12 i/s,
	 *  traits de vitesse radiaux centrés sur At (redessinés 12 fois par seconde) et onomatopée Mot de la planche P7
	 *  (0 BAM, 1 ZAP, 2 BOOM, 3 KRAK, 4 DOOM, 5 VRRM), le tout en 0,6 s. Un seul à la fois : un autre dans les 0,35 s
	 *  est ignoré. Dessiné par AEONHUD (traits, mot) et AEONPlayerCameraManager (bichrome). */
	struct FGrosCoup
	{
		FVector At = FVector::ZeroVector;
		FLinearColor Col = FLinearColor::White;
		int32 Mot = 0;
		double T0 = -100.0;
		float Rot = 0.f;
		int32 Graine = 0;
	};
	constexpr float GrosCoupDuree = 0.6f;
	constexpr float GrosCoupBichrome = 2.f / 12.f;
	EON_API void GrosCoup(const UObject* Ctx, const FVector& At, const FLinearColor& Col, int32 Mot);
	/** Gros coup en cours (nullptr sinon) et son âge en s (horloge des gros coups). */
	EON_API const FGrosCoup* GrosCoupActif(float* OutAge = nullptr);

	/** Onde de choc (Vincent, 25/11 ; façon Akira) : sphère de distorsion de l'image, au taux lié à un fresnel (nette au
	 *  centre, tordue sur le bord), qui part de At, s'élargit vite puis ralentit et s'éteint. Effet à part, appelable
	 *  par n'importe quel effet (le gros coup l'utilise). Dessinée par AEONPlayerCameraManager (M_PP_EON_OndeChoc),
	 *  4 au plus en même temps (la plus ancienne cède sa place).
	 *  Delai : départ après Delai s. Rayon : rayon final en hauteurs d'écran. Force : déviation max de l'image. */
	EON_API void OndeChoc(const UObject* Ctx, const FVector& At, float Delai = 0.f, float Duree = 0.43f, float Rayon = 0.94f, float Force = 0.08f);
	/** Même onde de choc, mais de rayon final RayonCm dans le monde (ex. le pouvoir E de Jira : rayon de l'onde).
	 *  Emission > 0 : léger éclat de teinte Teinte sur le bord de la sphère (même fresnel), pour en montrer la portée. */
	EON_API void OndeChocMonde(const UObject* Ctx, const FVector& At, float RayonCm, float Delai = 0.f, float Duree = 0.35f, float Force = 0.06f,
		float Emission = 0.f, const FLinearColor& Teinte = FLinearColor::White);
	/** Une onde de choc en cours, prête à dessiner : rayon (hauteurs d'écran, ou cm si RayonCm > 0) et force du moment. */
	struct FOndeChocVue
	{
		FVector At = FVector::ZeroVector;
		float Rayon = 0.f;
		float RayonCm = 0.f;
		float Force = 0.f;
		float Emission = 0.f;
		FLinearColor Teinte = FLinearColor::White;
	};
	EON_API void OndesChocActives(TArray<FOndeChocVue>& Out);
	constexpr int32 OndesChocMax = 4;

	/** Horloge des effets d'écran (gros coups, ondes de choc) : non dilatée (le hitstop ne la fige pas ; pas fixe en
	 *  tournage), avancée une fois par image par le HUD. */
	EON_API void AvancerHorlogeEcran(float Dt);
}
