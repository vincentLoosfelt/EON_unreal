// EON — jalon 8 : les écrans qui figent le jeu (06, sections 3, 5 et 6).
// - Dialogues : boîte en bas, portrait provisoire, texte lettre par lettre (46 car/s), file d'attente (pendingDialog).
// - Terminal du C.O.M : 5 onglets (Améliorations, Logs, Quêtes, Descente, Cassettes).
// - Écran titre : Continuer / Nouvelle partie (deux appuis pour effacer).
// - Menu pause (Échap / Menu) : Redémarrer, Options, Quitter (deux appuis pour confirmer) ; Échap reprend.
// - Options : volume, affichage, résolution, V-sync, limite d'images, qualité (GameUserSettings.ini).
// L'état vit ici, l'entrée est lue par AEONPlayerController (TickUI, même en pause), le dessin par AEONHUD.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EONUI.generated.h"

class APlayerController;

/** Une réplique (dialogues.json : who, ex, t). */
struct EON_API FEONDlgLine
{
	FName Who;
	FName Ex;
	FString T;
};

UCLASS()
class EON_API UEONUISubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	static UEONUISubsystem* Get(const UObject* WorldContext);

	// --- Données (DONNEES/dialogues.json, générées dans EONDialogData.inl)
	static TArray<FEONDlgLine> Lines(FName Key);
	static bool CharInfo(FName Who, FString& OutName, FColor& OutCol);
	static int32 NumLogs();
	static FString Log(int32 I);

	// --- Dialogues
	/** pendingDialog = DLG[Key] : s'ouvre dès que le jeu est libre (ni terminal, ni choix de niveau, ni mort). */
	void Say(FName Key, TFunction<void()> OnEnd = nullptr);
	void SayLines(const TArray<FEONDlgLine>& L, TFunction<void()> OnEnd = nullptr);
	bool IsDialogOpen() const { return Dlg.Num() > 0; }
	bool HasPendingDialog() const { return Pending.Num() > 0; }
	const FEONDlgLine* DlgLine() const { return Dlg.IsValidIndex(DlgI) ? &Dlg[DlgI] : nullptr; }
	int32 DlgShown() const { return FMath::FloorToInt(DlgShownF); }

	// --- Terminal du C.O.M
	bool IsComOpen() const { return bCom; }
	int32 GetComTab() const { return ComTab; }
	int32 GetComSel(int32 Tab) const { return Tab >= 0 && Tab < 5 ? ComSel[Tab] : 0; }
	/** openCOM : sur Descente si le levier est prêt, sinon Améliorations. */
	void OpenCom();
	void CloseCom();
	void SetComTab(int32 T) { ComTab = FMath::Clamp(T, 0, 4); }
	void SetComSel(int32 S) { ComSel[ComTab] = FMath::Max(0, S); }
	/** Entrée / A dans l'onglet courant : levier (Descente) ou Installer (Cassettes). */
	void ComAct();
	struct FQuest { FString T; FColor C; bool bOk = false; TArray<FString> L; };
	TArray<FQuest> Quests() const;
	/** comItems : objets de la run regroupés (×n). */
	TArray<TPair<FName, int32>> ComItems() const;
	struct FCassRow { FName K; int32 Lvl = 0; FString Br; FString Lock; };
	static const TArray<FCassRow>& CassRows();
	/** cassState(r) : 0 installé, 1 achetable, 2 pas assez de Données, 3 verrouillé. Texte et coût en sortie. */
	int32 CassState(const FCassRow& R, FString& OutT, int32& OutCost) const;
	/** Le C.O.M est à quai : seul endroit où l'on installe une cassette. */
	bool IsDocked() const;

	// --- Écran titre
	bool IsTitleOpen() const { return bTitle; }
	int32 GetTitleSel() const { return TitleSel; }
	bool IsTitleConfirm() const { return bTitleConfirm; }
	void OpenTitle(TFunction<void()> OnNew);

	// --- Menu pause et Options
	bool IsPauseOpen() const { return bPause; }
	int32 GetPauseSel() const { return PauseSel; }
	/** Bouton qui attend son second appui (0 Redémarrer, 2 Quitter), sinon -1. */
	int32 GetPauseConfirm() const { return PauseConfirm; }
	bool IsOptionsOpen() const { return bOptions; }
	int32 GetOptSel() const { return OptSel; }
	struct FOptRow { FString L; FString V; };
	/** Lignes des Options (la dernière : RETOUR, sans valeur). */
	TArray<FOptRow> OptionRows() const;
	void OpenPause();
	void ClosePause();
	/** Volume général enregistré (0 à 1), appliqué au périphérique audio. */
	static float GetSavedVolume();
	static void ApplyVolume(UWorld* W);

	/** Un de ces écrans est ouvert : le jeu est figé, Jira ne reçoit plus d'entrée. */
	bool IsBlocking() const { return bTitle || bCom || bPause || IsDialogOpen(); }
	/** Fermé depuis moins de 0,25 s : la touche qui a fermé ne doit pas rouvrir autre chose. */
	bool JustClosed() const;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	/** Commande eon.ui (tests) : continuer, nouvelle, suite, passer ; pause, haut, bas, gauche, droite, valider, retour. */
	void DevAction(const FString& A);
		/** Appelé par le contrôleur à chaque image, en temps réel, même en pause. */
	void TickUI(APlayerController* PC, float RealDt);

private:
	void DlgAdvance();
	void DlgClose();
	void SyncPause();
	int32 ComCount(int32 Tab) const;
	void PauseAct();
	void OptChange(int32 Dir);
	void PauseNav(int32 Dir);

	TArray<FEONDlgLine> Dlg;
	int32 DlgI = 0;
	float DlgShownF = 0.f;
	float DlgAutoT = 0.f;
	TFunction<void()> DlgEnd;
	struct FPending { TArray<FEONDlgLine> L; TFunction<void()> OnEnd; };
	TArray<FPending> Pending;

	bool bCom = false;
	int32 ComTab = 0;
	int32 ComSel[5] = { 0, 0, 0, 0, 0 };
	float ComAutoT = 0.f;

	bool bTitle = false;
	int32 TitleSel = 0;
	bool bTitleConfirm = false;
	float TitleAutoT = 0.f;
	TFunction<void()> TitleNew;

	bool bPause = false;
	int32 PauseSel = 0;
	int32 PauseConfirm = -1;
	bool bOptions = false;
	int32 OptSel = 0;
	bool bDevWasOpen = false;

	bool bWePaused = false;
	double ClosedAt = -10.0;
};
