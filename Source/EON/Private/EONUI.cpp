// EON — jalon 8 : dialogues, terminal du C.O.M et écran titre (openDialog, dialogAdvance, openCOM, comQuests, cassState,
// buyCass, titleChoose du proto v0.60.1).
#include "EONUI.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONSfx.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "EONCombatDirector.h"
#include "EONOptions.h"
#include "EONPlayerController.h"

namespace
{
	struct FEONChar { const TCHAR* Key; const TCHAR* Name; FColor Col; };
	struct FEONDlgSrc { const TCHAR* Key; const TCHAR* Who; const TCHAR* Ex; const TCHAR* T; };
#include "EONDialogData.inl"

	int32 AutoMode()
	{
		static IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("eon.auto"));
		return V ? V->GetInt() : 0;
	}

	bool Pressed(APlayerController* PC, std::initializer_list<FKey> Keys)
	{
		for (const FKey& K : Keys) { if (PC->WasInputKeyJustPressed(K)) { return true; } }
		return false;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdTerm(TEXT("eon.terminal"), TEXT("EON : eon.terminal ONGLET [LIGNE] [1 = agir] ; -1 ferme."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* W)
		{
			UEONUISubsystem* S = UEONUISubsystem::Get(W);
			if (!S) { return; }
			const int32 T = A.Num() ? FCString::Atoi(*A[0]) : 0;
			if (T < 0) { S->CloseCom(); return; }
			if (!S->IsComOpen()) { S->OpenCom(); }
			S->SetComTab(T);
			if (A.Num() > 1) { S->SetComSel(FCString::Atoi(*A[1])); }
			if (A.Num() > 2 && FCString::Atoi(*A[2]) > 0) { S->ComAct(); }
			UE_LOG(LogEON, Display, TEXT("EON UI : eon.terminal %d"), T);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdUi(TEXT("eon.ui"), TEXT("EON : eon.ui continuer | nouvelle | suite | passer (ecran titre et dialogues, tests sans clavier)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* W)
		{
			if (UEONUISubsystem* S = UEONUISubsystem::Get(W); S && A.Num()) { S->DevAction(FString::Join(A, TEXT(" "))); }
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDlg(TEXT("eon.dialogue"), TEXT("EON : joue le dialogue CLE (dialogues.json)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& A, UWorld* W)
		{
			if (UEONUISubsystem* S = UEONUISubsystem::Get(W); S && A.Num()) { S->Say(FName(*A[0])); }
		}));
}

UEONUISubsystem* UEONUISubsystem::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	return W ? W->GetSubsystem<UEONUISubsystem>() : nullptr;
}

TArray<FEONDlgLine> UEONUISubsystem::Lines(FName Key)
{
	TArray<FEONDlgLine> Out;
	for (const FEONDlgSrc& S : GDlgs) { if (Key == S.Key) { Out.Add({ FName(S.Who), FName(S.Ex), FString(S.T) }); } }
	return Out;
}

bool UEONUISubsystem::CharInfo(FName Who, FString& OutName, FColor& OutCol)
{
	for (const FEONChar& C : GChars) { if (Who == C.Key) { OutName = C.Name; OutCol = C.Col; return true; } }
	OutName = Who.ToString();
	OutCol = FColor::White;
	return false;
}

int32 UEONUISubsystem::NumLogs() { return UE_ARRAY_COUNT(GLogs); }
FString UEONUISubsystem::Log(int32 I) { return I >= 0 && I < NumLogs() ? FString(GLogs[I]) : FString(); }

// ---------------------------------------------------------------- dialogues

void UEONUISubsystem::Say(FName Key, TFunction<void()> OnEnd)
{
	const TArray<FEONDlgLine> L = Lines(Key);
	if (!L.Num()) { UE_LOG(LogEON, Warning, TEXT("EON UI : dialogue inconnu %s"), *Key.ToString()); if (OnEnd) { OnEnd(); } return; }
	UE_LOG(LogEON, Display, TEXT("EON UI dialogue en attente : %s"), *Key.ToString());
	SayLines(L, MoveTemp(OnEnd));
}

void UEONUISubsystem::SayLines(const TArray<FEONDlgLine>& L, TFunction<void()> OnEnd)
{
	if (!L.Num()) { if (OnEnd) { OnEnd(); } return; }
	// Le proto ne garde qu'un pendingDialog ; une file évite d'en perdre un quand deux tombent à la même image.
	Pending.Add({ L, MoveTemp(OnEnd) });
}

void UEONUISubsystem::DlgAdvance()
{
	const FEONDlgLine* L = DlgLine();
	if (!L) { return; }
	if (DlgShownF < L->T.Len()) { DlgShownF = L->T.Len(); return; }
	++DlgI;
	DlgShownF = 0.f;
	if (DlgI >= Dlg.Num()) { DlgClose(); }
}

void UEONUISubsystem::DlgClose()
{
	TFunction<void()> Fn = MoveTemp(DlgEnd);
	DlgEnd = nullptr;
	Dlg.Reset();
	DlgI = 0;
	DlgShownF = 0.f;
	ClosedAt = FPlatformTime::Seconds();
	SyncPause();
	if (Fn) { Fn(); }
}

// ---------------------------------------------------------------- terminal

bool UEONUISubsystem::IsDocked() const
{
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	return F && F->IsDocked();
}

void UEONUISubsystem::OpenCom()
{
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	bCom = true;
	ComTab = F && F->IsFloorDone() ? 3 : 0;
	for (int32& S : ComSel) { S = 0; }
	ComAutoT = 0.f;
	SyncPause();
	EONSfx::Event(this, TEXT("menu_open"));
	UE_LOG(LogEON, Display, TEXT("EON UI terminal ouvert, onglet %d"), ComTab);
}

void UEONUISubsystem::CloseCom()
{
	if (!bCom) { return; }
	bCom = false;
	ClosedAt = FPlatformTime::Seconds();
	SyncPause();
	UE_LOG(LogEON, Display, TEXT("EON UI terminal ferme"));
}

TArray<TPair<FName, int32>> UEONUISubsystem::ComItems() const
{
	TArray<TPair<FName, int32>> Out;
	const AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!J) { return Out; }
	for (const FName& Id : J->GetItems())
	{
		TPair<FName, int32>* E = Out.FindByPredicate([&Id](const TPair<FName, int32>& P) { return P.Key == Id; });
		if (E) { ++E->Value; } else { Out.Add({ Id, 1 }); }
	}
	return Out;
}

TArray<UEONUISubsystem::FQuest> UEONUISubsystem::Quests() const
{
	TArray<FQuest> Q;
	const AEONFloorManager* F = AEONFloorManager::Get(this);
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	const UEONSaveGame* M = MS ? MS->M() : nullptr;
	const FColor Cy(62, 242, 224), Am(255, 194, 61);
	if (!F || F->GetFloorN() <= 0)
	{
		Q.Add({ TEXT("Le C.O.M, à quai"), Cy, false, { TEXT("Tire le levier (onglet Descente) pour descendre à l'étage 1.") } });
	}
	else if (F->GetFloorN() == 4)
	{
		Q.Add({ TEXT("Étage 4 - Les serres"), FColor(154, 200, 96), F->IsFloorDone(), { TEXT("Pas de carte rouge ici."),
			F->IsFloorDone() ? TEXT("[X] Parler à la vieille dame, sous le saule") : TEXT("[ ] Traverser les serres jusqu'au grand saule") } });
	}
	else
	{
		const bool bDone = F->IsFloorDone(), bCard = F->HasRedCard();
		Q.Add({ FString::Printf(TEXT("Étage %d"), F->GetFloorN()), Cy, bDone, {
			TEXT("Gardien : ") + F->GuardDir().ToUpper() + TEXT(" du C.O.M"),
			bDone ? TEXT("[X] Gardien neutralisé : tire le levier (onglet Descente)") : TEXT("[ ] Neutraliser le gardien"),
			bCard ? TEXT("[X] Carte rouge en main") : TEXT("[ ] Trouver la carte rouge, sur le chemin de l'autre entrée du gardien"),
			bCard ? TEXT("[ ] Rejoindre l'ascenseur et vaincre son gardien") : TEXT("[ ] L'ascenseur reste verrouillé sans elle") } });
	}
	if (M && M->bUlysse)
	{
		// Équipage d'Ulysse : les insignes arrivent avec les salles Événement (jalon 9).
		FQuest U{ TEXT("Ulysse et son équipage"), Am, false, { TEXT("[X] Ulysse sauvé, à l'abri au Kibo.") } };
		int32 Rendus = 0;
		for (int32 I = 0; I < (int32)UE_ARRAY_COUNT(GCrew); ++I)
		{
			const FString* St = M->Crew.Find(GCrew[I]);
			if (St && *St == TEXT("rendu")) { ++Rendus; U.L.Add(FString(TEXT("[X] ")) + GCrew[I] + TEXT(" : insigne rendu")); }
			else if (St && *St == TEXT("poche")) { U.L.Add(FString(TEXT("[>] ")) + GCrew[I] + TEXT(" : insigne en poche, à rendre au Kibo")); }
			else { U.L.Add(TEXT("[ ] ??? : quelque part dans les étages")); }
		}
		U.bOk = Rendus >= (int32)UE_ARRAY_COUNT(GCrew);
		U.L.Add(FString::Printf(TEXT("Rendus %d / %d  -  PV max +%d / 2"), Rendus, (int32)UE_ARRAY_COUNT(GCrew), M->HpBonus));
		Q.Add(U);
	}
	if (M && M->bNewtDone)
	{
		Q.Add({ TEXT("La gamine"), FColor(232, 176, 192), true, M->NewtEnd == TEXT("perdue")
			? TArray<FString>{ TEXT("Elle ne souffre plus."), TEXT("Sans Lapin, elle n'a pas pu se calmer.") }
			: TArray<FString>{ TEXT("Lapin l'a calmée. Elle t'a laissé un objet.") } });
	}
	if (M && M->bCobUnlock) { Q.Add({ TEXT("Le Cobaye"), FColor(224, 80, 58), true, { TEXT("Il t'attend au Kibo."), TEXT("Parle-lui au Kibo pour changer de perso.") } }); }
	if (M && M->bVieille) { Q.Add({ TEXT("La vieille dame"), FColor(200, 224, 168), false, { TEXT("Elle attend sous le saule, aux serres."), TEXT("Elle ne sait plus pourquoi elle est là."), TEXT("Quelqu'un lui a dit d'attendre. Qui ?") } }); }
	{
		const TArray<FEONUnlockDef>& U = UEONMetaSubsystem::Unlocks();
		int32 N = 0;
		FQuest D{ TEXT("Déblocages"), Am, false, {} };
		for (const FEONUnlockDef& Ud : U)
		{
			const bool bOn = MS && MS->IsUnlocked(Ud.Id);
			N += bOn ? 1 : 0;
			D.L.Add(bOn ? FString::Printf(TEXT("[X] %s : %s"), *Ud.Kind, *Ud.Name) : FString::Printf(TEXT("[ ] %s ??? : %s"), *Ud.Kind, *Ud.Cond));
		}
		D.bOk = N >= U.Num();
		D.L.Insert(FString::Printf(TEXT("Débloqués : %d / %d  (persos, greffes, cassettes, objets)"), N, U.Num()), 0);
		Q.Add(D);
	}
	const int32 Ln = FMath::Min(M ? M->LogN : 0, NumLogs());
	Q.Add({ TEXT("Archives de la station"), FColor(154, 150, 200), Ln >= NumLogs(), { FString::Printf(TEXT("Logs retrouvés : %d / %d"), Ln, NumLogs()), TEXT("Relis-les dans l'onglet Logs.") } });
	return Q;
}

const TArray<UEONUISubsystem::FCassRow>& UEONUISubsystem::CassRows()
{
	static const TArray<FCassRow> R = {
		{ TEXT("shield"), 2, TEXT(""), TEXT("") },
		{ TEXT("shield"), 3, TEXT("A"), TEXT("") },
		{ TEXT("shield"), 3, TEXT("B"), TEXT("") },
		{ NAME_None, 0, TEXT(""), TEXT("Cassette Mobilité") },
		{ NAME_None, 0, TEXT(""), TEXT("Cassette Cobra") },
	};
	return R;
}

int32 UEONUISubsystem::CassState(const FCassRow& R, FString& OutT, int32& OutCost) const
{
	OutCost = 0;
	if (!R.Lock.IsEmpty()) { OutT = TEXT("À définir (GDD)"); return 3; }
	const UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
	const int32 L = MS ? MS->CasLvl(R.K) : 0;
	const FString B = MS ? MS->CasBr(R.K) : FString();
	static const int32 Cost[4] = { 0, 0, 30, 60 };   // dialogues.json, cassettes.shield.cost
	if (L >= R.Lvl && (R.Br.IsEmpty() || B == R.Br)) { OutT = TEXT("Installe"); return 0; }
	if (L == 0) { OutT = TEXT("NV1 à trouver en run"); return 3; }
	if (R.Lvl == 3 && !B.IsEmpty() && B != R.Br) { OutT = TEXT("Branche ") + B + TEXT(" choisie"); return 3; }
	if (L < R.Lvl - 1) { OutT = FString::Printf(TEXT("NV%d requis"), R.Lvl - 1); return 3; }
	OutCost = Cost[FMath::Clamp(R.Lvl, 0, 3)];
	OutT = FString::Printf(TEXT("%d données"), OutCost);
	return MS && MS->GetDonnees() >= OutCost ? 1 : 2;
}

int32 UEONUISubsystem::ComCount(int32 Tab) const
{
	return Tab == 0 ? ComItems().Num() : Tab == 1 ? NumLogs() : Tab == 2 ? Quests().Num() : Tab == 4 ? CassRows().Num() : 0;
}

void UEONUISubsystem::ComAct()
{
	AEONFloorManager* F = AEONFloorManager::Get(this);
	if (ComTab == 3)
	{
		// pullCOM : le levier.
		if (F && F->IsFloorDone()) { CloseCom(); F->PullLever(); }
		else if (F) { F->ShowBanner(TEXT("Levier verrouillé : neutralise le gardien"), FLinearColor(FColor(140, 134, 180))); }
		return;
	}
	if (ComTab == 4)
	{
		// buyCass : seulement à quai.
		const TArray<FCassRow>& Rows = CassRows();
		if (!Rows.IsValidIndex(ComSel[4])) { return; }
		const FCassRow& R = Rows[ComSel[4]];
		FString T; int32 C = 0;
		const int32 St = CassState(R, T, C);
		if (!IsDocked()) { if (F) { F->ShowBanner(TEXT("Améliorations au Kibo : C.O.M à quai"), FLinearColor(FColor(140, 134, 180))); } return; }
		UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this);
		if (St != 1 || !MS || !MS->BuyCass(R.K, R.Lvl, R.Br, C)) { return; }
		if (AEONCharacter* J = Cast<AEONCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) { J->ApplyCassettes(); }
		if (F) { F->ShowBanner(FString::Printf(TEXT("Cassette Shield NV%d%s"), R.Lvl, R.Br.IsEmpty() ? TEXT("") : *(TEXT(" ") + R.Br)), FLinearColor(FColor(62, 242, 224))); }
		UE_LOG(LogEON, Display, TEXT("EON UI cassette installee : shield NV%d %s"), R.Lvl, *R.Br);
	}
}

// ---------------------------------------------------------------- titre

void UEONUISubsystem::OpenTitle(TFunction<void()> OnNew)
{
	bTitle = true;
	TitleSel = 0;
	bTitleConfirm = false;
	TitleAutoT = 0.f;
	TitleNew = MoveTemp(OnNew);
	SyncPause();
	UE_LOG(LogEON, Display, TEXT("EON UI ecran titre"));
}

/// ---------------------------------------------------------------- menu pause et options (huit onglets : EONOptions)

namespace
{
	// Clavier AZERTY : Z / S / Q / D (et les flèches), comme les aides affichées.
	const FKey KeysUp[] = { EKeys::Up, EKeys::Z, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_LeftStick_Up };
	const FKey KeysDown[] = { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_LeftStick_Down };
	const FKey KeysLeft[] = { EKeys::Left, EKeys::Q, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left };
	const FKey KeysRight[] = { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right };

	template <int32 N> bool AnyPressed(APlayerController* PC, const FKey (&Keys)[N]) { for (const FKey& K : Keys) { if (PC->WasInputKeyJustPressed(K)) { return true; } } return false; }
	template <int32 N> bool AnyDown(APlayerController* PC, const FKey (&Keys)[N]) { for (const FKey& K : Keys) { if (PC->IsInputKeyDown(K)) { return true; } } return false; }

	/** Touche acceptée pour une réassignation. Manette : boutons seulement (pas les directions des sticks, Menu reste la pause).
	 *  Clavier-souris : touches et boutons de souris, sauf Échap (annuler) et la molette. */
	bool AcceptKey(const FKey& K, bool bPad)
	{
		if (!K.IsValid() || K == EKeys::AnyKey) { return false; }
		if (bPad)
		{
			static const FKey Pad[] = {
				EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top,
				EKeys::Gamepad_LeftShoulder, EKeys::Gamepad_RightShoulder, EKeys::Gamepad_LeftTrigger, EKeys::Gamepad_RightTrigger,
				EKeys::Gamepad_LeftThumbstick, EKeys::Gamepad_RightThumbstick, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down,
				EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_Special_Left };
			for (const FKey& P : Pad) { if (P == K) { return true; } }
			return false;
		}
		if (K.IsGamepadKey() || !K.IsDigital() || K.IsTouch() || K.IsGesture()) { return false; }
		if (K == EKeys::Escape || K == EKeys::MouseScrollUp || K == EKeys::MouseScrollDown) { return false; }
		const FString N = K.GetFName().ToString();
		return !N.StartsWith(TEXT("Virtual_")) && !N.StartsWith(TEXT("Global_")) && !N.StartsWith(TEXT("Android_"));
	}
}

void UEONUISubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	EONOptions::ApplyWorld(&InWorld);
}

void UEONUISubsystem::OpenPause()
{
	if (bPause) { return; }
	bPause = true;
	PauseSel = 0;
	PauseConfirm = -1;
	bOptions = false;
	bWaitKey = false;
	bDragSlider = false;
	LastMouse = FVector2D(-1.0, -1.0);
	SyncPause();
	ShowMenuCursor(UGameplayStatics::GetPlayerController(this, 0), true);
	EONSfx::Event(this, TEXT("menu_open"));
	UE_LOG(LogEON, Display, TEXT("EON UI pause ouverte"));
}

void UEONUISubsystem::ClosePause()
{
	if (!bPause) { return; }
	bPause = false;
	bOptions = false;
	bWaitKey = false;
	bDragSlider = false;
	PauseConfirm = -1;
	ClosedAt = FPlatformTime::Seconds();
	SyncPause();
	ShowMenuCursor(UGameplayStatics::GetPlayerController(this, 0), false);
	UE_LOG(LogEON, Display, TEXT("EON UI pause fermee"));
}

void UEONUISubsystem::ShowMenuCursor(APlayerController* PC, bool bMenu)
{
	if (!PC) { return; }
	if (bMenu)
	{
		if (SavedCursor == 255) { SavedCursor = (uint8)PC->CurrentMouseCursor.GetValue(); }
		PC->CurrentMouseCursor = EMouseCursor::Default;   // flèche dans le menu, viseur en jeu
	}
	else if (SavedCursor != 255)
	{
		PC->CurrentMouseCursor = (EMouseCursor::Type)SavedCursor;
		SavedCursor = 255;
	}
}

bool UEONUISubsystem::MouseRef(APlayerController* PC, FVector2D& Out, float& OutW) const
{
	int32 Vx = 0, Vy = 0;
	PC->GetViewportSize(Vx, Vy);
	double Mx = 0.0, My = 0.0;
	if (Vx <= 0 || Vy <= 0 || !PC->GetMousePosition(Mx, My)) { return false; }
	const double S = 1080.0 / Vy;
	Out = FVector2D(Mx * S, My * S);
	OutW = float(Vx * S);
	return true;
}

void UEONUISubsystem::PauseNav(int32 Dir)
{
	PauseSel = (PauseSel + Dir + 3) % 3;
	PauseConfirm = -1;
	EONSfx::Event(this, TEXT("menu_move"));
}

void UEONUISubsystem::PauseAct()
{
	if (PauseSel == 1)
	{
		const AEONPlayerController* EPC = Cast<AEONPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
		OpenOptions(EPC && EPC->IsUsingGamepad());
		return;
	}
	if (PauseConfirm != PauseSel) { PauseConfirm = PauseSel; return; }   // premier appui : « CONFIRMER »
	if (PauseSel == 0)
	{
		ClosePause();
		UE_LOG(LogEON, Display, TEXT("EON UI pause : redemarrer"));
		if (AEONCombatDirector* D = AEONCombatDirector::Get(GetWorld())) { D->RestartRun(); }
		else { UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true))); }
	}
	else
	{
		UE_LOG(LogEON, Display, TEXT("EON UI pause : quitter"));
		UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
	}
}

