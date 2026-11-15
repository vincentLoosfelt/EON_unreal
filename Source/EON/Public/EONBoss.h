// EON — le gardien de l'étage 1 : le Tentaculaire (jalon 7). Portage de spawnBoss, bossUpdate, startPhase,
// fanShot, addTent, addMortar, updateBossFx et bossDeath du proto v0.60.1 (eon-gunfeel.html).
// Comme les autres ennemis, il vit en px « salle » et c'est AEONCombatDirector qui l'appelle.
// Visuel provisoire en 3D (MANQUANTS.md §2, « Le Tentaculaire ») : corps sphérique en chair, un œil qui suit Jira,
// 4 tentacules au repos en chapelets de sphères, tentacules d'attaque et mortiers en formes simples.
#pragma once

#include "CoreMinimal.h"
#include "EONEnemy.h"
#include "EONBoss.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(NotBlueprintable)
class EON_API AEONBoss : public AEONEnemy
{
	GENERATED_BODY()

public:
	AEONBoss();

	/** spawnBoss(r) : devant la cuve de la salle (tuiles K), PV 170 × (1 + 0,35 × (étage − 1)). nullptr si la salle n'a pas de cuve. */
	static AEONBoss* SpawnInRoom(AEONCombatDirector* Director, AEONTileRoom* Room, int32 FloorN);
	/** Le gardien vivant (ou en train de mourir) du monde, s'il y en a un. */
	static AEONBoss* GetActive(const UObject* WorldContext);

	virtual void UpdateEnemy(float Dt, const FVector2D& TargetPx) override;
	virtual void UpdateVisual(float Dt, const FVector2D& TargetPx) override;
	virtual void HitPx(float Damage, float Dx, float Dy, float KnockMul = 1.f) override;
	virtual bool IsBoss() const override { return true; }

	// --- Lu par le HUD et la caméra
	FString GetBossName() const { return TEXT("Le Tentaculaire"); }
	FString GetBossSub() const { return TEXT("Specimen de la cuve - gardien de l'ascenseur"); }
	int32 GetPhase() const { return Phase; }
	bool IsInvulnerableNow() const { return bInv; }
	bool IsInIntro() const { return BSt == EBossSt::Intro; }
	bool IsDying() const { return bDying; }
	/** Carton-titre : temps restant (2,6 s au départ), 0 = aucun. */
	float GetTitleT() const { return TitleT; }
	/** Centre de la cuve (monde). */
	FVector GetTankWorld() const;

protected:
	virtual void Destroyed() override;

private:
	enum class EBossSt : uint8 { Intro, Fight, Trans, ChWind, Charge };
	struct FTent { float Ang = 0.f; float Max = 40.f; float Len = 0.f; uint8 St = 0; float T = 0.f; float TMax = 0.f; };   // St : 0 annonce, 1 frappe, 2 tenue, 3 retrait
	struct FMortar { FVector2D P; float R = 22.f; float T = 1.1f; float Max = 1.1f; };

	float AimAt(const FVector2D& Tg, bool bLead) const;
	void FanShot(int32 N, float Spread, float Speed);
	void Shot(float Ang, float Speed, float Life);
	void AddTent(float Ang, float Tele);
	void AddMortar(const FVector2D& At);
	void StartPhase(int32 N);
	void StartDeath();
	void FinishDeath();
	void ClearAttacks();
	void UpdateFx(float Dt);
	void UpdateMeshes(float Dt);
	UStaticMeshComponent* NewPart(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Mat);

	// --- État du proto
	EBossSt BSt = EBossSt::Intro;
	int32 Phase = 1;
	bool bInv = true;
	bool bDying = false;
	float BStT = 3.f;
	float FireT = 1.2f, SpT = 2.5f, MorT = 1.5f, TentT = 1.f, ChT = 3.f;
	float Osc = 0.f, SpA = 0.f, Cha = 0.f, Pop = 0.f;
	FVector2D Anchor = FVector2D::ZeroVector;
	FVector2D TankC = FVector2D::ZeroVector;
	FVector2D TankU = FVector2D(0.f, -1.f);
	FVector2D TankSize = FVector2D(64.f, 16.f);
	float Crack = 0.f;
	bool bTankBroken = false;
	float TitleT = 0.f;
	float DeathT = 0.f, DeathNext = 0.f;
	FVector2D JiraLast = FVector2D::ZeroVector;
	FVector2D JiraVel = FVector2D::ZeroVector;
	FVector2D LastTg = FVector2D::ZeroVector;
	TArray<FTent> Tents;
	TArray<FMortar> Mortars;
	int32 FloorN = 1;

	// --- Visuel
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Eye;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Pupil;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RestTents;      // 4 × 6 sphères
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TentMeshes;     // tentacules d'attaque (cylindres)
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TeleMeshes;     // annonces (lignes rouges au sol)
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> MortarMeshes;   // cercles au sol
	UPROPERTY() TObjectPtr<UStaticMeshComponent> TankGlass;              // vitre de la cuve
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatFlesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatEye;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatPupil;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatRed;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatOrange;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MatGlass;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylMesh;

	static TWeakObjectPtr<AEONBoss> Active;
};
