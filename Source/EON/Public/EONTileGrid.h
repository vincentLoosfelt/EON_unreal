// EON — grille de tuiles d'une salle (jalon 2). Portage direct du proto v0.60.1 :
// T = {F:0, W:1, P:2, D:3, X:4, K:5}, SHAPES, doorTiles(), genTiles(), comTiles(), buildTraining().
// Tuile = 16 px = 100 cm. Repère : x vers la droite (= +X Unreal), y vers le bas de l'écran (= +Y Unreal).
#pragma once

#include "CoreMinimal.h"
#include "EONTileGrid.generated.h"

/** Types de tuile (T du proto). */
UENUM(BlueprintType)
enum class EEONTile : uint8
{
	Floor = 0  UMETA(DisplayName = "Sol (F)"),
	Wall = 1   UMETA(DisplayName = "Mur (W)"),
	Pit = 2    UMETA(DisplayName = "Vide (P)"),
	Door = 3   UMETA(DisplayName = "Porte (D)"),
	Crate = 4  UMETA(DisplayName = "Décor solide (X)"),
	Tank = 5   UMETA(DisplayName = "Cuve (K)"),
};

/** Portes d'une salle, en masque : N = 1, S = 2, E = 4, W = 8 (ordre d'insertion du proto : N, S, E, W). */
namespace EONDoor
{
	constexpr uint8 N = 1, S = 2, E = 4, W = 8;
	inline const TCHAR* Name(uint8 D) { return D == N ? TEXT("N") : D == S ? TEXT("S") : D == E ? TEXT("E") : TEXT("W"); }
	inline uint8 Opp(uint8 D) { return D == N ? S : D == S ? N : D == E ? W : E; }
	inline FIntPoint Dir(uint8 D) { return D == N ? FIntPoint(0, -1) : D == S ? FIntPoint(0, 1) : D == E ? FIntPoint(1, 0) : FIntPoint(-1, 0); }
}

USTRUCT(BlueprintType)
struct EON_API FEONTileGrid
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "EON|Salle") int32 W = 0;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Salle") int32 H = 0;
	/** W × H tuiles, ligne par ligne (index = y × W + x). */
	UPROPERTY(BlueprintReadOnly, Category = "EON|Salle") TArray<uint8> T;
	/** Vides « fossés » : franchissables au dash, comptés comme praticables pour la validation (gap du proto). */
	UPROPERTY(BlueprintReadOnly, Category = "EON|Salle") TSet<int32> Gap;
	/** Masque des portes (EONDoor). */
	UPROPERTY(BlueprintReadOnly, Category = "EON|Salle") uint8 Doors = 0;
	/** Variante tirée (feat du proto : cross, L, donut, notch, pillars, chasm, islands…). */
	UPROPERTY(BlueprintReadOnly, Category = "EON|Salle") FString Feat;

	bool IsValidTile(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < W && Y < H; }
	/** Hors grille = mur (tileAt du proto). */
	EEONTile Get(int32 X, int32 Y) const { return IsValidTile(X, Y) ? (EEONTile)T[Y * W + X] : EEONTile::Wall; }
	void Set(int32 X, int32 Y, EEONTile V) { if (IsValidTile(X, Y)) { T[Y * W + X] = (uint8)V; } }

	/** Tailles des formes (SHAPES) : std 32×20, large 40×26, small 24×16, corH 50×9, corV 12×32. */
	static FIntPoint ShapeSize(FName Shape);
	/** Les deux tuiles d'une porte (doorTiles). */
	static void DoorTiles(uint8 Door, int32 W, int32 H, FIntPoint Out[2]);

	/** genTiles(role, shape, doors). Rôles : COMBAT, TENSION, CALME, CORRIDOR, DEPART, OPPORTUNITE, ASCENSEUR,
	 *  SECURITE, MARCHAND, FAILLE, EVENEMENT. FloorN : numéro d'étage (le saule de l'étage 4 pousse la cuve). */
	static FEONTileGrid GenTiles(FName Role, FName Shape, uint8 Doors, FRandomStream& Rng, int32 FloorN = 1);
	/** Salle du C.O.M : plateforme ovale sur le vide, une passerelle par porte, la sphère (K) au centre. */
	static FEONTileGrid ComTiles(uint8 Doors);
	/** Salle de réveil du Kibo (buildTraining) : 36×20, fossé à dasher en x 27–28, trou 4×2, 7 capsules, porte nord. */
	static FEONTileGrid BuildTraining();
	/** Capsules de la salle de réveil (PODS) : coin haut-gauche en tuiles, 2×3 tuiles. */
	static const TArray<FIntPoint>& TrainingPods();
};
