// EON — directeur de combat (jalon 3). Tient ce que le proto garde en global pour la salle courante :
// enemies, spawns, ebullets, eggs, nums, et l'état de vague de la salle (waves, wave, startT, cleared).
// Portage de spawnWave, walkableTiles, la section « vagues » de update(), killEnemy, aoe, et la relance de drawDead.
//
// Deux modes :
//  - avec AEONFloorManager (cas normal) : il écoute OnRoomLoaded ; rôles, vagues, startT, combatN, floorN
//    et le verrou viennent de l'étage (loadRoom y est déjà porté) ; la salle finie est rendue par MarkCurrentRoomCleared.
//  - sans étage (carte de test avec des AEONTileRoom posées) : il suit Jira de salle en salle (à 20 px des murs)
//    et applique lui-même les rôles de loadRoom.
// Console : eon.wave [n] | eon.combat [vagues] | eon.kill | eon.combatn <n>
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONCombatDirector.generated.h"

class AEONEnemy;
class AEONEnemyProjectile;
class AEONEnemyEgg;
class AEONTileRoom;
class AEONCharacter;
class AEONFloorManager;

UCLASS(NotBlueprintable)
class EON_API AEONCombatDirector : public AActor
{
	GENERATED_BODY()

public:
	AEONCombatDirector();

