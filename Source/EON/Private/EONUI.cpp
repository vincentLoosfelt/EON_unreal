// EON — jalon 8 : dialogues, terminal du C.O.M et écran titre (openDialog, dialogAdvance, openCOM, comQuests, cassState,
// buyCass, titleChoose du proto v0.60.1).
#include "EONUI.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONItems.h"
#include "EONMeta.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

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
			if (UEONUISubsystem* S = UEONUISubsystem::Get(W); S && A.Num()) { S->DevAction(A[0]); }
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

// ---------------------------------------------------------------- commun

void UEONUISubsystem::DevAction(const FString& A)
{
	if (bTitle && A == TEXT("continuer")) { TitleSel = 0; bTitle = false; ClosedAt = FPlatformTime::Seconds(); SyncPause(); }
	else if (bTitle && A == TEXT("nouvelle")) { TitleSel = 1; if (!bTitleConfirm) { bTitleConfirm = true; } else { bTitle = false; if (UEONMetaSubsystem* MS = UEONMetaSubsystem::Get(this)) { MS->Wipe(); } SyncPause(); if (TitleNew) { TitleNew(); } } }
	else if (IsDialogOpen() && A == TEXT("suite")) { DlgAdvance(); }
	else if (IsDialogOpen() && A == TEXT("passer")) { DlgClose(); }
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
	// --- Écran titre
	if (bTitle)
	{
		if (Auto > 0 && (TitleAutoT += RealDt) > 0.5f) { bTitle = false; SyncPause(); UE_LOG(LogEON, Display, TEXT("EON UI titre : continuer (pilote)")); return; }
		if (!PC) { return; }
		if (Pressed(PC, { EKeys::Left, EKeys::Right, EKeys::Up, EKeys::Down, EKeys::Q, EKeys::D, EKeys::A, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Left, EKeys::Gamepad_LeftStick_Right }))
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
	// --- Dialogue en cours
	if (IsDialogOpen())
	{
		const FEONDlgLine* L = DlgLine();
		if (L && DlgShownF < L->T.Len()) { DlgShownF = FMath::Min((float)L->T.Len(), DlgShownF + RealDt * 46.f); }
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
		if (Pressed(PC, { EKeys::C, EKeys::Escape, EKeys::Gamepad_FaceButton_Right })) { CloseCom(); return; }
		int32 Tab = ComTab;
		if (Pressed(PC, { EKeys::Left, EKeys::Q, EKeys::A, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftShoulder })) { Tab = (Tab + 4) % 5; }
		if (Pressed(PC, { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_RightShoulder })) { Tab = (Tab + 1) % 5; }
		static const FKey Nums[5] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
		for (int32 I = 0; I < 5; ++I) { if (PC->WasInputKeyJustPressed(Nums[I])) { Tab = I; } }
		if (Tab != ComTab) { ComTab = Tab; UE_LOG(LogEON, Display, TEXT("EON UI terminal onglet %d"), ComTab); }
		const int32 N = ComCount(ComTab);
		if (N > 0)
		{
			if (Pressed(PC, { EKeys::Up, EKeys::Z, EKeys::W, EKeys::Gamepad_DPad_Up })) { ComSel[ComTab] = (ComSel[ComTab] + N - 1) % N; }
			if (Pressed(PC, { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down })) { ComSel[ComTab] = (ComSel[ComTab] + 1) % N; }
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
