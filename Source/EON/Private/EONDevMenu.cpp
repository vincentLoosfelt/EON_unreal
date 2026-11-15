#include "EONDevMenu.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONTuningSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// Actions du menu Triche (et deux du menu Tweak).
	enum EAct : int32
	{
		A_Preset = 0, A_Export, A_Reset,
		A_Heal, A_Kill, A_Wave, A_Adn, A_Scrap, A_GoRole, A_Door, A_Floor, A_Auto, A_Unlock, A_Donnees,
	};
	const TCHAR* Presets[] = { TEXT("studio"), TEXT("sec"), TEXT("max") };

	bool ParseBool(const FString& S) { return S.Equals(TEXT("True"), ESearchCase::IgnoreCase) || S == TEXT("1"); }

	FString Fmt(float V, float Step)
	{
		if (Step >= 1.f) { return FString::Printf(TEXT("%.0f"), V); }
		if (Step >= 0.05f) { return FString::Printf(TEXT("%.2f"), V); }
		return FString::Printf(TEXT("%.3f"), V);
	}

	/** Rôles présents sur l'étage, dans un ordre stable. */
	TArray<FName> FloorRoles(const UObject* Ctx)
	{
		TArray<FName> Out;
		if (const AEONFloorManager* FM = AEONFloorManager::Get(Ctx))
		{
			for (const TPair<FIntPoint, FEONFloorCell>& KV : FM->GetCells()) { Out.AddUnique(KV.Value.Role); }
		}
		Out.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		return Out;
	}
}