void UEONUISubsystem::TickPauseMouse(APlayerController* PC)
{
	using namespace EONOptLayout;
	FVector2D M;
	float W = 1920.f;
	if (!MouseRef(PC, M, W)) { return; }
	const bool bMoved = LastMouse.X >= 0.0 && FVector2D::Distance(M, LastMouse) > 0.5;
	LastMouse = M;
	const bool bClick = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
	if ((!bMoved && !bClick) || FMath::Abs(M.X - W * 0.5f) > PauseBtnW * 0.5f) { return; }
	for (int32 I = 0; I < 3; ++I)
	{
		const float Y = PauseBtnY + I * PauseBtnStep;
		if (M.Y < Y || M.Y > Y + PauseBtnH) { continue; }
		if (PauseSel != I) { PauseSel = I; PauseConfirm = -1; EONSfx::Event(this, TEXT("menu_move")); }
		if (bClick) { PauseAct(); }
		return;
	}
}

void UEONUISubsystem::OpenOptions(bool bPad)
{
	bOptions = true;
	PauseConfirm = -1;
	OptTab = 0;
	OptSel = 0;
	OptScroll = 0;
	OptCol = bPad ? 1 : 0;
	bWaitKey = false;
	bDragSlider = false;
	RepDir = 0;
	OptNav(0);
	EONSfx::Event(this, TEXT("menu_open"));
	UE_LOG(LogEON, Display, TEXT("EON UI options ouvertes"));
}

