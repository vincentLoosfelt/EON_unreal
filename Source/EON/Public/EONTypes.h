// EON — types partagés : échelle proto -> Unreal et lignes des DataTables.
// Les DataTables sont générées depuis Docs/PASSATION_UNREAL/DONNEES/*.json
// par Saved/Scripts/eon_datatables.py : les JSON restent la source.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EONTypes.generated.h"

namespace EON
{
	/** 1 tuile = 16 px proto = 100 cm. Toute conversion passe par ici. */
	constexpr float CmPerPx = 6.25f;
	constexpr float PxPerTile = 16.f;
	constexpr float CmPerTile = 100.f;

	/** Écran interne du proto (px). */
	constexpr float ProtoViewW = 480.f;
	constexpr float ProtoViewH = 270.f;

	/** Hauteur du bras de Jira au-dessus des pieds (AIMY du proto, px). */
	constexpr float ArmHeightPx = 17.f;

	FORCEINLINE float PxToCm(float Px) { return Px * CmPerPx; }
	FORCEINLINE float CmToPx(float Cm) { return Cm / CmPerPx; }
}

/** Réglages du banc de game feel (reglages_gamefeel.json). Unités proto : px, s, ms. */
USTRUCT(BlueprintType)
struct EON_API FEONGameFeelRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement") float RunMul = 1.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact") float Shake = 1.f;
	/** ms */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact") float Hitstop = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact") float Knock = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact") bool HitFlash = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact") bool DmgNums = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") float BulletSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") float Spread = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") float RateMul = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") float Recoil = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") bool Flash = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") bool Shells = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Caméra") float CamKick = 1.f;
	/** px d'anticipation de la caméra vers la visée */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Caméra") float Lookahead = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") bool Crt = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Énergie") float EnergyCost = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutation") float MutDur = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir") float AimAssist = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vide") float FallDmg = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss") float BossHp = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutation") float ArmBlock = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutation") float ArmCost = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutation") float ArmMut = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") bool Light = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") float Dark = 0.55f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") float Paint = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") float Holes = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Énergie") float Regen = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") bool Ambient = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interface") bool LockUI = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mouvement") float DashMul = 1.f;
	// Post-process des cartes (PP_Etage, PP_HUB) : PPPaint = post-process anime (M_PP_EON_Trait + M_PP_EON_Film), PPAll = tout le volume (exposition, bloom).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") bool PPPaint = false;   // DT_GameFeel : True depuis le 23/11 (trait celluloïd)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") bool PPAll = true;
	// Celluloïds animés en paliers (Vincent, 24/11 : « les persos c'est toujours 12 fps », comme Orbitals) ; 0 = fluide.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") float CelAnimFps = 12.f;
	// Émission des néons des lampes de salle (AEONLampe), multipliée (Vincent, 24/11 : « les matériaux lumineux sont trop forts »).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") float NeonMul = 0.12f;
	// Lumière ponctuelle des lampes de salle, multipliée (Vincent, 24/11 : « la lumière est trop forte »).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendu") float LampeMul = 0.15f;
	// Texte de debug en haut à gauche (entrée, énergie, vitesse...) : caché par défaut (Vincent, 15/11), menu Tweak.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interface") bool DebugText = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis") float EnemyHp = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis") float EnemySpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis") float EnemyDmg = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis") float WaveMul = 1.f;
	// Triche (menu F7) : gardées quand on change de préréglage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triche") bool God = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triche") bool InfEnergy = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triche") bool InfDash = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Triche") bool OneShot = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Son") bool Sound = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Son") float Vol = 0.5f;

	// --- Multiplicateurs d'écart 3D (MANQUANTS.md §9). 1 = valeur du proto. ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Écarts 3D") float SpeedMul3D = 0.55f;   // Vincent 14/11 : Jira trop rapide (MANQUANTS §9)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Écarts 3D") float RangeMul3D = 1.f;

	// --- Caméra 3D (pas dans le proto : choix du portage) ---
	/** Plongée de la caméra, degrés vers le bas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Caméra 3D") float CamPitch = 55.f;
	/** Champ horizontal, degrés. Petit = longue focale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Caméra 3D") float CamFov = 25.f;
	/** Largeur de sol visible au centre de l'image, en tuiles (proto : 30). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Caméra 3D") float CamViewTiles = 16.f;   // Vincent, 24/11 : 16 tuiles (caméra plus proche ; proto : 30)
};

/** Une greffe (greffes.json). */
USTRUCT(BlueprintType)
struct EON_API FEONGreffeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
	/** auto, burst, beam, melee, spread, lash */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Type;
	/** tirs/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Rate = 0.f;
	/** s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Charge = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinShots = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxShots = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Dmg = 0.f;
	/** px/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Spd = 0.f;
	/** degrés (cône total) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Spread = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cost = 0.f;
	/** s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Life = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Kick = 0.f;
	/** px */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Len = 0.f;
	/** px */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 0.f;
	/** radians */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Arc = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Pellets = 0;
	/** largeur de la Griffe, px */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float W = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Pull = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Col = FLinearColor::White;
	/** Ordre de Jira (1 à 5), 0 = hors ordre (greffes du Cobaye). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 JiraOrder = 0;
};

/** Un ennemi (ennemis.json). */
USTRUCT(BlueprintType)
struct EON_API FEONEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString OfficialName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Hp = 1.f;
	/** rayon de collision au sol, px */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float R = 5.f;
	/** px/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Spd = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Mass = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool Contact = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool Fly = false;
	/** hauteur / rayon de la capsule de hitbox, px */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HH = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HR = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Adn = 0;
};

