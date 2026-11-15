// EON — salle en tuiles (jalon 2). Hérite de AEONRoomBounds : la caméra reste bornée à la salle.
// Pivot = centre de la salle au niveau du sol ; tuile (0,0) = coin haut-gauche (−X, −Y).
// Le gameplay du proto travaille en px « salle » (origine = coin haut-gauche, x = +X, y = +Y) :
// cette classe fournit tileAt / pitAt / solidAt / collideCircle / computeFlow dans ces unités.
#pragma once

#include "CoreMinimal.h"
#include "EONRoomBounds.h"
#include "EONTileGrid.h"
#include "EONTileRoom.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

UCLASS(Blueprintable)
class EON_API AEONTileRoom : public AEONRoomBounds
{
	GENERATED_BODY()

public:
	AEONTileRoom();

	// --- Génération (éditeur ou jeu)
	/** Rôle : COMBAT, TENSION, CALME, CORRIDOR, DEPART, OPPORTUNITE, ASCENSEUR, SECURITE, MARCHAND, FAILLE, EVENEMENT,
	 *  ou TRAINING (salle de réveil du Kibo), COM (salle du C.O.M). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle") FName RoomRole = TEXT("COMBAT");
	/** std, large, small, corH, corV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle") FName Shape = TEXT("std");
	/** Portes : N = 1, S = 2, E = 4, W = 8. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle", meta = (Bitmask)) int32 DoorMask = 1 | 2 | 4 | 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle") int32 Seed = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle") int32 FloorN = 1;

	/** Hauteur des murs (cm). Le proto n'a pas de hauteur : choix du portage, assez bas pour ne rien cacher en plongée. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") float WallHeightCm = 160.f;
	/** Épaisseur de la dalle de sol (cm, sous le niveau 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") float FloorThicknessCm = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> FloorMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> WallMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> CrateMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> TankMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> DoorMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> GapMaterial;

	// --- Salle dessinée à la main (Blueprint enfant de cette classe, rangé dans /Game/EON/Salles)
	/** Plan de la salle, une ligne par rangée de tuiles (de haut en bas, 1 tuile = 1 m) :
	 *  #  mur      .  sol      _  vide      =  fossé (vide franchissable au dash)
	 *  X  décor solide (caisse)      K  cuve      D  porte (ignoré : les portes sont posées au milieu de chaque côté)
	 *  Vide = salle générée par le proto. Le générateur d'étage prend ce plan pour RoomRole (et ExtraRoles) et Shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Plan") TArray<FString> Layout;
	/** Autres rôles que cette salle peut tenir dans un étage (en plus de RoomRole). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Plan") TArray<FName> ExtraRoles;
	/** Poids du tirage parmi les salles dessinées qui conviennent (0 = jamais tirée). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Plan", meta = (ClampMin = "0")) float Weight = 1.f;
	/** Remplit le plan avec la génération du proto (RoomRole, Shape, Seed, 4 portes) : point de départ à retoucher à la main. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "EON|Salle|Plan") void PlanDepuisGenerateur();
	bool HasLayout() const { return Layout.Num() > 0; }
	/** Cette salle dessinée peut-elle tenir ce rôle avec cette forme ? */
	bool CanServe(FName InRole, FName InShape) const;
	/** Plan -> grille, avec les portes de la case (masque EONDoor). Faux si le plan est inutilisable. */
	static bool LayoutToGrid(const TArray<FString>& Rows, uint8 Doors, FEONTileGrid& Out, FString* Err = nullptr);
	static void GridToLayout(const FEONTileGrid& G, TArray<FString>& Rows);

	// --- Murs du HUB (Unity) : façades WallA320 le long des murs qui donnent sur la salle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") bool bHubWalls = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UStaticMesh> HubWallMesh;
	/** Matériau des murs du HUB : walll painted d'Unity (vert-de-gris de MasterPainted), comme dans le HUB. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> HubWallMaterial;
	/** Emplacement « light » du mur (fines lignes lumineuses) : Room light d'Unity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> HubWallLightMaterial;
	/** Dessus des murs épais (vu de haut). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Salle|Rendu") TObjectPtr<UMaterialInterface> WallTopMaterial;

	/** Régénère la grille avec Role / Shape / DoorMask / Seed, puis reconstruit le décor. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "EON|Salle")
	void Generate();
	/** Pose une grille déjà faite (étage, salle de réveil…) et reconstruit le décor. */
	void SetGrid(const FEONTileGrid& InGrid);
	const FEONTileGrid& GetGrid() const { return Grid; }