void UEONUISubsystem::OptSetTab(int32 Tab)
{
	Tab = (Tab % EONOptions::NumTabs + EONOptions::NumTabs) % EONOptions::NumTabs;
	if (Tab == OptTab) { return; }
	OptTab = Tab;
	OptSel = 0;
	OptScroll = 0;
	bDragSlider = false;
	OptNav(0);
	EONSfx::Event(this, TEXT("menu_move"));
	UE_LOG(LogEON, Display, TEXT("EON UI options onglet %s"), *EONOptions::TabName(OptTab));
}

void UEONUISubsystem::OptNav(int32 Dir)
{
	const TArray<EONOptions::FRow> R = EONOptions::Rows(OptTab);
	const int32 N = R.Num() + 1;   // + RETOUR
	int32 S = FMath::Clamp(OptSel, 0, N - 1);
	if (Dir == 0)
	{
		while (S < R.Num() && R[S].Kind == EONOptions::EKind::Section) { ++S; }   // jamais sur un intertitre
	}
	else
	{
		for (int32 K = 0; K < N; ++K)
		{
			S = (S + Dir + N) % N;
			if (S == R.Num() || R[S].Kind != EONOptions::EKind::Section) { break; }
		}
		EONSfx::Event(this, TEXT("menu_move"));
	}
	OptSel = S;
	OptKeepVisible();
}

