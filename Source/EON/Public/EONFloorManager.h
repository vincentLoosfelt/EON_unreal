// EON — étage (jalons 2 et 5) : genFloor() du proto (grille 7×7, C.O.M au centre, gardien sur un bras, rôles, formes, portes),
// passage de salle en salle par les portes (enterCell / loadRoom / startTrans), mémoire de chaque salle.
// Une seule salle existe à la fois, comme dans le proto : elle est reconstruite sur place à chaque passage de porte.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONTileGrid.h"
#include "EONFloorManager.generated.h"

class AEONTileRoom;
class AEONCharacter;

/** Une case de l'étage (cell du proto) et l'état de sa salle. */
USTRUCT(BlueprintType)
struct EON_API FEONFloorCell
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") FIntPoint Pos = FIntPoint::ZeroValue;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") FName Role;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") FName Shape = TEXT("std");
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") uint8 Doors = 0;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") bool bVisited = false;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") bool bSeen = false;
	// --- salle (buildRoom), construite à la première entrée
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") bool bBuilt = false;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") bool bCleared = false;
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") bool bCounted = false;
	/** Salle SECURITE : la carte rouge a été ramassée. */
	UPROPERTY(BlueprintReadOnly, Category = "EON|Étage") bool bCardTaken = false;
	/** Tuiles murées par la chair (passerelle du C.O.M vers le gardien, porte côté gardien). */
	UPROPERTY() TArray<int32> Infest;
	float InfestK = 0.f;
	/** Socles d'objets de la salle (Opportunité, Marchand, Trésor) : objet, prix, position px. Vidés quand on se sert. */
	UPROPERTY() TArray<FName> PedItems;
	UPROPERTY() TArray<int32> PedPrice;
	UPROPERTY() TArray<FVector2D> PedPx;
	/** Trésor : son butin a été posé (une seule fois). */
	bool bLootDone = false;
	bool bStock = false;
	UPROPERTY() FEONTileGrid Grid;
	/** Salle dessinée (Blueprint) tirée pour cette case ; vide = salle générée. */
	UPROPERTY() TSubclassOf<AEONTileRoom> RoomClass;
};

UCLASS(Blueprintable)
class EON_API AEONFloorManager : public AActor
{
	GENERATED_BODY()

public:
	AEONFloorManager();

	/** startFloor() : étage suivant (floorN++), génère l'étage et entre par le C.O.M. */
	UFUNCTION(BlueprintCallable, Category = "EON|Étage") void StartFloor();
	/** Nouvelle run : étage 0 puis StartFloor (le Kibo et la descente du C.O.M viendront au jalon 5). */
	UFUNCTION(BlueprintCallable, Category = "EON|Étage") void NewRun();

	UFUNCTION(BlueprintPure, Category = "EON|Étage") int32 GetFloorN() const { return FloorN; }
	UFUNCTION(BlueprintPure, Category = "EON|Étage") int32 GetCombatN() const { return CombatN; }
	UFUNCTION(BlueprintPure, Category = "EON|Étage") int32 GetRoomN() const { return RoomN; }
	UFUNCTION(BlueprintPure, Category = "EON|Étage") FName GetCurrentRole() const;
	UFUNCTION(BlueprintPure, Category = "EON|Étage") AEONTileRoom* GetRoom() const { return Room; }
	/** La salle en cours est-elle à faire (vagues, verrou) ? Lu par le directeur de combat. */
	UFUNCTION(BlueprintPure, Category = "EON|Étage") bool IsCurrentRoomCleared() const;
	/** Le directeur de combat signale la salle finie : déverrouille et la mémorise. */
	UFUNCTION(BlueprintCallable, Category = "EON|Étage") void MarkCurrentRoomCleared();

	/** Nombre de vagues et délai de départ de la salle en cours (loadRoom), 0 si rien à combattre. */
	int32 GetRoomWaves() const { return RoomWaves; }
	float GetRoomStartDelay() const { return RoomStartT; }
	/** Multiplicateur de vague (spawnWave(mul)) pour le rôle en cours : première vague puis suivantes. */
	float GetWaveMul(bool bFirst) const;