void FEONDevMenu::Rebuild()
{
	Items.Reset();
	auto Sec = [this](const TCHAR* L) { FItem I; I.Label = L; I.bSection = true; Items.Add(I); };
	auto Val = [this](const TCHAR* L, const TCHAR* K, float Step, float Min = 0.f) { FItem I; I.Label = L; I.Key = K; I.Step = Step; I.Min = Min; Items.Add(I); };
	auto Sw = [this](const TCHAR* L, const TCHAR* K) { FItem I; I.Label = L; I.Key = K; Items.Add(I); };
	auto Act = [this](const TCHAR* L, int32 A) { FItem I; I.Label = L; I.Action = A; Items.Add(I); };

	if (Open == EEONDevMenu::Tweak)
	{
		Sec(TEXT("Banc"));
		Act(TEXT("Préréglage"), A_Preset);
		Act(TEXT("Exporter les réglages"), A_Export);
		Act(TEXT("Tout remettre (studio)"), A_Reset);
		Sec(TEXT("Jira : déplacement"));
		Val(TEXT("Vitesse de marche (x)"), TEXT("SpeedMul3D"), 0.05f, 0.05f);
		Val(TEXT("Vitesse de course (x)"), TEXT("RunMul"), 0.05f, 0.1f);
		Val(TEXT("Distance du dash (x)"), TEXT("DashMul"), 0.05f, 0.1f);
		Sec(TEXT("Jira : tir"));
		Val(TEXT("Vitesse des balles (x)"), TEXT("BulletSpeed"), 0.05f, 0.05f);
		Val(TEXT("Portée (x)"), TEXT("RangeMul3D"), 0.05f, 0.05f);
		Val(TEXT("Cadence (x)"), TEXT("RateMul"), 0.05f, 0.05f);
		Val(TEXT("Dispersion (x)"), TEXT("Spread"), 0.05f);
		Val(TEXT("Recul de Jira (x)"), TEXT("Recoil"), 0.1f);
		Val(TEXT("Aide à la visée"), TEXT("AimAssist"), 0.01f);
		Val(TEXT("Coût en énergie (x)"), TEXT("EnergyCost"), 0.05f);
		Sw(TEXT("Flash de bouche"), TEXT("Flash"));
		Sec(TEXT("Impacts"));
		Val(TEXT("Tremblement (x)"), TEXT("Shake"), 0.1f);
		Val(TEXT("Arrêt sur image (ms)"), TEXT("Hitstop"), 5.f);
		Val(TEXT("Recul des ennemis (x)"), TEXT("Knock"), 0.1f);
		Sw(TEXT("Flash blanc au coup"), TEXT("HitFlash"));
		Sw(TEXT("Chiffres de dégâts"), TEXT("DmgNums"));
		Sec(TEXT("Caméra"));
		Val(TEXT("Kick au tir (x)"), TEXT("CamKick"), 0.1f);
		Val(TEXT("Anticipation (px)"), TEXT("Lookahead"), 1.f);
		Val(TEXT("Inclinaison (°)"), TEXT("CamPitch"), 1.f, 20.f);
		Val(TEXT("Focale (FOV °)"), TEXT("CamFov"), 1.f, 5.f);
		Val(TEXT("Largeur de vue (tuiles)"), TEXT("CamViewTiles"), 1.f, 8.f);
		Sec(TEXT("Ennemis"));
		Val(TEXT("Points de vie (x)"), TEXT("EnemyHp"), 0.1f, 0.1f);
		Val(TEXT("Vitesse (x)"), TEXT("EnemySpeed"), 0.05f, 0.05f);
		Val(TEXT("Dégâts sur Jira (coeurs)"), TEXT("EnemyDmg"), 1.f, 0.f);
		Val(TEXT("Taille des vagues (x)"), TEXT("WaveMul"), 0.1f, 0.1f);
		Sec(TEXT("Mutation et vide"));
		Val(TEXT("Durée d'un stade (s)"), TEXT("MutDur"), 0.5f, 0.5f);
		Val(TEXT("Armure : blocage"), TEXT("ArmBlock"), 0.02f);
		Val(TEXT("Armure : coût"), TEXT("ArmCost"), 0.02f);
		Val(TEXT("Armure : mutation"), TEXT("ArmMut"), 0.02f);
		Val(TEXT("Dégâts de chute (x)"), TEXT("FallDmg"), 0.1f);
		Sec(TEXT("Rendu"));
		Sw(TEXT("Filtre peinture (post-process)"), TEXT("PPPaint"));
		Sw(TEXT("Post-process complet (expo, bloom)"), TEXT("PPAll"));
		Sec(TEXT("Interface"));
		Sw(TEXT("Texte de debug (haut gauche)"), TEXT("DebugText"));
	}
	else if (Open == EEONDevMenu::Triche)
	{
		Sec(TEXT("Jira"));
		Sw(TEXT("Invincible (vie infinie)"), TEXT("God"));
		Sw(TEXT("Énergie infinie"), TEXT("InfEnergy"));
		Sw(TEXT("Dash infini"), TEXT("InfDash"));
		Sw(TEXT("Tue en un coup"), TEXT("OneShot"));
		Act(TEXT("Soigner (coeurs et bouclier)"), A_Heal);
		Act(TEXT("+100 ADN"), A_Adn);
		Act(TEXT("+100 scrap"), A_Scrap);
		Act(TEXT("+50 Données"), A_Donnees);
		Act(TEXT("Tout débloquer"), A_Unlock);
		Act(TEXT("Pilote automatique"), A_Auto);
		Sec(TEXT("Salle"));
		Act(TEXT("Tuer les ennemis"), A_Kill);
		Act(TEXT("Lancer une vague"), A_Wave);
		Sec(TEXT("Étage"));
		Act(TEXT("Aller à"), A_GoRole);
		Act(TEXT("Passer une porte (case neuve)"), A_Door);
		Act(TEXT("Étage suivant"), A_Floor);
	}
}