void UEONUISubsystem::OptKeepVisible()
{
	using namespace EONOptLayout;
	const TArray<EONOptions::FRow> R = EONOptions::Rows(OptTab);
	if (OptSel < R.Num())
	{
		// L'intertitre juste au-dessus reste visible avec sa première ligne.
		const int32 Top = OptSel > 0 && R[OptSel - 1].Kind == EONOptions::EKind::Section ? OptSel - 1 : OptSel;
		if (Top < OptScroll) { OptScroll = Top; }
		if (OptSel >= OptScroll + RowsVisible) { OptScroll = OptSel - RowsVisible + 1; }
	}
	OptScroll = FMath::Clamp(OptScroll, 0, FMath::Max(0, R.Num() - RowsVisible));
}

void UEONUISubsystem::OptSide(int32 Dir)
{
	const TArray<EONOptions::FRow> R = EONOptions::Rows(OptTab);
	if (!R.IsValidIndex(OptSel)) { return; }
	const EONOptions::FRow& Row = R[OptSel];
	if (Row.Kind == EONOptions::EKind::Bind)
	{
		// Gauche / droite : colonne clavier-souris ou manette.
		const int32 Col = Dir > 0 ? 1 : 0;
		if (OptColFor(OptCol, Row.bHasKb, Row.bHasPad) != Col && (Col == 1 ? Row.bHasPad : Row.bHasKb)) { OptCol = Col; EONSfx::Event(this, TEXT("menu_move")); }
		return;
	}
	if (Row.Kind == EONOptions::EKind::Action || Row.Kind == EONOptions::EKind::Section || !Row.bEnabled) { return; }
	EONOptions::Change(GetWorld(), OptTab, OptSel, Dir);
	EONSfx::Event(this, TEXT("menu_move"));
	OptKeepVisible();
}