	/** Portes verrouillées (salle Combat en cours) : les tuiles D deviennent solides. */
	UFUNCTION(BlueprintCallable, Category = "EON|Salle") void SetLocked(bool bInLocked);
	UFUNCTION(BlueprintPure, Category = "EON|Salle") bool IsLocked() const { return bLocked; }
	/** Porte rouge (redDoor du proto) : la porte vers le gardien reste fermée sans la carte. 0 = aucune. */
	void SetRedDoor(uint8 Door);
	uint8 GetRedDoor() const { return RedDoor; }
	/** Passerelle infestée (setInfest) : tuiles murées par la chair ; K = 1 pleine, 0 rétractée (solide tant que K > 0,35). */
	void SetInfest(const TArray<int32>& Tiles, float K);
	void SetInfestK(float K);
	float GetInfestK() const { return InfestK; }
	const TArray<int32>& GetInfestTiles() const { return InfestTiles; }

	// --- Repères (px salle <-> monde)
	FVector2D WorldToRoomPx(const FVector& World) const;
	FVector RoomPxToWorld(const FVector2D& Px, float Z = 0.f) const;
	/** Centre d'une tuile, en px salle. */
	static FVector2D TileCenterPx(int32 X, int32 Y) { return FVector2D((X + 0.5f) * 16.f, (Y + 0.5f) * 16.f); }

	// --- Requêtes du proto (px salle)
	EEONTile TileAtPx(const FVector2D& P) const;
	bool PitAtPx(const FVector2D& P) const { return TileAtPx(P) == EEONTile::Pit; }
	/** tileSolid() : murs, décor, cuves, portes si verrouillées ; vide si bPits. */
	bool IsTileSolid(int32 Index, bool bPits) const;
	bool SolidAtPx(const FVector2D& P) const;
	/** collideCircle(o, r, pits) : repousse le cercle hors des tuiles solides. Renvoie vrai s'il y a eu contact. */
	bool CollideCircle(FVector2D& PosPx, float RadiusPx, bool bPits) const;
	/** computeFlow() : distances en tuiles depuis la tuile de TargetPx, sur le sol et les portes (−1 = inaccessible). */
	void ComputeFlow(const FVector2D& TargetPx);
	int32 FlowAt(int32 X, int32 Y) const { return Grid.IsValidTile(X, Y) && Flow.IsValidIndex(Y * Grid.W + X) ? Flow[Y * Grid.W + X] : -1; }
	/** Tuiles de sol praticables (walkableTiles du proto : sol, hors bord). */
	void GetWalkableTiles(TArray<FIntPoint>& Out) const;

	/** Point d'entrée devant une porte (doorCenter du proto), en px salle. */
	FVector2D DoorCenterPx(uint8 Door) const;

	/** Salle en tuiles qui contient ce point (au sol), ou nullptr. */
	UFUNCTION(BlueprintPure, Category = "EON|Salle", meta = (WorldContext = "WorldContext"))
	static AEONTileRoom* FindTileRoomAt(const UObject* WorldContext, FVector Location);

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:
	void RebuildVisuals();
	void BuildHubWalls(const FVector& Origin);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> HubWallISM;
	void UpdateDoorCollision();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> FloorISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> GapISM;
	/** Sol invisible sous le vide : Jira reste en mode marche ; la chute est une règle du jeu (checkPlayerFall), pas de la physique. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> PitFloorISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> WallISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> CrateISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> TankISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> DoorISM;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> RedDoorISM;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> RedDoorMID;
	uint8 RedDoor = 0;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> InfestISM;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> InfestMID;
	TArray<int32> InfestTiles;
	float InfestK = 0.f;
	void RebuildInfest();
	TArray<int32> RedSet;

	UPROPERTY() FEONTileGrid Grid;
	TArray<int16> Flow;
	int32 FlowTile = -1;
	bool bLocked = false;
	bool bHasGrid = false;
};
