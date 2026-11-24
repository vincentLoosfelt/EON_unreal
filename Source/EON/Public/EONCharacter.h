// EON — Jira. Jalon 1 : déplacement lissé, dash (2 charges), course, énergie,
// greffe Standard (projectiles, cadence, dispersion), aide à la visée, game feel du tir.
// Les règles viennent de 02_JOUEUR_ET_COMBAT.md et de update() du proto (v0.60.1).
// Jalon 2 : PV, coups reçus (hurtPlayer), chute dans le vide (checkPlayerFall : 3 points sur 4, retour au dernier point sûr).
// Pas encore là : mutation et armure (J4), autres greffes (J4).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EONTypes.h"
#include "EONCharacter.generated.h"

class UPointLightComponent;
class UAnimInstance;
class AEONPlayerCameraManager;

UCLASS(Blueprintable)
class EON_API AEONCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEONCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	/** Direction voulue, repère écran : X = droite, Y = haut. Longueur 0..1. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetMoveInput(FVector2D ScreenInput) { RawMove = ScreenInput.GetClampedToMaxSize(1.f); }

	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	FVector2D GetMoveInput() const { return RawMove; }

	/** Angle de visée brut (radians, 0 = +X, + = vers +Y). L'aide à la visée s'applique ensuite. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetAimAngle(float Radians) { RawAimAngle = Radians; }

	/** Angle de visée final (après aide à la visée, ou direction de course). */
	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	float GetAimAngle() const { return AimAngle; }

	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetFireHeld(bool bHeld) { bFireHeld = bHeld; }

	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	bool IsFireHeld() const { return bFireHeld; }

	/** Bouton de dash maintenu : au-delà de 0,2 s en mouvement, Jira court. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetDashHeld(bool bHeld) { bDashHeld = bHeld; }
	/** Maj (accélération) : course tout de suite, sans dash (Vincent, 16/11). */
	void SetSprintHeld(bool bHeld) { bSprintHeld = bHeld; }
	/** Marche scriptée (sortie de cuve) : direction écran, durée en s. */
	void WalkOut(FVector2D ScreenDir, float Duration) { WalkOutDir = ScreenDir; WalkOutT = Duration; }

	/** Appui sur le bouton de dash. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void PressDash();

	/** Hauteur du bras (départ des tirs) au-dessus du sol, en world. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	FVector GetArmLocation() const;

	/** Bout du bras pour un angle donné (tip(a) du proto : 3 px de côté, 12 px devant). */
	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	FVector GetMuzzleLocation(float Angle) const;

	UFUNCTION(BlueprintPure, Category = "EON|Jira") bool IsFallingIntoVoid() const { return bFallingIntoVoid; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") bool IsDashing() const { return DashRemaining > 0.f; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") bool IsRunning() const { return bRunning; }
	/** Invulnérable (dash, après un coup). Utilisé par les ennemis au jalon 3. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") bool IsInvulnerable() const { return InvTimer > 0.f; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetEnergy() const { return Energy; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetEnergyMax() const { return EnergyMax; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetDashCharges() const { return DashCharges; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetDashMax() const { return DashMax; }

	// --- PV (02, section 4) et chute (section 5)
	/** hurtPlayer(amt, sx, sy) du proto : rien pendant l'invulnérabilité, le dash ou la chute. Source = position du coup. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void HurtPlayer(int32 Amount, FVector SourceWorld);
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetHP() const { return HP; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetMaxHP() const { return MaxHP; }

	// --- Jalon 6 : progression de la run (ADN, niveaux, scrap, objets)
	/** gainADN(v) : ×adnM ; chaque palier (40 + 14 × niveau) donne un niveau à choisir. */
	void GainADN(float V);
	void AddScrap(int32 N) { Scrap = FMath::Max(0, Scrap + N); }
	/** Ramassable énergie : +V, rien en mutation. */
	void PickupEnergy(float V);
	void Heal(int32 N) { HP = FMath::Min(MaxHP, HP + N); }
	/** Insignes rendus à Ulysse : +N PV max (et PV), pour la run en cours. */
	void AddMaxHP(int32 N) { MaxHP += N; HP += N; }
	/** takeItem(it) : applique l'effet de l'objet (DONNEES/items.json). */
	void TakeItem(FName Id);
	bool HasItem(FName Id) const { return Items.Contains(Id); }
	const TArray<FName>& GetItems() const { return Items; }
	float GetADN() const { return Adn; }
	float GetADNNeed() const { return 40.f + 14.f * Lvl; }
	int32 GetLevel() const { return Lvl; }
	int32 GetScrap() const { return Scrap; }
	float GetMagnetPx() const { return MagR; }
	float GetScrapM() const { return ScrapM; }
	int32 GetShield() const { return Shield; }
	int32 GetShieldMax() const { return ShieldMax; }
	void RefillShield() { Shield = ShieldMax; }
	/** Cassettes gardées entre les runs (META.cass) : Shield NV1 et plus = un bouclier dès le départ. */
	void ApplyCassettes();
	float GetDonM() const { return DonM; }
	int32 GetPierce() const { return Pierce; }
	int32 GetBounce() const { return Bounce; }
	/** Écran de niveau (1 parmi 3) : ouvert seulement hors combat, salle nettoyée et ADN ramassé (+0,6 s). Le jeu est en pause. */
	class AEONPedestal* AutoFindPedestal(class AEONTileRoom* Rm) const;
	bool IsLevelUpOpen() const { return LvOpts.Num() > 0; }
	const TArray<FName>& GetLevelOpts() const { return LvOpts; }
	int32 GetLevelSel() const { return LvSel; }
	float GetLevelOpenT() const { return LvOpenT; }
	int32 GetLevelPending() const { return LvPend; }
	void LevelNav(int32 Step) { if (LvOpts.Num()) { LvSel = (LvSel + Step + LvOpts.Num()) % LvOpts.Num(); } }
	void PickLevel(int32 I);
	/** Appelé par le contrôleur à chaque image, même en pause (temps réel). */
	void TickLevelUp(float RealDt);
	UFUNCTION(BlueprintPure, Category = "EON|Jira") bool IsDead() const { return HP <= 0; }
	/** Nombre de chutes de la run (P.falls). */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetFallCount() const { return FallCount; }
	/** Flash rouge de coup reçu (flashHurt), 0..0,25 s. Lu par le HUD. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetHurtFlash() const { return HurtFlash; }
	/** Remet Jira en état (nouvelle run / changement de salle). */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void ResetForRoom(FVector Location);
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void ResetRun();

	/** Change de greffe (ligne de DT_Greffes). Seule « standard » tire au jalon 1. */
	UFUNCTION(BlueprintCallable, Category = "EON|Jira")
	void SetGreffe(FName NewGreffeId);

	UFUNCTION(BlueprintPure, Category = "EON|Jira")
	FName GetGreffeId() const { return GreffeId; }

	// --- Jalon 4 : greffes (02, section 5), mutation et armure (section 7), sorts et ultime (section 8)
	/** Greffes de Jira dans l'ordre 1 à 5 (JiraOrder). Les verrous de méta (LOCK_G) arrivent au jalon 8 : toutes sont en main. */
	TArray<FName> GetGreffeSet() const;
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void SelectGreffeSlot(int32 Slot);
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void CycleGreffe(int32 Step);
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void SetSpellHeld(bool bHeld) { bSpellHeld = bHeld; }
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void PressSpell() { bSpellPress = true; }
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void PressHolo() { bHoloPress = true; }
	UFUNCTION(BlueprintCallable, Category = "EON|Jira") void PressUlt() { bUltPress = true; }
	/** Stade de mutation (0 = sain, 1 à 3). */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetMut() const { return Mut; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetArmorPerm() const { return ArmorPerm; }
	/** armorLvl() : le plus haut entre l'armure permanente et le stade en cours. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") int32 GetArmorLevel() const { return FMath::Max(ArmorPerm, Mut); }
	/** enCap() : énergie max − 12 % par niveau d'armure permanente. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetEnergyCap() const;
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetUlt() const { return Ult; }
	/** Charge de Rafale / Laser, 0..1. */
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetCharge01() const;
	UFUNCTION(BlueprintPure, Category = "EON|Jira") bool AreSpellsBlocked() const { return Mut >= 2; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetSpellCooldown() const { return SpellCd; }
	UFUNCTION(BlueprintPure, Category = "EON|Jira") float GetHoloCooldown() const { return HoloCd; }
	/** P.ult += dégâts × 0,8 (hitEnemy). */
	void AddUlt(float Damage) { Ult = FMath::Min(100.f, Ult + Damage * 0.8f); }
	/** dmgMult() : Catalyseur Berserk (+35 % par stade) au jalon 6. */
	float DmgMult() const;
	// Hologramme (decoy)
	bool HasDecoy() const { return DecoyT > 0.f; }
	FVector GetDecoyLocation() const { return DecoyLoc; }
	float GetDecoyHP01() const { return FMath::Clamp(DecoyHp / 8.f, 0.f, 1.f); }
	void DamageDecoy(float Amount) { DecoyHp -= Amount; }
	// QTE et cut-in de l'ultime (lus par le HUD)
	bool IsInQTE() const { return bQTE; }
	const TArray<uint8>& GetQTESeq() const { return QTESeq; }
	int32 GetQTEIndex() const { return QTEIdx; }
	float GetQTETime01() const { return QTET / 3.2f; }
	float GetCutin() const { return CutinT; }
	/** Écart du réticule (crossT du proto) : +0,35 par tir, 1 au coup de lame, retombe de 4 par seconde (JIR-U2). */
	float GetCrossT() const { return CrossT; }
	/** Vitrine des effets (eon.vitrine) : force un stade de mutation, lance l'ultime sans QTE. */
	void DevSetMut(int32 Stage) { Mut = FMath::Clamp(Stage, 0, 3); MutT = 0.f; LastSpendTime = WorldTime; }
	void DevUltFire() { UltFire(); }
	/** Vitrine : une frappe de l'ultime (JIR-V10.4) sur E, tout de suite. */
	void DevUltStrike(class AEONEnemy* E);
	/** Vitrine : un coup de la greffe Id (laser, lame, griffe) dans l'axe de visée, sans changer la greffe portée. */
	void DevWeapon(FName Id, float Ratio = 1.f);
	/** Vitrine : l'hologramme JIR-V9.2 est-il affiché à cet instant (clignotement) ? */
	bool DevHoloVisible() const;
	/** Vitrine aux commandes : le contrôleur ne touche plus au tir, au déplacement ni à la visée. */
	bool bDevDrive = false;

	/** Lueur du flash de bouche. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|Jira")
	TObjectPtr<UPointLightComponent> MuzzleLight;

	// --- Valeurs de départ (02, section 2). Unités proto : px, px/s, s.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira") float WalkSpeedPx = 82.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira") float CollisionRadiusPx = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira") float HeightPx = 29.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira") float EnergyMax = 100.f;
	/** PV de départ (basePlayer : hp 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira") int32 StartHP = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Jira") FName GreffeId = TEXT("standard");

	// --- Dash et course (02, section 3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Dash") int32 DashMax = 2;
	/** px/s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Dash") float DashSpeedPx = 345.f;
	/** s */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Dash") float DashDuration = 0.15f;
	/** Invulnérabilité au dash, s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Dash") float DashInvulnerability = 0.2f;
	/** Une charge toutes les DashCooldown secondes, une par une. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Dash") float DashCooldown = 1.1f;
	/** Durée d'appui avant que la course prenne le relais, s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Dash") float RunHoldTime = 0.2f;

	// --- Énergie (02, section 6)
	/** Énergie par seconde, après EnergyRegenDelay sans dépense. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Énergie") float EnergyRegen = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Énergie") float EnergyRegenDelay = 0.7f;

protected:
	virtual void BeginPlay() override;

private:
	float ApplyAimAssist(float Raw, float Strength) const;
	const FEONGreffeRow* FindGreffe() const;
	void FireAuto(const FEONGreffeRow& G, const FEONGameFeelRow& Feel);
	void SpawnBullet(const FEONGreffeRow& G, const FEONGameFeelRow& Feel, float SpeedOverridePx);
	void OnShot(const FEONGreffeRow& G, const FEONGameFeelRow& Feel, float KickOverride = -1.f, bool bShell = true);
	void Spend(float Cost);
	void CheckPlayerFall();
	void CheckDeath();
	AEONPlayerCameraManager* GetEONCamera() const;

	FVector2D RawMove = FVector2D::ZeroVector;
	FVector2D SmoothMove = FVector2D::ZeroVector;
	float RawAimAngle = 0.f;
	float AimAngle = 0.f;
	bool bFallingIntoVoid = false;

	bool bFireHeld = false;
	/** Poids de la couche « tir » du haut du corps (variable EONTirAlpha de l'ABP d'Epic), lissé. */
	float TirAlpha = 0.f;
	/** ABP de locomotion du Game Animation Sample (SandboxCharacter_CMC_ABP). */
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> LocoAnimClass;
	bool bDashHeld = false;
	bool bSprintHeld = false;
	FVector2D WalkOutDir = FVector2D::ZeroVector;
	float WalkOutT = 0.f;
	/** Jira toujours au sol : jamais de chute, de vol ni de nage hors du vide (Vincent 16/11). */
	void KeepOnGround();
	/** Kill zone franchie : retour sur le sol le plus proche, −FallDmg PV, 1,2 s d'invulnérabilité (conséquence de la chute du proto). */
	void VoidRespawn();
	bool bRunning = false;
	float HoldTimer = 0.f;

	int32 DashCharges = 2;
	float DashRemaining = 0.f;
	float DashRecharge = 0.f;
	FVector DashDir = FVector::ForwardVector;

	float InvTimer = 0.f;
	int32 HP = 6;
	int32 MaxHP = 6;
	int32 FallCount = 0;
	/** Kill zone : 10 m sous le dernier sol où Jira a marché (Vincent 16/11) ; dernier sol et dernier point sûr. */
	static constexpr float KillZoneDepth = 1000.f;
	float LastGroundZ = 0.f;
	FVector LastGroundPos = FVector::ZeroVector;
	bool bHasGround = false;
	FVector SafeLocation = FVector::ZeroVector;
	bool bHasSafeLocation = false;
	float HurtFlash = 0.f;
	FVector MeshBaseLocation = FVector::ZeroVector;
	FVector MeshBaseScale = FVector::OneVector;
	float Energy = 100.f;
	float LastSpendTime = -9.f;
	float WorldTime = 0.f;
	/** Démo de locomotion (console eon.demo) : temps écoulé, < 0 = inactive. */
	float DemoTime = -1.f;
	/** Pilote automatique de test (eon.auto 1) : vise et tire sur l'ennemi le plus proche, tourne autour, puis passe une porte. */
	void AutoPilot(float Dt);
	float AutoSwap = 0.f;
	float AutoIdle = 0.f;
	float AutoSide = 1.f;
	float AutoGreffeT = 0.f;
	bool bAutoCool = false;
	TMap<TWeakObjectPtr<AActor>, FVector> AutoLastPos;
	bool AutoQTEFlip = false;
	float CooldownTimer = 0.f;
	float MuzzleTimer = 0.f;
	/** Recul du tir, cm/s, amorti comme la vitesse (k = min(1, dt×16)). */
	FVector RecoilVel = FVector::ZeroVector;
	bool bWarnedGreffe = false;

	// --- Jalon 4
	void UpdateMutation(float Dt, const FEONGameFeelRow& Feel);
	void UpdateWeapon(float Dt, const FEONGreffeRow& G, const FEONGameFeelRow& Feel);
	void UpdateSpells(float Dt, const FEONGameFeelRow& Feel);
	void UpdateQTE(float RealDt);
	void UltFire();
	void DoBeam(float A, float Ratio, const FEONGreffeRow& G, const FEONGameFeelRow& Feel);
	void DoSlash(float A, const FEONGreffeRow& G, const FEONGameFeelRow& Feel);
	void DoLash(float A, const FEONGreffeRow& G, const FEONGameFeelRow& Feel);
	void Shockwave(float RPx, float Dmg, float Kn, bool bBig);
	void CastHolo();
	/** Position de Jira et du bout du bras en px de salle (proto). Faux hors d'une salle en tuiles. */
	class AEONTileRoom* GetRoomPx(FVector2D& OutJira, FVector2D& OutTip, float A) const;
	void Banner(const FString& Text, const FColor& Col) const;
	void StartQTE();

	int32 Mut = 0;
	float MutT = 0.f;
	float RecT = 0.f;
	int32 PeakMut = 0;
	int32 ArmorPerm = 0;
	float Ult = 0.f;
	float DmgM = 1.f;
	bool bBerserk = false, bStab = false, bReg = false;
	// Jalon 6 : ressources et multiplicateurs des objets (champs de P dans le proto)
	float Adn = 0.f;
	int32 Lvl = 1;
	int32 LvPend = 0;
	float LvT = 0.f;
	TArray<FName> LvOpts;
	int32 LvSel = 0;
	float LvOpenT = 0.f;
	int32 Scrap = 0;
	TArray<FName> Items;
	float MagR = 40.f, AdnM = 1.f, ScrapM = 1.f, DonM = 1.f;
	float RateM = 1.f, RangeM = 1.f, ChargeM = 1.f, DashDistM = 1.f, CdM = 1.f, SpdItemM = 1.f;
	int32 Size = 0, Revive = 0, Shield = 0, ShieldMax = 0, Pierce = 0, Bounce = 0, Multi = 0;
	/** shieldT : temps sans dégâts (Shield NV2 : recharge à 10 s). */
	float ShieldT = 0.f;
	float BaseEnergyMax = 100.f;
	int32 BaseDashMax = 2;
	float Charge = 0.f;
	bool bCharged = false;
	int32 BurstQ = 0;
	float BurstT = 0.f;
	float SpellCd = 0.f;
	float HoloCd = 0.f;
	bool bSpellHeld = false, bSpellPress = false, bHoloPress = false, bUltPress = false;
	bool bSpHold = false, bSpMaint = false;
	float SpT = 0.f, SpPulse = 0.f;
	float DecoyT = 0.f, DecoyHp = 0.f;
	float RunFxT = 0.f;
	float CrossT = 0.f;
	/** Fantômes du dash et de la course (JIR-V6) : cadence du proto (une chance par image à 60 i/s). */
	float GhostT = 0.f;
	/** Veines de mutation (JIR-V24) : instances dynamiques des matériaux de Jira, paramètre Veines. */
	UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> VeinMids;
	float VeinK = 0.f;
	FVector DecoyLoc = FVector::ZeroVector;
	/** JIR-V9.2 : copie lumineuse de Jira posée au leurre (EONFx::Fantome sans fondu). */
	TWeakObjectPtr<class AEONFxPiece> Holo;
	bool bQTE = false;
	TArray<uint8> QTESeq;
	int32 QTEIdx = 0;
	float QTET = 0.f;
	uint8 QTEPrevDir = 0;
	float CutinT = 0.f;
	struct FUltStrike { TWeakObjectPtr<class AEONEnemy> E; float T = 0.f; };
	TArray<FUltStrike> UltStrikes;
	int32 ShownArmor = 0;
	TArray<int32> ArmorMsgShown;
};