void UEONUISubsystem::OptActivate()
{
	const TArray<EONOptions::FRow> R = EONOptions::Rows(OptTab);
	if (OptSel >= R.Num()) { OptBack(); return; }
	const EONOptions::FRow& Row = R[OptSel];
	if (Row.Kind == EONOptions::EKind::Section || !Row.bEnabled) { return; }
	const bool bPad = Row.Kind == EONOptions::EKind::Bind && OptColFor(OptCol, Row.bHasKb, Row.bHasPad) == 1;
	if (EONOptions::Activate(GetWorld(), OptTab, OptSel, bPad))
	{
		bWaitKey = true;
		bWaitPad = bPad;
		WaitT = 0.f;
		EONSfx::Event(this, TEXT("menu_open"));
		UE_LOG(LogEON, Display, TEXT("EON UI options : en attente d'une touche %s pour %s"), bPad ? TEXT("manette") : TEXT("clavier"), *Row.Label);
		return;
	}
	EONSfx::Event(this, TEXT("menu_move"));
	OptKeepVisible();
}

void UEONUISubsystem::OptBack()
{
	bOptions = false;
	bWaitKey = false;
	bDragSlider = false;
	PauseSel = 1;
	PauseConfirm = -1;
	EONSfx::Event(this, TEXT("menu_move"));
}

