// EON — un ennemi du proto (jalon 3). Portage de mkEnemy, de la boucle « ennemis » de update(),
// de chaseDir, hitEnemy et des hitbox en capsule (ehh / ehr / ecy) du proto v0.60.1.
// Le gameplay tourne en 2D, en px « salle » (AEONTileRoom), exactement comme le proto ;
// l'acteur ne fait que suivre (RoomPxToWorld). C'est AEONCombatDirector qui appelle UpdateEnemy
// dans l'ordre de la liste, comme le proto : l'acteur ne se met pas à jour tout seul.
// Visuel : modèle 3D (EONEnemy.cpp, table Looks) : modèles du studio avec anims de déplacement, coup et mort (cadavre) ;
// Brainless et Méduse restent des meshes Unity (MANQUANTS.md §2).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONTypes.h"
#include "EONEnemy.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UAnimSequence;
class AEONTileRoom;
class AEONCombatDirector;

/** États du proto (e.st). */
UENUM()
enum class EEONEnemySt : uint8
{
	Move, Arm, Wind, Dash, Rec, Aim, Fire, Lock, Chg, Stun, Lay
};

UCLASS(NotBlueprintable)
class EON_API AEONEnemy : public AActor
{
	GENERATED_BODY()

public:
	AEONEnemy();

	/** mkEnemy(type, x, y) : PV × (1 + 0,07 × combatN) arrondi sup. Faux si le type manque dans DT_Ennemis. */
	bool Init(AEONCombatDirector* InDirector, AEONTileRoom* InRoom, FName InType, FVector2D InPosPx, int32 CombatN);
	/** Élite (spawns du proto) : PV × 2,2 arrondis, rayon × 1,3, masse × 2, capsule × 1,45 / 1,3, sprite × 1,45. */
	void MakeElite();

	/** hitEnemy() pour un coup venu du monde (projectile de Jira). Dir = direction du coup (au sol). */
	void ApplyHit(float Damage, FVector Dir, float KnockMul = 1.f);
	/** hitEnemy(e, dmg, dx, dy, kmul) en unités proto. */
	virtual void HitPx(float Damage, float Dx, float Dy, float KnockMul = 1.f);
	/** Le gardien (AEONBoss) : 30 dégâts par rayon de l'ultime au lieu de 14. */
	virtual bool IsBoss() const { return false; }

	/** Un pas de la boucle « ennemis » du proto. Cible = Jira (TargetPx), Jira à portée de coup si bTargetHittable. */
	virtual void UpdateEnemy(float Dt, const FVector2D& TargetPx);
	/** Position du sprite, clignotement, télégraphes. */
	virtual void UpdateVisual(float Dt, const FVector2D& TargetPx);

	/** Point visé par l'aide à la visée (hauteur du bras de Jira). */
	FVector GetAimPoint() const;
	/** Visable : ni mort, ni en chute. */
	/** Vitrine des effets (eon.vitrine) : un trou sans dégâts ni éclat blanc. */
	void DevPunch(float Dmg, float Dx, float Dy, int32 Niveau = 1) { PunchHole(Dmg, Dx, Dy, Niveau); }
	/** Vitrine : la prochaine balle qui touche un ennemi est un critique (tournages réglés à l'image près). */
	static bool bCritSuivant;
	bool IsTargetable() const { return !bDead && Falling <= 0.f; }
	bool IsDead() const { return bDead; }
	bool IsFalling() const { return Falling > 0.f; }

	/** ehh(e) / ehr(e) / ecy(e) du proto (px). */
	float Ehh() const { return HH * (bElite ? 1.45f : 1.f); }
	float Ehr() const { return HR * (bElite ? 1.3f : 1.f); }
	float Ecy() const { return Pos.Y - Ehh() * 0.5f; }

	/** Mort sans crédit (Onibi qui explose) ou par killEnemy : coupe la collision. */
	void MarkDead();
	/** Retiré de la liste du directeur : joue l'anim de mort (ou d'explosion), s'enfonce dans le sol puis se détruit.
	 *  Faux si le modèle n'a pas d'anim de mort (ou s'il est tombé dans le vide) : le directeur le détruit tout de suite. */
	bool BeginCorpse();
	virtual void Tick(float DeltaSeconds) override;