void FEONDevMenu::Tick(APlayerController* PC, float RealDt)
{
	if (!PC) { return; }
	MessageT = FMath::Max(0.f, MessageT - RealDt);
	auto Pressed = [PC](const FKey& K) { return PC->WasInputKeyJustPressed(K); };
	auto Down = [PC](const FKey& K) { return PC->IsInputKeyDown(K); };

	// F6 / F7 : ouvre, change de menu ou ferme. Échap ferme aussi.
	EEONDevMenu Want = Open;
	if (Pressed(EKeys::F6)) { Want = Open == EEONDevMenu::Tweak ? EEONDevMenu::None : EEONDevMenu::Tweak; }
	if (Pressed(EKeys::F7)) { Want = Open == EEONDevMenu::Triche ? EEONDevMenu::None : EEONDevMenu::Triche; }
	if (IsOpen() && Pressed(EKeys::Escape)) { Want = EEONDevMenu::None; }
	if (Want != Open) { Open = Want; Rebuild(); }
	if (!IsOpen() || !Items.Num()) { return; }

	int32& S = Sel[(int32)Open];
	S = FMath::Clamp(S, 0, Items.Num() - 1);
	const bool bShift = Down(EKeys::LeftShift) || Down(EKeys::RightShift);
	auto Move = [&](int32 D)
	{
		// Maj : saute à la rubrique suivante ; les titres ne se sélectionnent pas.
		do { S = (S + D + Items.Num()) % Items.Num(); } while (Items[S].bSection || (bShift && S > 0 && !Items[S - 1].bSection));
	};
	if (Pressed(EKeys::Up)) { Move(-1); }
	if (Pressed(EKeys::Down)) { Move(1); }
	if (Items[S].bSection) { Move(1); }

	// Gauche / droite : un pas, puis répétition si on garde la touche.
	int32 Dir = 0;
	const bool bL = Down(EKeys::Left), bR = Down(EKeys::Right);
	if (Pressed(EKeys::Left)) { Dir = -1; HoldT = 0.f; RepeatT = 0.f; }
	else if (Pressed(EKeys::Right)) { Dir = 1; HoldT = 0.f; RepeatT = 0.f; }
	else if (bL || bR)
	{
		HoldT += RealDt;
		if (HoldT > 0.35f && (RepeatT -= RealDt) <= 0.f) { Dir = bR ? 1 : -1; RepeatT = 0.06f; }
	}
	const bool bEnter = Pressed(EKeys::Enter);
	if (Dir != 0 || bEnter) { Change(PC, Items[S], Dir, bEnter); }
}

void FEONDevMenu::Change(APlayerController* PC, const FItem& It, int32 Dir, bool bEnter)
{
	if (It.Action >= 0) { Activate(PC, It.Action, Dir, bEnter); return; }
	UEONTuningSubsystem* T = UEONTuningSubsystem::Get(PC);
	if (!T || It.Key.IsEmpty()) { return; }
	const FString Cur = T->GetValueAsString(It.Key);
	if (It.Step <= 0.f)
	{
		T->SetValue(It.Key, ParseBool(Cur) ? TEXT("False") : TEXT("True"));
		return;
	}
	if (Dir == 0) { return; }
	const bool bShift = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
	const bool bCtrl = PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);
	const float Step = It.Step * (bShift ? 10.f : 1.f) * (bCtrl ? 0.2f : 1.f);
	float V = FCString::Atof(*Cur) + Dir * Step;
	V = FMath::Max(It.Min, FMath::RoundToFloat(V / (Step * 0.5f)) * Step * 0.5f);   // évite 0.30000001
	T->SetValue(It.Key, FString::SanitizeFloat(V));
}