void UEONUISubsystem::OptMouse(APlayerController* PC)
{
	using namespace EONOptLayout;
	FVector2D M;
	float W = 1920.f;
	if (!MouseRef(PC, M, W)) { return; }
	const float Cx = W * 0.5f, Lx = Cx + ListDx;
	const bool bMoved = LastMouse.X >= 0.0 && FVector2D::Distance(M, LastMouse) > 0.5;
	LastMouse = M;
	const bool bClick = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
	const TArray<EONOptions::FRow> R = EONOptions::Rows(OptTab);
	const float SlT = (M.X - (Lx + SlX0)) / (SlX1 - SlX0);
	// Curseur tenu : la valeur suit la souris jusqu'au relâchement.
	if (bDragSlider)
	{
		if (!PC->IsInputKeyDown(EKeys::LeftMouseButton) || !R.IsValidIndex(OptSel)) { bDragSlider = false; }
		else if (bMoved) { EONOptions::SetFraction(GetWorld(), OptTab, OptSel, SlT); }
		return;
	}
	// Molette : la liste défile (Commandes).
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp)) { OptScroll = FMath::Max(0, OptScroll - 3); }
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown)) { OptScroll = FMath::Clamp(OptScroll + 3, 0, FMath::Max(0, R.Num() - RowsVisible)); }
	if (!bMoved && !bClick) { return; }
	// Onglets.
	if (M.Y >= TabY && M.Y <= TabY + TabH && M.X >= Cx - TabSpan && M.X < Cx + TabSpan)
	{
		if (bClick) { OptSetTab(FMath::Clamp(FMath::FloorToInt((M.X - (Cx - TabSpan)) / TabW), 0, EONOptions::NumTabs - 1)); }
		return;
	}
	// Bouton RETOUR.
	if (FMath::Abs(M.X - Cx) <= BackW * 0.5f && M.Y >= BackY && M.Y <= BackY + BackH)
	{
		if (OptSel != R.Num()) { OptSel = R.Num(); EONSfx::Event(this, TEXT("menu_move")); }
		if (bClick) { OptBack(); }
		return;
	}
	// Lignes.
	if (M.X < Lx || M.X > Lx + ListW || M.Y < ListY || M.Y >= ListY + RowsVisible * RowH) { return; }
	const int32 Row = OptScroll + FMath::FloorToInt((M.Y - ListY) / RowH);
	if (!R.IsValidIndex(Row) || R[Row].Kind == EONOptions::EKind::Section) { return; }
	const EONOptions::FRow& Rw = R[Row];
	const float X = M.X - Lx;
	const bool bInKb = X >= KbX0 && X <= KbX1 && Rw.bHasKb, bInPad = X >= PadX0 && X <= PadX1 && Rw.bHasPad;
	if (Rw.Kind == EONOptions::EKind::Bind && (bInKb || bInPad)) { OptCol = bInPad ? 1 : 0; }
	if (Row != OptSel) { OptSel = Row; EONSfx::Event(this, TEXT("menu_move")); }
	if (!bClick || !Rw.bEnabled) { return; }
	const bool bInVal = X >= ValX0 && X <= ValX1;
	switch (Rw.Kind)
	{
	case EONOptions::EKind::Slider:
		if (bInVal) { bDragSlider = true; EONOptions::SetFraction(GetWorld(), OptTab, Row, SlT); }
		break;
	case EONOptions::EKind::Choice:
	case EONOptions::EKind::Toggle:
		if (bInVal && X < ValX0 + ArrowW) { OptSide(-1); }
		else if (bInVal && X > ValX1 - ArrowW) { OptSide(1); }
		else if (bInVal) { OptActivate(); }
		break;
	case EONOptions::EKind::Action:
		if (bInVal) { OptActivate(); }
		break;
	case EONOptions::EKind::Bind:
		if (bInKb || bInPad) { OptActivate(); }
		break;
	default:
		break;
	}
}

void UEONUISubsystem::TickOptions(APlayerController* PC, float RealDt)
{
	// --- Réassignation : la prochaine touche (ou le prochain bouton) devient celle de la ligne.
	if (bWaitKey)
	{
		WaitT += RealDt;
		if (WaitT < 0.15f) { return; }   // la touche qui a lancé l'attente ne compte pas
		if (PC->WasInputKeyJustPressed(EKeys::Escape) || (bWaitPad && PC->WasInputKeyJustPressed(EKeys::Gamepad_Special_Right)))
		{
			bWaitKey = false;
			OptIgnoreT = 0.2f;
			EONSfx::Event(this, TEXT("menu_move"));
			UE_LOG(LogEON, Display, TEXT("EON UI options : reassignation annulee"));
			return;
		}
		static TArray<FKey> All;
		if (All.IsEmpty()) { EKeys::GetAllKeys(All); }
		for (const FKey& K : All)
		{
			if (!PC->WasInputKeyJustPressed(K) || !AcceptKey(K, bWaitPad)) { continue; }
			EONOptions::Rebind(GetWorld(), OptTab, OptSel, bWaitPad, K);
			bWaitKey = false;
			OptIgnoreT = 0.25f;
			EONSfx::Event(this, TEXT("menu_move"));
			return;
		}
		return;
	}
	if (OptIgnoreT > 0.f) { OptIgnoreT -= RealDt; return; }
	OptMouse(PC);
	if (!bOptions || bWaitKey) { return; }   // un clic a pu fermer les options ou lancer une réassignation
	if (Pressed(PC, { EKeys::Escape, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Right })) { OptBack(); return; }
	// Onglets : Tab (Maj+Tab), Page préc./suiv., LB / RB, 1 à 8 (rangée du haut du clavier actif : & é " ' en AZERTY).
	const bool bShift = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
	if (PC->WasInputKeyJustPressed(EKeys::Tab)) { OptSetTab(OptTab + (bShift ? -1 : 1)); }
	if (Pressed(PC, { EKeys::PageUp, EKeys::Gamepad_LeftShoulder })) { OptSetTab(OptTab - 1); }
	if (Pressed(PC, { EKeys::PageDown, EKeys::Gamepad_RightShoulder })) { OptSetTab(OptTab + 1); }
	for (int32 I = 0; I < EONOptions::NumTabs; ++I) { if (PC->WasInputKeyJustPressed(EONOptions::DigitKey(I + 1))) { OptSetTab(I); } }
	// Flèches : un appui, puis répétition après 0,42 s (curseurs, longues listes).
	int32 NavV = AnyPressed(PC, KeysUp) ? -1 : AnyPressed(PC, KeysDown) ? 1 : 0;
	int32 NavH = AnyPressed(PC, KeysLeft) ? -1 : AnyPressed(PC, KeysRight) ? 1 : 0;
	const int32 HeldDir = AnyDown(PC, KeysUp) ? -10 : AnyDown(PC, KeysDown) ? 10 : AnyDown(PC, KeysLeft) ? -1 : AnyDown(PC, KeysRight) ? 1 : 0;
	if (NavV || NavH) { RepDir = NavV ? NavV * 10 : NavH; RepT = 0.f; }
	else if (HeldDir != 0 && HeldDir == RepDir)
	{
		RepT += RealDt;
		if (RepT > 0.42f)
		{
			RepT -= 0.07f;
			if (FMath::Abs(RepDir) >= 10) { NavV = RepDir / 10; } else { NavH = RepDir; }
		}
	}
	else { RepDir = 0; RepT = 0.f; }
	if (NavV) { OptNav(NavV); }
	if (NavH) { OptSide(NavH); }
	if (Pressed(PC, { EKeys::Enter, EKeys::SpaceBar, EKeys::E, EKeys::Gamepad_FaceButton_Bottom })) { OptActivate(); }
}

