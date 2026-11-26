// EON — jalon 8 : les écrans qui figent le jeu (06, sections 3, 5 et 6).
// - Dialogues : boîte en bas, portrait provisoire, texte lettre par lettre (46 car/s), file d'attente (pendingDialog).
// - Terminal du C.O.M : 5 onglets (Améliorations, Logs, Quêtes, Descente, Cassettes).
// - Écran titre : Continuer / Nouvelle partie (deux appuis pour effacer).
// - Menu pause (Échap / Menu) : Redémarrer, Options, Quitter (deux appuis pour confirmer) ; Échap reprend.
// - Options : huit onglets (EONOptions) : affichage, graphismes, éclairage, détails, audio, commandes, jouabilité, accessibilité.
// L'état vit ici, l'entrée est lue par AEONPlayerController (TickUI, même en pause), le dessin par AEONHUD.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EONUI.generated.h"

class APlayerController;

/** Géométrie du panneau Options dans l'écran de référence (1080 de haut ; Cx = milieu de l'écran) : la même pour le dessin et la souris. */
namespace EONOptLayout
{
	/** Huit onglets égaux de Cx - TabSpan à Cx + TabSpan. */
	constexpr float TabY = 168.f, TabH = 46.f, TabSpan = 800.f, TabW = 200.f;
	/** Liste : de Cx + ListDx, ListW de large ; RowsVisible lignes de RowH. */
	constexpr float ListDx = -800.f, ListW = 1000.f, ListY = 236.f, RowH = 46.f;
	constexpr int32 RowsVisible = 14;
	/** Case de valeur (depuis le bord gauche de la liste) ; curseurs : barre de SlX0 à SlX1 ; touches : colonnes clavier et manette. */
	constexpr float ValX0 = 620.f, ValX1 = 980.f, ArrowW = 46.f, SlX0 = 640.f, SlX1 = 870.f;
	constexpr float KbX0 = 620.f, KbX1 = 796.f, PadX0 = 804.f, PadX1 = 980.f;
	/** Panneau d'aide à droite. */
	constexpr float HelpDx = 240.f, HelpW = 560.f;
	/** Bouton RETOUR (centré) et ligne des touches. */
	constexpr float BackY = 920.f, BackH = 50.f, BackW = 300.f, HintY = 986.f;
	/** Boutons du menu pause (centrés). */
	constexpr float PauseBtnY = 460.f, PauseBtnStep = 92.f, PauseBtnW = 480.f, PauseBtnH = 66.f;
}

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
	int32 GetOptTab() const { return OptTab; }
	/** Ligne choisie dans l'onglet (EONOptions::Rows) ; égale au nombre de lignes = bouton RETOUR. */
	int32 GetOptSel() const { return OptSel; }
	/** Première ligne affichée (onglets longs : Commandes). */
	int32 GetOptScroll() const { return OptScroll; }
	/** Lignes de touches : colonne choisie (0 clavier-souris, 1 manette). */
	int32 GetOptCol() const { return OptCol; }
	/** Réassignation en cours : le panneau attend une touche (IsWaitingPad : un bouton de manette). */
	bool IsWaitingKey() const { return bWaitKey; }
	bool IsWaitingPad() const { return bWaitPad; }
	/** Colonne effective d'une ligne de touches : celle choisie si la ligne l'a, sinon l'autre. */
	static int32 OptColFor(int32 Col, bool bHasKb, bool bHasPad) { return Col == 1 ? (bHasPad ? 1 : 0) : (bHasKb ? 0 : 1); }
	void OpenPause();
	void ClosePause();

	/** Un de ces écrans est ouvert : le jeu est figé, Jira ne reçoit plus d'entrée. */
	bool IsBlocking() const { return bTitle || bCom || bPause || IsDialogOpen(); }
	/** Fermé depuis moins de 0,25 s : la touche qui a fermé ne doit pas rouvrir autre chose. */
	bool JustClosed() const;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	/** Commande eon.ui (tests) : continuer, nouvelle, suite, passer ; pause, haut, bas, gauche, droite, valider, retour ;
	 *  options, onglet N (1 à 8), colonne 0|1, ligne N, touche <nom de touche Unreal> (pendant une réassignation). */
	void DevAction(const FString& A);
		/** Appelé par le contrôleur à chaque image, en temps réel, même en pause. */
	void TickUI(APlayerController* PC, float RealDt);

private:
	void DlgAdvance();
	void DlgClose();
	void SyncPause();
	int32 ComCount(int32 Tab) const;
	void PauseAct();
	void PauseNav(int32 Dir);
	// Options : navigation, souris, réassignation.
	void OpenOptions(bool bPad);
	void TickOptions(APlayerController* PC, float RealDt);
	void TickPauseMouse(APlayerController* PC);
	void OptSetTab(int32 Tab);
	void OptNav(int32 Dir);
	void OptSide(int32 Dir);
	void OptActivate();
	void OptBack();
	void OptKeepVisible();
	void OptMouse(APlayerController* PC);
	/** Souris dans l'écran de référence (1080 de haut) ; false si inconnue. */
	bool MouseRef(APlayerController* PC, FVector2D& Out, float& OutW) const;
	void ShowMenuCursor(APlayerController* PC, bool bMenu);

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
	int32 OptTab = 0;
	int32 OptSel = 0;
	int32 OptScroll = 0;
	int32 OptCol = 0;
	bool bWaitKey = false;
	bool bWaitPad = false;
	float WaitT = 0.f;
	/** Après une réassignation : la touche choisie ne doit rien faire d'autre dans le menu. */
	float OptIgnoreT = 0.f;
	/** Répétition des flèches tenues (curseurs, listes). */
	int32 RepDir = 0;
	float RepT = 0.f;
	bool bDragSlider = false;
	FVector2D LastMouse = FVector2D(-1.0, -1.0);
	/** Curseur du jeu (viseur) gardé pendant le menu : flèche dans le menu. */
	uint8 SavedCursor = 255;
	bool bDevWasOpen = false;

	bool bWePaused = false;
	double ClosedAt = -10.0;
};