void FEONDevMenu::Activate(APlayerController* PC, int32 Action, int32 Dir, bool bEnter)
{
	UEONTuningSubsystem* T = UEONTuningSubsystem::Get(PC);
	AEONCharacter* J = Cast<AEONCharacter>(PC->GetPawn());
	const bool bDo = bEnter || Dir > 0;
	switch (Action)
	{
	case A_Preset:
		if (Dir == 0 && !bEnter) { return; }
		PresetIdx = (PresetIdx + (Dir < 0 ? 2 : 1)) % 3;
		if (T && T->ApplyPreset(Presets[PresetIdx])) { Flash(FString::Printf(TEXT("Préréglage %s chargé"), Presets[PresetIdx])); }
		return;
	case A_Reset:
		if (bDo && T) { T->ApplyPreset(TEXT("studio")); PresetIdx = 0; Flash(TEXT("Réglages du studio remis")); }
		return;
	case A_Export:
		if (bDo) { Export(PC); }
		return;
	case A_GoRole:
	{
		const TArray<FName> Roles = FloorRoles(PC);
		if (!Roles.Num()) { return; }
		if (Dir != 0 && !bEnter) { RoleIdx = (RoleIdx + Dir + Roles.Num()) % Roles.Num(); return; }
		RoleIdx = FMath::Clamp(RoleIdx, 0, Roles.Num() - 1);
		PC->ConsoleCommand(TEXT("eon.salle ") + Roles[RoleIdx].ToString());
		Flash(TEXT("Salle ") + Roles[RoleIdx].ToString());
		return;
	}
	case A_Auto:
		if (Dir != 0 || bEnter)
		{
			IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("eon.auto"));
			if (V) { V->Set(V->GetInt() > 0 ? 0 : 1, ECVF_SetByConsole); }
		}
		return;
	default: break;
	}
	if (!bDo) { return; }
	switch (Action)
	{
	case A_Heal: if (J) { J->Heal(99); J->RefillShield(); J->PickupEnergy(J->GetEnergyMax()); Flash(TEXT("Jira soignée")); } break;
	case A_Kill: PC->ConsoleCommand(TEXT("eon.kill")); Flash(TEXT("Ennemis de la salle tués")); break;
	case A_Wave: PC->ConsoleCommand(TEXT("eon.wave")); Flash(TEXT("Vague lancée")); break;
	case A_Adn: PC->ConsoleCommand(TEXT("eon.adn 100")); Flash(TEXT("+100 ADN")); break;
	case A_Scrap: PC->ConsoleCommand(TEXT("eon.scrap 100")); Flash(TEXT("+100 scrap")); break;
	case A_Door: PC->ConsoleCommand(TEXT("eon.go")); Flash(TEXT("Porte passée")); break;
	case A_Floor: PC->ConsoleCommand(TEXT("eon.etage")); Flash(TEXT("Étage suivant")); break;
	case A_Unlock: PC->ConsoleCommand(TEXT("eon.toutdebloquer")); Flash(TEXT("Tout est débloqué")); break;
	case A_Donnees: PC->ConsoleCommand(TEXT("eon.meta plusdonnees 50")); Flash(TEXT("+50 Données")); break;
	default: break;
	}
}

FString FEONDevMenu::ActionValue(const APlayerController* PC, int32 Action) const
{
	switch (Action)
	{
	case A_Preset: return FString::Printf(TEXT("< %s >"), Presets[PresetIdx]);
	case A_GoRole:
	{
		const TArray<FName> Roles = FloorRoles(PC);
		return Roles.Num() ? FString::Printf(TEXT("< %s >"), *Roles[FMath::Clamp(RoleIdx, 0, Roles.Num() - 1)].ToString()) : TEXT("-");
	}
	case A_Auto:
	{
		const IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("eon.auto"));
		return V && V->GetInt() > 0 ? TEXT("OUI") : TEXT("non");
	}
	default: return TEXT("Entrée");
	}
}