// ---------------------------------------------------------------- commun

void UEONUISubsystem::DevAction(const FString& A)
{
	if (bTitle && A == TEXT("continuer")) { TitleSel = 0; bTitle = false; ClosedAt = FPlatformTime::Seconds(); SyncPause(); }
	else if (bTitle && A == TEXT("nouvelle")) { TitleSel = 1; if (!bTitleConfirm) { bTitleConfirm = true; } else { bTitle = false; if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->Wipe(); } SyncPause(); if (TitleNew) { TitleNew(); } } }
	else if (IsDialogOpen() && A == TEXT("suite")) { DlgAdvance(); }
	else if (IsDialogOpen() && A == TEXT("passer")) { DlgClose(); }
	else if (A == TEXT("pause")) { if (bPause) { ClosePause(); } else if (!IsBlocking()) { OpenPause(); } }
	else if (bPause && A == TEXT("options")) { if (!bOptions) { PauseSel = 1; OpenOptions(false); } }
	else if (bPause && bOptions && A.StartsWith(TEXT("onglet "))) { OptSetTab(FCString::Atoi(*A.RightChop(7)) - 1); }
	else if (bPause && bOptions && A.StartsWith(TEXT("colonne "))) { OptCol = FCString::Atoi(*A.RightChop(8)) != 0 ? 1 : 0; }
	else if (bPause && bOptions && A.StartsWith(TEXT("ligne "))) { OptSel = FMath::Max(0, FCString::Atoi(*A.RightChop(6))); OptNav(0); }
	else if (bPause && bOptions && bWaitKey && A.StartsWith(TEXT("touche ")))
	{
		const FKey K = FKey(FName(*A.RightChop(7)));
		if (AcceptKey(K, bWaitPad)) { EONOptions::Rebind(GetWorld(), OptTab, OptSel, bWaitPad, K); bWaitKey = false; }
	}
	else if (bPause && A == TEXT("haut")) { if (bOptions) { OptNav(-1); } else { PauseNav(-1); } }
	else if (bPause && A == TEXT("bas")) { if (bOptions) { OptNav(1); } else { PauseNav(1); } }
	else if (bPause && bOptions && A == TEXT("gauche")) { OptSide(-1); }
	else if (bPause && bOptions && A == TEXT("droite")) { OptSide(1); }
	else if (bPause && A == TEXT("valider")) { if (bOptions) { OptActivate(); } else { PauseAct(); } }
	else if (bPause && A == TEXT("retour")) { if (bWaitKey) { bWaitKey = false; } else if (bOptions) { OptBack(); } else { ClosePause(); } }
	UE_LOG(LogEON, Display, TEXT("EON UI eon.ui %s"), *A);
}

bool UEONUISubsystem::JustClosed() const { return FPlatformTime::Seconds() - ClosedAt < 0.25; }

void UEONUISubsystem::SyncPause()
{
	const bool bNeed = IsBlocking();
	if (bNeed && !bWePaused) { bWePaused = true; UGameplayStatics::SetGamePaused(this, true); }
	else if (!bNeed && bWePaused) { bWePaused = false; UGameplayStatics::SetGamePaused(this, false); }
}