	/** Le directeur du monde (créé s'il n'existe pas). */
	static AEONCombatDirector* Get(UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

	/** loadRoom (partie combat) : vide la salle précédente puis lance celle-ci selon son rôle (mode sans étage). */
	UFUNCTION(BlueprintCallable, Category = "EON|Combat")
	void EnterRoom(AEONTileRoom* Room);
	/** spawnWave(mul) dans la salle courante. */
	UFUNCTION(BlueprintCallable, Category = "EON|Combat")
	void SpawnWave(float Mul);
	/** Force une salle Combat dans la salle courante (verrouillée, Waves vagues, départ 0,6 s). */
	UFUNCTION(BlueprintCallable, Category = "EON|Combat")
	void StartCombat(int32 Waves, float StartT = 0.6f);
	/** Plus aucun ennemi vivant ni à venir dans la salle (le choix de niveau peut s'ouvrir). */
	bool IsQuiet() const;
	/** killEnemy pour tous les ennemis vivants. */
	UFUNCTION(BlueprintCallable, Category = "EON|Combat")
	void KillAll();
	/** Nouvelle run après « Signal perdu ». */
	UFUNCTION(BlueprintCallable, Category = "EON|Combat")
	void RestartRun();

	// --- Appelés par les ennemis
	void KillEnemy(AEONEnemy* E, float Dx, float Dy);
	/** Ajoute un ennemi déjà créé à la salle courante (le gardien). */
	void AddEnemy(AEONEnemy* E) { if (E) { Enemies.Add(E); } }
	/** aoe(x, y, R, dmg, hurtsPlayer) du proto, en px salle. */
	void Aoe(const FVector2D& At, float RPx, float Dmg, bool bHurtsPlayer, const FLinearColor& Col);
	/** ebullets.push : ScreenPx = position du proto (y déjà remontée de HeightPx). */
	void FireEnemyBullet(const FVector2D& ScreenPx, const FVector2D& VelPx, float Life, bool bBig, float HeightPx);
	void LayEgg(const FVector2D& FromScreenPx, const FVector2D& ToPx);
	int32 NumEggs() const { return Eggs.Num(); }
	/** Efface les tirs ennemis (position px) pour lesquels Pred est vrai : Lame, onde. Renvoie le nombre effacé. */
	int32 ClearBullets(TFunctionRef<bool(const FVector2D&)> Pred);
	/** shieldBurst : Shield NV3 A (onde de 80 px, 5 dégâts / 8 sur un boss, tirs effacés à 110 px) ou B (tirs à 120 px renvoyés). */
	void ShieldBurst(bool bMirror);
	/** Chute d'un ennemi : « Chute +1 », +1 scrap, ADN. */
	void OnEnemyFell(AEONEnemy* E);
	/** Chiffre flottant (nums du proto), dessiné par AEONHUD en lettres manga (police Bangers, Vincent 25/11). */
	struct FNum
	{
		FVector Loc = FVector::ZeroVector;
		float VelZ = 0.f;
		float Life = 0.f;
		float Life0 = 0.f;
		/** 1 = coup blanc ; plus gros pour un coup orange (1,3) ou critique (1,8). */
		float Taille = 1.f;
		FString Text;
		FColor Col = FColor::White;
	};
	/** VyPx < 0 = monte. Taille : échelle du chiffre (gros coups plus gros). */
	void AddNum(const FVector& World, const FString& Text, const FColor& Col, float Life = 0.6f, float VyPx = -38.f, float Taille = 1.f);
	const TArray<FNum>& GetNums() const { return Nums; }
	void Shake(float V) const;
	void Hitstop(float Ms) const;
	/** hurtPlayer(amt, sx, sy), source en px salle. Pousse de PushPx (px/s) en plus si non nul (Brainless). */
	void HurtJira(int32 Amount, const FVector2D& SourcePx, const FVector2D& PushPx = FVector2D::ZeroVector);
	/** Jira peut être touchée (ni invulnérable, ni en dash, ni en chute, ni morte). */
	bool JiraHittable() const;

	const TArray<TObjectPtr<AEONEnemy>>& GetEnemies() const { return Enemies; }
	/** Toutes les vagues de la salle sont parties et il ne reste ni ennemi vivant ni apparition en cours. */
	bool IsFightOver() const;
	/** Vertical slice (Vincent, 24/11) : Petite Nuke, Brainless et Méduse sont isolés. Leur code reste, mais ils ne
	 *  sont jamais lancés. Renvoie le type réellement lancé : gouleP -> goule (œufs d'Ubume), brainless -> slasher
	 *  (événements, élite), meduse -> NAME_None (rien). Pour les rendre : bVerticalSlice = false dans EONCombatDirector.cpp. */
	static FName VSType(FName Type);
	/** Fait apparaître un ennemi tout de suite (events : la chose, le dormeur des pièces…). */
	AEONEnemy* SpawnEnemyAt(FName Type, const FVector2D& Px, bool bElite);
	/** Lance des vagues dans la salle en cours (Cartographe : 2 vagues après son dialogue). */
	void StartRoomWaves(int32 Waves, float StartT);
	AEONTileRoom* GetRoom() const { return CurRoom.Get(); }
	FVector2D GetJiraPx() const { return JiraPx; }
	AEONCharacter* GetJira() const;
	/** combatN / floorN : ceux de l'étage s'il existe, sinon les nôtres. */
	int32 GetCombatN() const;
	int32 GetFloorN() const;

	// --- Compteurs de la run (P.kills, P.scrap, ADN, Données ; les ramassables arrivent au jalon 6 : crédités d'office)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Combat") int32 Kills = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Combat") int32 Scrap = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Combat") int32 AdnGained = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Combat") int32 Donnees = 0;
	/** combatN du mode sans étage, ou forcé par eon.combatn (-1 = suivre l'étage). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Combat") int32 CombatN = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Combat") int32 CombatNOverride = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Combat") int32 FloorN = 1;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	/** État de vague d'une salle (room.cleared / waves / wave / startT / locked du proto). */
	struct FRoomState
	{
		bool bCleared = true;
		bool bCounted = false;
		bool bLocked = false;
		int32 Waves = 0;
		int32 Wave = 0;
		float StartT = 0.f;
		FName Role;
	};
	struct FSpawn
	{
		FName Type;
		FVector2D Pos = FVector2D::ZeroVector;
		float T = 0.f;
		bool bElite = false;
	};
	AEONFloorManager* GetFloor() const;
	void BindFloor();
	void OnFloorRoomLoaded(AEONFloorManager* Floor);
	void ClearRoomContents();
	void UpdateWaves(float Dt);
	void UpdateSpawns(float Dt);
	void UpdateBullets(float Dt);
	void UpdateEggs(float Dt);
	void UpdateNums(float Dt);
	void UpdateDeathScreen();
	void SpawnEnemyNow(const FSpawn& S);
	/** walkableTiles(minD) : sol atteignable depuis les portes, sans vide adjacent, à minD px de Jira (centres de tuiles). */
	void WalkableTiles(float MinD, TArray<FVector2D>& Out) const;
	float RoleMul(bool bFirst) const;
	void Banner(const FString& Text, const FColor& Col) const;

	TWeakObjectPtr<AEONTileRoom> CurRoom;
	/** Mode sans étage : état par salle posée. */
	TMap<TWeakObjectPtr<AEONTileRoom>, FRoomState> States;
	/** État de la salle courante (copié de l'étage à chaque chargement, ou de States). */
	FRoomState Cur;
	TWeakObjectPtr<AEONFloorManager> BoundFloor;
	FDelegateHandle FloorHandle;

	UPROPERTY() TArray<TObjectPtr<AEONEnemy>> Enemies;
	UPROPERTY() TArray<TObjectPtr<AEONEnemyProjectile>> Bullets;
	UPROPERTY() TArray<TObjectPtr<AEONEnemyEgg>> Eggs;
	TArray<FSpawn> Spawns;
	bool bDecoy = false;
	FVector2D DecoyPx = FVector2D::ZeroVector;
	TArray<FNum> Nums;

	/** Élites : jamais deux de suite (eliteCool, ELITE_PAUSE = 2 salles). */
	int32 EliteCool = 0;
	FVector2D JiraPx = FVector2D::ZeroVector;
	/** Temps réel de la mort de Jira (-1 = vivante). */
	double DeadSince = -1.0;
	int32 Falls0 = 0;
	float Time = 0.f;
};