void FEONDevMenu::Export(APlayerController* PC)
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(PC);
	if (!T) { return; }
	// Une ligne par réglage : clé = valeur, et la valeur du studio quand elle diffère.
	TArray<FString> Out;
	Out.Add(FString::Printf(TEXT("# EON - réglages exportés le %s (menu Tweak, F6)"), *FDateTime::Now().ToString(TEXT("%d/%m/%Y %H:%M"))));
	Out.Add(TEXT("# clé = valeur   [studio : valeur d'origine si différente]"));
	int32 NChanged = 0;
	for (const FString& Line : T->DescribeAll())
	{
		FString K, V;
		if (!Line.Split(TEXT(" = "), &K, &V)) { continue; }
		K.TrimStartAndEndInline();
		const FString Ref = T->GetPresetValueAsString(TEXT("studio"), K);
		const bool bDiff = !Ref.IsEmpty() && Ref != V;
		NChanged += bDiff ? 1 : 0;
		Out.Add(bDiff ? FString::Printf(TEXT("%s = %s   [studio : %s]"), *K, *V, *Ref) : FString::Printf(TEXT("%s = %s"), *K, *V));
	}
	const FString Path = FPaths::ProjectSavedDir() / TEXT("EON_Reglages.txt");
	const bool bOk = FFileHelper::SaveStringArrayToFile(Out, *Path, FFileHelper::EEncodingOptions::ForceUTF8);
	for (const FString& L : Out) { UE_LOG(LogEON, Display, TEXT("EON_REGLAGE %s"), *L); }
	Flash(bOk ? FString::Printf(TEXT("%d réglage(s) modifié(s) -> Saved/EON_Reglages.txt"), NChanged) : TEXT("Export impossible"));
}

void FEONDevMenu::Build(const APlayerController* PC, FString& Title, TArray<FEONMenuLine>& Lines, FString& Footer) const
{
	Lines.Reset();
	if (!IsOpen()) { return; }
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(PC);
	const int32 S = Sel[(int32)Open];
	Title = Open == EEONDevMenu::Tweak ? TEXT("TWEAK - calibrage (F6)") : TEXT("TRICHE (F7)");

	// Fenêtre de 24 lignes autour de la sélection.
	const int32 NVis = 24;
	const int32 First = FMath::Clamp(S - NVis / 2, 0, FMath::Max(0, Items.Num() - NVis));
	for (int32 I = First; I < FMath::Min(Items.Num(), First + NVis); ++I)
	{
		const FItem& It = Items[I];
		FEONMenuLine L;
		L.Label = It.Label;
		L.bSection = It.bSection;
		L.bSelected = I == S;
		if (It.Action >= 0) { L.Value = ActionValue(PC, It.Action); L.bOn = L.Value == TEXT("OUI"); }
		else if (!It.Key.IsEmpty() && T)
		{
			const FString Cur = T->GetValueAsString(It.Key);
			const FString Ref = T->GetPresetValueAsString(TEXT("studio"), It.Key);
			if (It.Step <= 0.f)
			{
				L.bOn = ParseBool(Cur);
				L.Value = L.bOn ? TEXT("OUI") : TEXT("non");
				L.bChanged = Open == EEONDevMenu::Tweak && !Ref.IsEmpty() && ParseBool(Ref) != L.bOn;
				if (L.bChanged) { L.Hint = ParseBool(Ref) ? TEXT("oui") : TEXT("non"); }
			}
			else
			{
				const float V = FCString::Atof(*Cur), R = Ref.IsEmpty() ? V : FCString::Atof(*Ref);
				L.Value = Fmt(V, It.Step);
				L.bChanged = !FMath::IsNearlyEqual(V, R, It.Step * 0.25f);
				if (L.bChanged) { L.Hint = Fmt(R, It.Step); }
			}
		}
		Lines.Add(L);
	}
	Footer = MessageT > 0.f ? Message
		: Open == EEONDevMenu::Tweak
			? TEXT("Flèches haut/bas : choisir   gauche/droite : changer\nMaj x10, Ctrl /5   Entrée : action   ZQSD : bouger   Échap : fermer")
			: TEXT("Flèches haut/bas : choisir   gauche/droite ou Entrée : activer\nMaj : rubrique suivante   ZQSD : bouger   Échap : fermer");
}