void UEONUISubsystem::TickUI(APlayerController* PC, float RealDt)
{
	const int32 Auto = AutoMode();
	// Le menu F6 se ferme aussi par Échap : à l'image où il se ferme, Échap n'ouvre pas la pause.
	const AEONPlayerController* EPC = Cast<AEONPlayerController>(PC);
	const bool bDevNow = EPC && EPC->GetDevMenu().IsOpen(), bDevWas = bDevWasOpen;
	bDevWasOpen = bDevNow;
	// --- Écran titre
	if (bTitle)
	{
		if (Auto > 0 && (TitleAutoT += RealDt) > 0.5f) { bTitle = false; SyncPause(); UE_LOG(LogEON, Display, TEXT("EON UI titre : continuer (pilote)")); return; }
		if (!PC) { return; }
		if (Pressed(PC, { EKeys::Left, EKeys::Right, EKeys::Up, EKeys::Down, EKeys::Q, EKeys::D, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Left, EKeys::Gamepad_LeftStick_Right }))
		{
			TitleSel = 1 - TitleSel;
		}
		if (Pressed(PC, { EKeys::Enter, EKeys::SpaceBar, EKeys::E, EKeys::Gamepad_FaceButton_Bottom }))
		{
			if (TitleSel == 0)
			{
				bTitle = false;
				ClosedAt = FPlatformTime::Seconds();
				SyncPause();
				UE_LOG(LogEON, Display, TEXT("EON UI titre : continuer"));
			}
			else if (!bTitleConfirm) { bTitleConfirm = true; }
			else
			{
				bTitle = false;
				ClosedAt = FPlatformTime::Seconds();
				if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->Wipe(); }
				SyncPause();
				UE_LOG(LogEON, Display, TEXT("EON UI titre : nouvelle partie"));
				if (TitleNew) { TitleNew(); }
			}
		}
		return;
	}
	// --- Menu pause et Options
	if (bPause)
	{
		if (!PC) { return; }
		if (bOptions) { TickOptions(PC, RealDt); return; }
		if (Pressed(PC, { EKeys::Escape, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Right })) { ClosePause(); return; }
		if (AnyPressed(PC, KeysUp)) { PauseNav(-1); }
		if (AnyPressed(PC, KeysDown)) { PauseNav(1); }
		if (Pressed(PC, { EKeys::Enter, EKeys::SpaceBar, EKeys::E, EKeys::Gamepad_FaceButton_Bottom })) { PauseAct(); return; }
		TickPauseMouse(PC);
		return;
	}
	// --- Ouverture de la pause : Échap ou Menu (manette), jeu libre.
	if (PC && !IsBlocking() && !bDevNow && !bDevWas && !JustClosed() && Pressed(PC, { EKeys::Escape, EKeys::Gamepad_Special_Right }))
	{
		const AEONCharacter* J = Cast<AEONCharacter>(PC->GetPawn());
		const AEONFloorManager* F = AEONFloorManager::Get(this);
		if (J && !J->IsDead() && !J->IsLevelUpOpen() && !J->IsInQTE() && !(F && F->IsCinematic())) { OpenPause(); return; }
	}
	// --- Dialogue en cours
	if (IsDialogOpen())
	{
		const FEONDlgLine* L = DlgLine();
		// 46 lettres par seconde, multipliées par la vitesse du texte (Accessibilité ; 0 = tout d'un coup).
		const float Speed = EONOptions::TextSpeed();
		if (L && DlgShownF < L->T.Len()) { DlgShownF = Speed <= 0.f ? (float)L->T.Len() : FMath::Min((float)L->T.Len(), DlgShownF + RealDt * 46.f * Speed); }
		if (Auto > 0 && (DlgAutoT += RealDt) > 0.3f) { DlgAutoT = 0.f; DlgShownF = 1e6f; DlgAdvance(); return; }
		if (!PC) { return; }
		if (Pressed(PC, { EKeys::Escape, EKeys::Gamepad_FaceButton_Right })) { DlgClose(); return; }
		if (Pressed(PC, { EKeys::SpaceBar, EKeys::E, EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom, EKeys::LeftMouseButton })) { DlgAdvance(); }
		return;
	}
	// --- Terminal
	if (bCom)
	{
		if (Auto > 0 && (ComAutoT += RealDt) > 0.5f)
		{
			ComTab = 3;
			const AEONFloorManager* F = AEONFloorManager::Get(this);
			if (F && F->IsFloorDone()) { ComAct(); } else { CloseCom(); }
			return;
		}
		if (!PC) { return; }
		// Fermer : la touche d'interaction choisie dans Commandes (C d'origine), Échap ou B. Entrée et Espace restent au levier.
		const FKey IKey = EPC ? EONOptions::RowKey(EPC->GetActiveContext(), TEXT("Interagir"), false) : FKey(EKeys::C);
		const bool bIKey = IKey.IsValid() && IKey != EKeys::Enter && IKey != EKeys::SpaceBar && PC->WasInputKeyJustPressed(IKey);
		if (bIKey || Pressed(PC, { EKeys::Escape, EKeys::Gamepad_FaceButton_Right })) { CloseCom(); return; }
		int32 Tab = ComTab;
		if (Pressed(PC, { EKeys::Left, EKeys::Q, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftShoulder })) { Tab = (Tab + 4) % 5; }
		if (Pressed(PC, { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_RightShoulder })) { Tab = (Tab + 1) % 5; }
		for (int32 I = 0; I < 5; ++I) { if (PC->WasInputKeyJustPressed(EONOptions::DigitKey(I + 1))) { Tab = I; } }
		if (Tab != ComTab) { ComTab = Tab; EONSfx::Event(this, TEXT("menu_move")); UE_LOG(LogEON, Display, TEXT("EON UI terminal onglet %d"), ComTab); }
		const int32 N = ComCount(ComTab);
		if (N > 0)
		{
			const int32 Sel0 = ComSel[ComTab];
			if (Pressed(PC, { EKeys::Up, EKeys::Z, EKeys::Gamepad_DPad_Up })) { ComSel[ComTab] = (ComSel[ComTab] + N - 1) % N; }
			if (Pressed(PC, { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down })) { ComSel[ComTab] = (ComSel[ComTab] + 1) % N; }
			if (ComSel[ComTab] != Sel0) { EONSfx::Event(this, TEXT("menu_move")); }
		}
		if (Pressed(PC, { EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom })) { ComAct(); }
		return;
	}
	// --- Dialogue en attente : seulement quand le jeu est libre (pendingDialog && state === 'play').
	if (Pending.Num())
	{
		const AEONCharacter* J = PC ? Cast<AEONCharacter>(PC->GetPawn()) : nullptr;
		const AEONFloorManager* F = AEONFloorManager::Get(this);
		const bool bFree = J && !J->IsDead() && !J->IsLevelUpOpen() && !J->IsInQTE() && !UGameplayStatics::IsGamePaused(this) && !(F && F->IsInTransition())
			&& !(F && F->SinceRoomReady() < 1.0); // 1 s après un chargement de salle (16/11)
		if (bFree)
		{
			FPending P = MoveTemp(Pending[0]);
			Pending.RemoveAt(0);
			Dlg = MoveTemp(P.L);
			DlgEnd = MoveTemp(P.OnEnd);
			DlgI = 0;
			DlgShownF = 0.f;
			DlgAutoT = 0.f;
			SyncPause();
			UE_LOG(LogEON, Display, TEXT("EON UI dialogue ouvert (%d repliques) : %s"), Dlg.Num(), *Dlg[0].T.Left(60));
		}
	}
}