	/** Diffusé à chaque salle chargée (après placement de Jira). */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoomLoaded, AEONFloorManager*);
	FOnRoomLoaded OnRoomLoaded;

	static AEONFloorManager* Get(const UObject* WorldContext);

	const TMap<FIntPoint, FEONFloorCell>& GetCells() const { return Cells; }
	FIntPoint GetCurrentCell() const { return Cur; }
	/** Test : va directement dans la première salle du rôle donné (eon.salle). */
	bool GoToRole(FName InRole);
	/** Va dans la première salle de cette forme (eon.forme). */
	bool GoToShape(FName InShape);
	/** Le gardien est mort (AEONBoss) : fin de l'étage, chair rétractée, 2 objets au choix. */
	void OnBossDefeated(const FVector2D& BossPx);
	FIntPoint GetBossCell() const { return Boss; }

	/** Bannière centrale (showBanner, 1,8 s) : texte, couleur, temps restant. Lue par le HUD. */
	UFUNCTION(BlueprintCallable, Category = "EON|Étage") void ShowBanner(const FString& Text, FLinearColor Color);
	FString BannerText;
	FLinearColor BannerColor = FLinearColor::White;
	float BannerT = 0.f;

	/** Pilote automatique / eon.go : passe une porte de la salle (de préférence vers une case jamais visitée). Dir 0 = au choix. */
	UFUNCTION(BlueprintCallable, Category = "EON|Étage") bool AutoExit(uint8 Dir = 0);

	/** Carte rouge en main (P.redCard) : ouvre la porte rouge du gardien. Remise à zéro à chaque étage. */
	UFUNCTION(BlueprintPure, Category = "EON|Étage") bool HasRedCard() const { return bRedCard; }
	/** Gardien neutralisé (floor.done) : le levier du C.O.M est actif. */
	UFUNCTION(BlueprintPure, Category = "EON|Étage") bool IsFloorDone() const { return bFloorDone; }
	/** Le C.O.M descend (comTravel, 1,7 s puis étage suivant). */
	UFUNCTION(BlueprintPure, Category = "EON|Étage") bool IsComTravelling() const { return ComTravelT >= 0.f; }
	/** Jira est dans la salle du C.O.M, près de la sphère (le terminal). */
	UFUNCTION(BlueprintPure, Category = "EON|Étage") bool IsNearComTerminal() const;
	/** Touche Interagir : au C.O.M, tire le levier si le gardien est neutralisé, sinon dit ce qui manque. */
	UFUNCTION(BlueprintCallable, Category = "EON|Étage") bool TryInteract();
	/** La carte rouge est-elle posée dans la salle en cours ? Position en px de salle. */
	bool HasCardInRoom() const;
	FVector2D GetCardPx() const;
	/** Direction du gardien vue du C.O.M (« nord »...). */
	FString GuardDir() const;

	/** Le C.O.M à quai (room.dock) : seul endroit où l'on installe une cassette. Sans Kibo (MANQUANTS.md), provisoirement :
	 *  la salle du C.O.M de l'étage 1 tant que Jira n'en est pas sortie. */
	bool IsDocked() const;
	/** pullCOM : le levier (onglet Descente du terminal). */
	bool PullLever();
	/** Fondu entre deux salles en cours (trans). */
	bool IsInTransition() const { return TransT > 0.f; }
	/** C.O.M à quai au Kibo : début de run, avant le premier levier (FloorN = 0). */
	bool IsDockedMode() const { return bDocked; }
	/** Salle du Kibo où se trouve Jira : 0 aucune (étage), 1 salle de réveil, 2 passerelle, 3 C.O.M à quai. */
	uint8 GetKiboRoom() const { return bDocked ? Kibo : 0; }
	/** Cinématique de réveil en cours (2,6 s) : Jira ne répond pas. */
	/** Pendant la descente du C.O.M, la caméra reste à hauteur des passerelles : on voit le C.O.M bouger (Jira bouge avec lui). */
	float GetCamDz() const { return WakeLift > 0.f ? WakeLift : (ComTravelT >= 0.f || ComArrT >= 0.f) ? ComDz : 0.f; }
	/** Vue de développement (eon.vue) : caméra fixe posée par rapport au centre du C.O.M, pour filmer une séquence. */
	bool bDevView = false;
	FVector DevViewLoc = FVector::ZeroVector;
	FRotator DevViewRot = FRotator::ZeroRotator;
	float DevViewFov = 60.f;
	void SetDevView(const FVector& Rel, const FVector& LookRel, float Fov) { bDevView = true; DevViewLoc = HubCentre + Rel; DevViewRot = (LookRel - Rel).Rotation(); DevViewFov = Fov; }
	bool IsCinematic() const { return WakeT >= 0.f || ComTravelT >= 0.f || ComArrT >= 0.f; }
	/** Tests : eon.kibo reveil|passerelle|quai. */
	void GoKibo(uint8 Which);

	/** Hub 3D (L_HUB) : la salle du C.O.M est le hub lui-même ; ses 4 sas mènent aux salles générées, construites plus loin dans la carte. */
	bool IsHubMode() const { return bHub; }

	/** Plan texte de l'étage (console eon.map). */
	FString DescribeFloor() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void GenFloor();
	void EnterCell(FIntPoint Cell, uint8 FromDir, bool bFromCom);
	void LoadRoom(FEONFloorCell& C, uint8 EntryDoor, bool bFromCom);
	void StartTrans(TFunction<void()> Fn);
	AEONCharacter* GetJira() const;
	FEONFloorCell* CellAt(FIntPoint P) { return Cells.Find(P); }
	const FEONFloorCell* CellAt(FIntPoint P) const { return Cells.Find(P); }

	UPROPERTY(Transient) TObjectPtr<AEONTileRoom> Room;
	/** Salles dessinées à la main : Blueprints enfants d'AEONTileRoom rangés dans /Game/EON/Salles. */
	UPROPERTY(Transient) TArray<TSubclassOf<AEONTileRoom>> Templates;
	void LoadTemplates();
	/** Tire une salle dessinée pour ce rôle et cette forme (nullptr s'il n'y en a pas). */
	TSubclassOf<AEONTileRoom> PickTemplate(FName Role, FName Shape, uint8 Doors);
	/** Carte rouge posée sur sa plateforme (provisoire : un pavé rouge lumineux). */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CardMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> CardMaterial;
	bool bRedCard = false;
	bool bRedHint = false;
	bool bFloorDone = false;
	float RedBumpT = 0.f;
	float AmbushT = -1.f;
	float ComTravelT = -1.f;
	bool bComGo = false;
	/** Arrivée du C.O.M à l'étage suivant : il descend d'en haut et se cale à hauteur des passerelles. */
	float ComArrT = -1.f;
	bool bComArrive = false;
	/** Décalage vertical courant du C.O.M (cm, négatif = plus bas) ; ce qui bouge avec lui : COMv1 et le champ de force. */
	float ComDz = 0.f;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> ComMovers;
	TArray<FVector> ComMoverBase;
	void SetComDz(float Dz);
	void SetJiraFrozen(bool bFrozen);
	void SayFloorIntro();
	float InfBumpT = 0.f;
	bool bInfHint = false;
	/** P.introDone : le dialogue iaIntro joue au premier étage de chaque run. */
	bool bIntroDone = false;
	/** Marchands déjà salués (s.greeted). */
	TSet<FIntPoint> Greeted;
	void StartOrTitle();
	// --- Hub 3D comme salle du C.O.M
	bool bHub = false;
	FVector HubCentre = FVector::ZeroVector;
	/** Centre des 4 portes sas du hub, rangés N, S, E, W. */
	FVector HubDoor[4];
	UPROPERTY(Transient) TObjectPtr<class AStaticMeshActor> HubCube;
	UPROPERTY(Transient) TArray<TObjectPtr<class AStaticMeshActor>> HubFlesh;
	float HubFleshK = 0.f;
	static int32 DoorIdx(uint8 D) { return D == 1 ? 0 : D == 2 ? 1 : D == 4 ? 2 : 3; }
	void SetupHub();
	/** Passerelle du hub qui ne mène pas au gardien (cube du terminal et arrivée de Jira). */
	uint8 HubSafeDoor() const;
	FVector HubSafeDir() const;
	void LoadHub(uint8 Entry, bool bFromCom);
	void BuildHubFlesh();
	void TickHub(float DeltaSeconds, AEONCharacter* J);
	/** Kibo : le C.O.M à quai (hub 3D, les 4 sas fermés, levier prêt). */
	bool bDocked = false;
	void LoadDock();
	void BeginKibo();
	void EnterTraining(bool bCine);
	void EnterPasserelle(uint8 Entry);
	void LoadKiboGrid(const FEONTileGrid& G, uint8 Entry, const FVector2D* At = nullptr);
	void TickKibo(float DeltaSeconds, AEONCharacter* J);
	void ClearKiboActors();
	uint8 Kibo = 0;
	float WakeT = -1.f;
	// Réveil : hauteur de Jira au-dessus du sol (dans le tube, puis pendant sa chute) ; la caméra reste au sol.
	float WakeLift = 0.f;
	float WakeGroundZ = 0.f;
	// Contrainte de plan de Jira coupée pendant sa sortie de cuve (sinon elle ne retombe pas des 130 cm), remise au sol.
	bool bWakePlaneOff = false;
	bool bWakePlaneWas = false;
	FEONTileGrid KiboGrid;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> KiboActors;
	UPROPERTY(Transient) TObjectPtr<class AEONWakePod> JiraPod;
	void SetHubDoorsForced(int32 State);
	UPROPERTY(Transient) TArray<TObjectPtr<class AEONSlidingDoor>> HubDoorActors;
	/** Quai : une seule passerelle (sud). Les 3 autres (grilles, rambardes, mur percé, sas) sont retirées et leurs trous murés ; tout revient au départ du C.O.M. */
	void SetDockLayout(bool bOn);
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> DockRemoved;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> DockAdded;

	/** La passerelle infestée entre le C.O.M et le gardien est-elle encore fermée ? */
	bool IsInfestEdge(const FIntPoint& A, const FIntPoint& B) const;
	void OnPedestalTaken(class AEONPedestal* P, bool bPaid);
	UPROPERTY() TMap<FIntPoint, FEONFloorCell> Cells;
	FIntPoint Start = FIntPoint(3, 3);
	FIntPoint Boss = FIntPoint(3, 2);
	FIntPoint Cur = FIntPoint(3, 3);
	int32 FloorN = 0;
	int32 CombatN = 0;
	int32 RoomN = 0;
	int32 RoomWaves = 0;
	float RoomStartT = 0.f;
	bool bInfest = false;
	FRandomStream Rng;
	/** Transition (trans du proto, 0,36 s : fondu au noir puis chargement). */
	float TransT = 0.f;
	bool bTransDone = false;
	TFunction<void()> TransFn;
};