	// --- État du proto, en px salle et px/s (lu par le directeur pour les vagues, l'AOE, les vols de mates).
	FName Type;
	FString DisplayName;
	/** Irruption, « la chose » (stalk du proto) : rôde autour de Jira, s'arrête (yeux rouges, 0,65 s), puis charge à 240 px/s
	 *  pendant 0,75 s. Un contact (hors rôde) tue. StalkSt : 0 rôde, 1 arrêt, 2 charge. */
	/** Cartographe : la moitié des ennemis apparus pendant sa défense le visent, lui (kidT du proto). */
	bool bKidT = false;
	bool bStalk = false;
	uint8 StalkSt = 0;
	float StalkT = 0.f;
	float StalkA = 0.f;
	FVector2D StalkV = FVector2D::ZeroVector;
	void MakeStalker();
	void StalkUpdate(float Dt, const FVector2D& JiraPx);
	FVector2D Pos = FVector2D::ZeroVector;
	FVector2D Vel = FVector2D::ZeroVector;
	FVector2D Knock = FVector2D::ZeroVector;
	float Hp = 1.f;
	float MaxHp = 1.f;
	float R = 5.f;
	float Flash = 0.f;
	EEONEnemySt St = EEONEnemySt::Move;
	float StT = 0.f;
	float Ft = 1.f;
	float Cd = 0.6f;
	float Side = 1.f;
	float T = 0.f;
	float Falling = 0.f;
	bool bElite = false;
	float MassM = 1.f;
	bool bDead = false;
	/** Données de la ligne DT_Ennemis. */
	float Spd = 0.f, Mass = 1.f, HH = 0.f, HR = 0.f;
	bool bContact = false, bFly = false;
	int32 Adn = 0;

protected:
	FVector2D ChaseDir(const FVector2D& Tg) const;
	void ApplyCapsuleSize();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Capsule;
	/** Pivot du modèle 3D : tourne vers la marche ou vers Jira. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ModelRoot;
	UPROPERTY() TArray<TObjectPtr<UMeshComponent>> Meshes;
	/** Matériaux d'origine, slot par slot, pour revenir du flash blanc. */
	UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> OrigMats;
	/** COM-V9 : trous dans le corps (punchHole du proto). Instances dynamiques de M_EON_ToonEnnemi, paramètres Trou0..7
	 *  (position avant skinning en cm du maillage, rayon en w). */
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> HoleMids;
	/** Trous posés (xyz position avant skinning en cm du maillage, w rayon), 16 au plus (Vincent, 25/11 : fusion et
	 *  16 emplacements). Une balle qui tombe sur un trou l'agrandit ; une fois les 16 pris, elle agrandit le plus proche :
	 *  aucun trou ne se referme. */
	TArray<FVector4f> Holes;
	/** Niveau du coup = couleur de son chiffre (Vincent, 25/11) : 0 blanc, giclée seule ; 1 orange, giclée et trou ;
	 *  2 critique rouge, giclée, trou et un membre qui part. */
	void PunchHole(float Dmg, float Dx, float Dy, int32 Niveau = 1);
	/** Membres coupés (bit k = membre k de la table du maillage). Vincent, 25/11 : seul un coup critique démembre, dans
	 *  l'ordre tête (elle explose), bras, puis jambes ; une jambe coupée, il tombe et meurt (un tronc ne vit pas). Le corps
	 *  cache le membre (Coupe{k}A/B/C), une copie figée tombe en rigid body (AEONMembre). */
	uint8 Coupes = 0;
	void CutLimb(int32 K, const FVector& Dw);
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlashMat;
	bool bFlashOn = false;
	FLinearColor FlashCol = FLinearColor::White;
	/** Sursaut à l'impact (Vincent, 25/11) : le modèle recule dans le sens du tir, 10 cm au plus en 50 ms, puis revient. */
	float RecoilT = -1.f;
	FVector RecoilDir = FVector::ZeroVector;
	float ModelScale = 1.f;
	float FaceYaw = 0.f;
	float YawOff = 0.f;
	void BuildModel();
	/** Flash de coup : teinte Col sur le matériau à trous (FlashCouleur), sans changer de matériau (les trous restent). */
	void SetFlash(bool bOn, const FLinearColor& Col = FLinearColor::White);

	// Anims du modèle (table Looks) : boucle de déplacement, attente, coup, explosion (Onibi), morts (une au hasard).
	UPROPERTY() TObjectPtr<UAnimSequence> AnimMove;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimIdle;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimAttack;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimExplode;
	/** Kama : provocation après un dash qui touche Jira ; Onibi : départ de course (se ramasse puis s'élance). */
	UPROPERTY() TObjectPtr<UAnimSequence> AnimTaunt;
	UPROPERTY() TObjectPtr<UAnimSequence> AnimStart;
	/** Onibi : temps restant du départ de course (immobile pendant ce temps). */
	float StartT = 0.f;
	/** Kama : son dash a touché Jira (la récupération devient une provocation). */
	bool bDashHit = false;
	UPROPERTY() TArray<TObjectPtr<UAnimSequence>> AnimDeaths;
	UPROPERTY() TObjectPtr<UAnimSequence> CurAnim;
	float AttackRate = 1.f;
	/** Temps restant de l'anim jouée une fois (coup, explosion) : la boucle reprend après. */
	float ActionT = 0.f;
	EEONEnemySt PrevSt = EEONEnemySt::Move;
	void PlayAnim(UAnimSequence* A, bool bLoop, float Rate);
	void UpdateAnim(float Dt, float DistToTarget);
	/** Tombé dans le vide (pas de cadavre), explosé (anim d'explosion au lieu de la mort). */
	bool bFell = false;
	bool bExploded = false;
	float CorpseT = 0.f;
	float SinkT = 0.f;

	TWeakObjectPtr<AEONCombatDirector> Director;
	TWeakObjectPtr<AEONTileRoom> Room;

	// Nuée (goule) : sd, al, rg du proto.
	float Sd = 1.f, Al = 0.f, Rg = 0.f;
	// Direction verrouillée (lx, ly) : charge, dash, visée.
	FVector2D L = FVector2D(1.f, 0.f);
};
