#include "EONOptions.h"
#include "EON.h"
#include "EONTypes.h"
#include "EONPlayerController.h"
#include "EONTuningSubsystem.h"
#include "AudioDevice.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/Package.h"
#include "RHIGlobals.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* Sec = TEXT("/Script/EON.EONOptions");

	// ---------------------------------------------------------------- valeurs d'EON

	struct FState
	{
		// Affichage
		int32 BgFps = 0;          // limite en arrière-plan (0 = aucune)
		int32 RenderPct = 100;    // résolution de rendu (TSR, TAA, FXAA, aucun) ; 100 = celle du jeu empaqueté (ResolutionQuality)
		int32 Bright = 0;         // luminosité -5..+5 (gamma d'affichage 2,2 ± 0,1 par cran)
		int32 CamDist = 1;        // 0 proche, 1 normale, 2 lointaine, 3 très lointaine
		int32 HdrNits = 1000;
		bool bFps = false;
		// Graphismes
		int32 Upscaler = 0;       // EUp
		int32 UpQuality = 2;      // 0 performance max .. 3 qualité
		int32 Sharpen = 0;        // %
		bool bRR = false;         // DLSS Ray Reconstruction
		bool bScala = false;      // le joueur a touché aux groupes de qualité
		// Éclairage
		int32 Gi = 0, Refl = 0;   // 0 Lumen, 1 écran, 2 aucun
		int32 Hwrt = 1;           // ray tracing matériel (projet : oui)
		int32 RayLight = 0;       // 0 qualité (projet), 1 rapide
		bool bAO = true;
		// Détails
		int32 Aniso = 0;          // 0 = celui de la qualité des textures
		bool bBloom = true, bVignette = true, bGrain = true, bChroma = true;
		// Audio
		int32 VolumeGeneral = 100, Effects = 100;
		bool bSpatial = true;
		bool bBgSound = false;    // moteur : son coupé quand le jeu n'a pas le focus (BaseEngine.ini)
		// Commandes
		int32 Device = 0, PadStyle = 0, Assist = 2, DeadZone = 0;
		bool bRunToggle = false, bStickFire = true;
		TMap<FString, FString> Binds;   // « Holo.Kb » -> « G »
		// Jouabilité
		int32 Shake = 100, HudOpacity = 100, HudScale = 100;
		bool bHitstop = true, bDmgNums = true, bMapRotate = false;
		// Accessibilité
		int32 Cvd = 0, CvdPct = 100, TextSize = 0, TextSpeed = 1;
		bool bContrast = false, bNoFlash = false, bHoldToggle = false, bEasy = false;
	};

#define EON_OPT_INTS(X) X(BgFps) X(RenderPct) X(Bright) X(CamDist) X(HdrNits) X(Upscaler) X(UpQuality) X(Sharpen) X(Gi) X(Refl) \
	X(Hwrt) X(RayLight) X(Aniso) X(VolumeGeneral) X(Effects) X(Device) X(PadStyle) X(Assist) X(DeadZone) X(Shake) X(HudOpacity) \
	X(HudScale) X(Cvd) X(CvdPct) X(TextSize) X(TextSpeed)
#define EON_OPT_BOOLS(X) X(bFps) X(bRR) X(bScala) X(bAO) X(bBloom) X(bVignette) X(bGrain) X(bChroma) X(bSpatial) X(bBgSound) \
	X(bRunToggle) X(bStickFire) X(bHitstop) X(bDmgNums) X(bMapRotate) X(bContrast) X(bNoFlash) X(bHoldToggle) X(bEasy)

	/** Lignes de touches : action d'IMC_EON (et sens pour le déplacement ; aucune action = course, lue hors IMC_EON). */
	struct FBindDef { const TCHAR* Id; const TCHAR* Label; const TCHAR* Action; int32 Dir; bool bKb; bool bPad; };
	const FBindDef BindDefs[] = {
		{ TEXT("Haut"), TEXT("AVANCER"), TEXT("IA_Move"), 0, true, false },
		{ TEXT("Bas"), TEXT("RECULER"), TEXT("IA_Move"), 1, true, false },
		{ TEXT("Gauche"), TEXT("ALLER À GAUCHE"), TEXT("IA_Move"), 2, true, false },
		{ TEXT("Droite"), TEXT("ALLER À DROITE"), TEXT("IA_Move"), 3, true, false },
		{ TEXT("Tir"), TEXT("TIR"), TEXT("IA_Fire"), -1, true, true },
		{ TEXT("Dash"), TEXT("DASH (MAINTENIR : COURSE)"), TEXT("IA_Dash"), -1, true, true },
		{ TEXT("Course"), TEXT("COURSE"), nullptr, -1, true, false },
		{ TEXT("Onde"), TEXT("ONDE"), TEXT("IA_Spell"), -1, true, true },
		{ TEXT("Holo"), TEXT("HOLOGRAMME"), TEXT("IA_Holo"), -1, true, true },
		{ TEXT("Ultime"), TEXT("HYPERCHARGE"), TEXT("IA_Ultimate"), -1, true, true },
		{ TEXT("Interagir"), TEXT("INTERAGIR"), TEXT("IA_Interact"), -1, true, true },
		{ TEXT("Carte"), TEXT("CARTE"), TEXT("IA_Map"), -1, true, true },
		{ TEXT("GreffeSuiv"), TEXT("GREFFE SUIVANTE"), TEXT("IA_GreffeNext"), -1, true, true },
		{ TEXT("GreffePrec"), TEXT("GREFFE PRÉCÉDENTE"), TEXT("IA_GreffePrev"), -1, true, true },
		{ TEXT("Greffe1"), TEXT("GREFFE 1"), TEXT("IA_Greffe1"), -1, true, false },
		{ TEXT("Greffe2"), TEXT("GREFFE 2"), TEXT("IA_Greffe2"), -1, true, false },
		{ TEXT("Greffe3"), TEXT("GREFFE 3"), TEXT("IA_Greffe3"), -1, true, false },
		{ TEXT("Greffe4"), TEXT("GREFFE 4"), TEXT("IA_Greffe4"), -1, true, false },
		{ TEXT("Greffe5"), TEXT("GREFFE 5"), TEXT("IA_Greffe5"), -1, true, false },
	};
	constexpr int32 NumBinds = UE_ARRAY_COUNT(BindDefs);
	/** Propriétaire fictif des touches de pause (conflits seulement). */
	constexpr int32 PauseOwner = NumBinds;

	FString BindKey(int32 B, bool bPad) { return FString(BindDefs[B].Id) + (bPad ? TEXT(".Pad") : TEXT(".Kb")); }
	const TCHAR* OwnerLabel(int32 B) { return B >= 0 && B < NumBinds ? BindDefs[B].Label : TEXT("PAUSE"); }
	int32 RunBind() { for (int32 B = 0; B < NumBinds; ++B) { if (!BindDefs[B].Action) { return B; } } return INDEX_NONE; }

	/** Variable console par son nom, gardée une fois trouvée : le panneau Options relit les mêmes à chaque image.
	 *  Une variable absente (plugin pas encore chargé) n'est pas gardée : elle sera recherchée au prochain appel. */
	IConsoleVariable* CV(const TCHAR* Name)
	{
		static TMap<FString, IConsoleVariable*> Known;
		if (IConsoleVariable* const* Hit = Known.Find(Name)) { return *Hit; }
		IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name);
		if (V) { Known.Add(Name, V); }
		return V;
	}

	FState& St()
	{
		static FState S;
		static bool bLoaded = false;
		if (!bLoaded && GConfig)
		{
			bLoaded = true;
			const FString& Ini = GGameUserSettingsIni;
			FString Probe;
			const bool bHasVol = GConfig->GetString(Sec, TEXT("VolumeGeneral"), Probe, Ini);
#define EON_GET_I(N) GConfig->GetInt(Sec, TEXT(#N), S.N, Ini);
#define EON_GET_B(N) GConfig->GetBool(Sec, TEXT(#N), S.N, Ini);
			EON_OPT_INTS(EON_GET_I)
			EON_OPT_BOOLS(EON_GET_B)
#undef EON_GET_I
#undef EON_GET_B
			// Premier menu Options (c11a6ef) : volume en fraction sous la clé « Volume ».
			float Old = -1.f;
			if (!bHasVol && GConfig->GetFloat(Sec, TEXT("Volume"), Old, Ini) && Old >= 0.f)
			{
				S.VolumeGeneral = FMath::RoundToInt(FMath::Clamp(Old, 0.f, 1.f) * 100.f);
			}
			for (int32 B = 0; B < NumBinds; ++B)
			{
				for (const bool bPad : { false, true })
				{
					FString K;
					if (GConfig->GetString(Sec, *(TEXT("Bind.") + BindKey(B, bPad)), K, Ini) && !K.IsEmpty()) { S.Binds.Add(BindKey(B, bPad), K); }
				}
			}
		}
		return S;
	}

	void Save()
	{
		if (!GConfig) { return; }
		FState& S = St();
		const FString& Ini = GGameUserSettingsIni;
#define EON_SET_I(N) GConfig->SetInt(Sec, TEXT(#N), S.N, Ini);
#define EON_SET_B(N) GConfig->SetBool(Sec, TEXT(#N), S.N, Ini);
		EON_OPT_INTS(EON_SET_I)
		EON_OPT_BOOLS(EON_SET_B)
#undef EON_SET_I
#undef EON_SET_B
		GConfig->RemoveKey(Sec, TEXT("Volume"), Ini);   // clé du premier menu, remplacée par VolumeGeneral
		for (int32 B = 0; B < NumBinds; ++B)
		{
			for (const bool bPad : { false, true })
			{
				const FString Key = TEXT("Bind.") + BindKey(B, bPad);
				if (const FString* K = S.Binds.Find(BindKey(B, bPad))) { GConfig->SetString(Sec, *Key, **K, Ini); }
				else { GConfig->RemoveKey(Sec, *Key, Ini); }
			}
		}
		GConfig->Flush(false, Ini);
	}

	// ---------------------------------------------------------------- moteur

	UGameUserSettings* Gus() { return GEngine ? GEngine->GetGameUserSettings() : nullptr; }
	void SetCV(const TCHAR* Name, int32 V) { if (IConsoleVariable* C = CV(Name)) { C->Set(V, ECVF_SetByCode); } }
	void SetCVf(const TCHAR* Name, float V) { if (IConsoleVariable* C = CV(Name)) { C->Set(V, ECVF_SetByCode); } }
	/** Rend la main au réglage d'en dessous (projet, qualité...) : l'historique des priorités est toujours suivi en 5.8. */
	void UnsetCV(const TCHAR* Name) { if (IConsoleVariable* C = CV(Name)) { C->Unset(ECVF_SetByCode); } }
	int32 GetCV(const TCHAR* Name, int32 Def = 0) { IConsoleVariable* C = CV(Name); return C ? C->GetInt() : Def; }

	UWorld* GameWorld() { return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr; }
	UGameInstance* GameInstance(UWorld* W)
	{
		if (W && W->GetGameInstance()) { return W->GetGameInstance(); }
		return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetGameInstance() : nullptr;
	}

	/** Le banc de game feel relit les choix du joueur (UEONTuningSubsystem::GetGameFeel). */
	void RefreshFeel(UWorld* W)
	{
		UGameInstance* GI = GameInstance(W);
		if (UEONTuningSubsystem* T = GI ? GI->GetSubsystem<UEONTuningSubsystem>() : nullptr) { T->RefreshPlayerOptions(); }
	}

	// ---------------------------------------------------------------- plugin DLSS (fonctions officielles, appelées par réflexion : EON ne dépend d'aucun plugin)

	/** Fonction statique de UDLSSLibrary. Ints remplit dans l'ordre les paramètres entiers, énumérés ou booléens ; l'objet de contexte reçoit W.
	 *  Renvoie false si le plugin (ou la fonction) n'est pas là. */
	bool CallDlss(const TCHAR* Func, UWorld* W = nullptr, std::initializer_list<int64> Ints = {}, bool* OutBool = nullptr)
	{
		UClass* C = FindObject<UClass>(nullptr, TEXT("/Script/DLSSBlueprint.DLSSLibrary"));
		UFunction* F = C ? C->FindFunctionByName(FName(Func)) : nullptr;
		if (!F) { return false; }
		TArray<uint8> Buf;
		Buf.SetNumZeroed(FMath::Max<int32>(FMath::Max<int32>(F->ParmsSize, F->GetStructureSize()), 1));
		F->InitializeStruct(Buf.GetData());
		const int64* NextInt = Ints.begin();
		for (TFieldIterator<FProperty> It(F); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm)) { continue; }
			if (FObjectProperty* O = CastField<FObjectProperty>(*It)) { O->SetObjectPropertyValue_InContainer(Buf.GetData(), W); continue; }
			if (NextInt == Ints.end()) { continue; }
			if (FBoolProperty* Bp = CastField<FBoolProperty>(*It)) { Bp->SetPropertyValue_InContainer(Buf.GetData(), *NextInt++ != 0); }
			else if (FEnumProperty* En = CastField<FEnumProperty>(*It)) { En->GetUnderlyingProperty()->SetIntPropertyValue(En->ContainerPtrToValuePtr<void>(Buf.GetData()), *NextInt++); }
			else if (FNumericProperty* N = CastField<FNumericProperty>(*It)) { N->SetIntPropertyValue(N->ContainerPtrToValuePtr<void>(Buf.GetData()), *NextInt++); }
		}
		C->GetDefaultObject()->ProcessEvent(F, Buf.GetData());
		if (OutBool)
		{
			const FBoolProperty* R = CastField<FBoolProperty>(F->GetReturnProperty());
			*OutBool = R && R->GetPropertyValue_InContainer(Buf.GetData());
		}
		F->DestroyStruct(Buf.GetData());
		return true;
	}
	// UDLSSMode du plugin : Off 0, Auto 1, DLAA 2, UltraQuality 3, Quality 4, Balanced 5, Performance 6, UltraPerformance 7.
	constexpr int64 DlssOff = 0, DlssDlaa = 2;
	const int64 DlssMode[4] = { 7, 6, 5, 4 };

	enum EUp : int32 { UP_TSR, UP_DLSS, UP_DLAA, UP_FSR, UP_XESS, UP_TAA, UP_FXAA, UP_NONE, UP_COUNT };
	const TCHAR* UpNames[UP_COUNT] = { TEXT("TSR"), TEXT("DLSS"), TEXT("DLAA"), TEXT("FSR"), TEXT("XESS"), TEXT("TAA"), TEXT("FXAA"), TEXT("AUCUN") };
	const TCHAR* QualNames[4] = { TEXT("PERFORMANCE MAX"), TEXT("PERFORMANCE"), TEXT("ÉQUILIBRÉE"), TEXT("QUALITÉ") };
	const int32 FsrMode[4] = { 4, 3, 2, 1 };
	const TCHAR* Levels[5] = { TEXT("BASSE"), TEXT("MOYENNE"), TEXT("HAUTE"), TEXT("ÉPIQUE"), TEXT("CINÉMA") };

	bool DlssOk()
	{
		static int32 Ok = -1;   // on ne retient la réponse que si le plugin a répondu
		if (Ok < 0) { bool b = false; if (CallDlss(TEXT("IsDLSSSupported"), nullptr, {}, &b)) { Ok = b ? 1 : 0; } }
		return Ok == 1;
	}
	bool RrOk()
	{
		static int32 Ok = -1;
		if (Ok < 0 && DlssOk()) { bool b = false; if (CallDlss(TEXT("IsDLSSRRSupported"), nullptr, {}, &b)) { Ok = b ? 1 : 0; } }
		return Ok == 1;
	}
	bool FsrOk() { return CV(TEXT("r.FidelityFX.FSR.Enabled")) != nullptr; }
	bool XessOk() { IConsoleVariable* C = CV(TEXT("r.XeSS.Supported")); return C && C->GetBool(); }
	bool UpOk(int32 U)
	{
		switch (U)
		{
		case UP_DLSS: case UP_DLAA: return DlssOk();
		case UP_FSR: return FsrOk();
		case UP_XESS: return XessOk();
		default: return U >= 0 && U < UP_COUNT;
		}
	}
	int32 CurUp() { const int32 U = St().Upscaler; return UpOk(U) ? U : UP_TSR; }
	bool UpHasQuality(int32 U) { return U == UP_DLSS || U == UP_FSR || U == UP_XESS; }
	bool UpUsesRenderPct(int32 U) { return U == UP_TSR || U == UP_TAA || U == UP_FXAA || U == UP_NONE; }

	/** Une seule mise à l'échelle tierce à la fois : on éteint l'ancienne (elle rend la résolution de rendu), puis on allume la nouvelle. */
	void ApplyUpscaler(UWorld* W)
	{
		const FState& S = St();
		const int32 U = CurUp();
		const int32 Q = FMath::Clamp(S.UpQuality, 0, 3);
		const bool bRR = S.bRR && RrOk() && (U == UP_DLSS || U == UP_DLAA);
		static int32 Prev = UP_TSR;   // au démarrage, aucune n'est active (DefaultEngine.ini, r.NGX.DLSS.Enable 0 par défaut)
		if (Prev != U)
		{
			if (Prev == UP_DLSS || Prev == UP_DLAA) { CallDlss(TEXT("EnableDLSSRR"), W, { 0 }); CallDlss(TEXT("SetDLSSMode"), W, { DlssOff }); }
			if (Prev == UP_FSR && FsrOk() && GetCV(TEXT("r.FidelityFX.FSR.Enabled")) != 0) { SetCV(TEXT("r.FidelityFX.FSR.Enabled"), 0); }
			if (Prev == UP_XESS) { SetCV(TEXT("r.XeSS.Enabled"), 0); }
		}
		switch (U)
		{
		case UP_DLSS: case UP_DLAA:
			CallDlss(TEXT("SetDLSSMode"), W, { U == UP_DLAA ? DlssDlaa : DlssMode[Q] });
			CallDlss(TEXT("EnableDLSSRR"), W, { bRR ? 1 : 0 });
			break;
		case UP_FSR:
			SetCV(TEXT("r.FidelityFX.FSR.QualityMode"), FsrMode[Q]);   // le plugin fixe lui-même la résolution de rendu
			if (GetCV(TEXT("r.FidelityFX.FSR.Enabled")) == 0) { SetCV(TEXT("r.FidelityFX.FSR.Enabled"), 1); }
			break;
		case UP_XESS:
			SetCV(TEXT("r.XeSS.Quality"), Q);
			SetCV(TEXT("r.XeSS.Enabled"), 1);
			break;
		default:
			SetCVf(TEXT("r.ScreenPercentage"), (float)FMath::Clamp(S.RenderPct, 50, 100));
			break;
		}
		Prev = U;
		// Anticrénelage : TSR (celui du projet) pour toutes les mises à l'échelle, sinon TAA, FXAA ou aucun.
		if (U == UP_TAA) { SetCV(TEXT("r.AntiAliasingMethod"), 2); }
		else if (U == UP_FXAA) { SetCV(TEXT("r.AntiAliasingMethod"), 1); }
		else if (U == UP_NONE) { SetCV(TEXT("r.AntiAliasingMethod"), 0); }
		else { UnsetCV(TEXT("r.AntiAliasingMethod")); }
		// Netteté : celle du FSR (RCAS) quand il est actif, sinon celle du tonemapper.
		const float Sh = FMath::Clamp(S.Sharpen, 0, 100) / 100.f;
		if (FsrOk())
		{
			if (U == UP_FSR && S.Sharpen > 0) { SetCVf(TEXT("r.FidelityFX.FSR.Sharpness"), Sh); }
			else { UnsetCV(TEXT("r.FidelityFX.FSR.Sharpness")); }
		}
		if (U != UP_FSR && S.Sharpen > 0) { SetCVf(TEXT("r.Tonemapper.Sharpen"), Sh * 2.f); }
		else { UnsetCV(TEXT("r.Tonemapper.Sharpen")); }
		const IConsoleVariable* Sp = CV(TEXT("r.ScreenPercentage"));
		UE_LOG(LogEON, Display, TEXT("EON options : mise a l'echelle %s, qualite %d, r.ScreenPercentage %.0f, AA %d, RR %d"),
			UpNames[U], Q, Sp ? Sp->GetFloat() : 0.f, GetCV(TEXT("r.AntiAliasingMethod")), bRR ? 1 : 0);
	}

	// ---------------------------------------------------------------- groupes de qualité (sg.*)

	struct FGroup { const TCHAR* Cvar; int32 (UGameUserSettings::*Get)() const; void (UGameUserSettings::*Set)(int32); };
	const FGroup Groups[] = {
		{ TEXT("sg.ViewDistanceQuality"), &UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality },
		{ TEXT("sg.AntiAliasingQuality"), &UGameUserSettings::GetAntiAliasingQuality, &UGameUserSettings::SetAntiAliasingQuality },
		{ TEXT("sg.ShadowQuality"), &UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality },
		{ TEXT("sg.GlobalIlluminationQuality"), &UGameUserSettings::GetGlobalIlluminationQuality, &UGameUserSettings::SetGlobalIlluminationQuality },
		{ TEXT("sg.ReflectionQuality"), &UGameUserSettings::GetReflectionQuality, &UGameUserSettings::SetReflectionQuality },
		{ TEXT("sg.PostProcessQuality"), &UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality },
		{ TEXT("sg.TextureQuality"), &UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality },
		{ TEXT("sg.EffectsQuality"), &UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality },
		{ TEXT("sg.FoliageQuality"), &UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality },
		{ TEXT("sg.ShadingQuality"), &UGameUserSettings::GetShadingQuality, &UGameUserSettings::SetShadingQuality },
	};
	enum EGrp : int32 { G_VIEW, G_AA, G_SHADOW, G_GI, G_REFL, G_PP, G_TEX, G_FX, G_FOLIAGE, G_SHADING, G_COUNT };

	int32 GroupLevel(int32 G) { return FMath::Clamp(GetCV(Groups[G].Cvar, 3), 0, 4); }
	/** sg.* posés par code : ils passent devant [SystemSettings] de DefaultEngine.ini (qualité Cinéma du projet). */
	void PushGroups()
	{
		UGameUserSettings* Gs = Gus();
		if (!Gs) { return; }
		for (const FGroup& G : Groups) { SetCV(G.Cvar, FMath::Clamp((Gs->*G.Get)(), 0, 4)); }
	}
	/** Première retouche : on part des niveaux en vigueur (projet compris) pour que rien d'autre ne bouge. */
	void EnsureScala()
	{
		FState& S = St();
		UGameUserSettings* Gs = Gus();
		if (S.bScala || !Gs) { return; }
		for (int32 G = 0; G < G_COUNT; ++G) { (Gs->*Groups[G].Set)(GroupLevel(G)); }
		S.bScala = true;
	}
	void SetGroup(int32 G, int32 Level)
	{
		EnsureScala();
		if (UGameUserSettings* Gs = Gus()) { (Gs->*Groups[G].Set)(FMath::Clamp(Level, 0, 4)); Gs->SaveSettings(); }
		PushGroups();
		Save();
	}
	FString OverallName()
	{
		const int32 L = GroupLevel(G_GI);
		for (int32 G = 0; G < G_COUNT; ++G) { if (G != G_FOLIAGE && GroupLevel(G) != L) { return TEXT("PERSONNALISÉE"); } }
		return Levels[L];
	}

	// ---------------------------------------------------------------- post-process et filtres d'EON (par carte)

	const FName PPTag(TEXT("EON_Options"));

	/** Reflets Lumen seulement avec l'illumination Lumen : sinon, reflets d'écran. */
	int32 EffRefl() { const FState& S = St(); return S.Gi != 0 && S.Refl == 0 ? 1 : S.Refl; }

	APostProcessVolume* OptionsVolume(UWorld* W)
	{
		for (TActorIterator<APostProcessVolume> It(W); It; ++It) { if (It->Tags.Contains(PPTag)) { return *It; } }
		FActorSpawnParameters P;
		P.ObjectFlags |= RF_Transient;
		APostProcessVolume* V = W->SpawnActor<APostProcessVolume>(P);
		if (V)
		{
			V->Tags.Add(PPTag);
			V->bUnbound = true;
			V->Priority = 1000.f;
			V->BlendWeight = 1.f;
		}
		return V;
	}

	/** Le filtre « Film » d'EON (MI_PP_EON_Film, M_PP_EON_Film) dans la chaîne des parents ? */
	bool IsFilm(const UMaterialInterface* M)
	{
		for (int32 Guard = 0; M && Guard < 8; ++Guard)
		{
			if (M->GetName().Contains(TEXT("PP_EON_Film"))) { return true; }
			const UMaterialInstance* Mi = Cast<UMaterialInstance>(M);
			M = Mi ? Mi->Parent.Get() : nullptr;
		}
		return false;
	}

	void ApplyPost(UWorld* W)
	{
		if (!W || !W->IsGameWorld()) { return; }
		const FState& S = St();
		if (APostProcessVolume* V = OptionsVolume(W))
		{
			FPostProcessSettings& P = V->Settings;
			P.bOverride_DynamicGlobalIlluminationMethod = S.Gi != 0;
			P.DynamicGlobalIlluminationMethod = S.Gi == 1 ? EDynamicGlobalIlluminationMethod::ScreenSpace : EDynamicGlobalIlluminationMethod::None;
			const int32 Rf = EffRefl();
			P.bOverride_ReflectionMethod = Rf != 0;
			P.ReflectionMethod = Rf == 1 ? EReflectionMethod::ScreenSpace : EReflectionMethod::None;
			P.bOverride_LumenRayLightingMode = S.RayLight == 1;
			P.LumenRayLightingMode = ELumenRayLightingModeOverride::SurfaceCache;
			P.bOverride_AmbientOcclusionIntensity = !S.bAO;
			P.AmbientOcclusionIntensity = 0.f;
			P.bOverride_BloomIntensity = !S.bBloom;
			P.BloomIntensity = 0.f;
			P.bOverride_VignetteIntensity = !S.bVignette;
			P.VignetteIntensity = 0.f;
		}
		// Filtre « Film » (grain, aberration chromatique, coulure lumineuse) : copie dynamique dans chaque volume de la carte.
		for (TActorIterator<APostProcessVolume> It(W); It; ++It)
		{
			if (It->Tags.Contains(PPTag)) { continue; }
			for (FWeightedBlendable& B : It->Settings.WeightedBlendables.Array)
			{
				UMaterialInterface* M = Cast<UMaterialInterface>(B.Object);
				if (!M || !IsFilm(M)) { continue; }
				UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(M);
				if (!D) { D = UMaterialInstanceDynamic::Create(M, *It); B.Object = D; }
				const UMaterialInterface* Src = D->Parent ? D->Parent.Get() : D;
				float Gr = 0.f, Ch = 0.f, Bl = 0.f;
				Src->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("GrainStrength")), Gr);
				Src->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("ChromaPx")), Ch);
				Src->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("BleedStrength")), Bl);
				D->SetScalarParameterValue(TEXT("GrainStrength"), S.bGrain ? Gr : 0.f);
				D->SetScalarParameterValue(TEXT("ChromaPx"), S.bChroma ? Ch : 0.f);
				D->SetScalarParameterValue(TEXT("BleedStrength"), S.bBloom ? Bl : 0.f);
			}
		}
	}

	bool HwrtPossible() { return GRHISupportsRayTracing && GetCV(TEXT("r.RayTracing")) > 0; }

	void ApplyLighting()
	{
		const FState& S = St();
		if (HwrtPossible())
		{
			if (S.Hwrt) { UnsetCV(TEXT("r.Lumen.HardwareRayTracing")); }
			else { SetCV(TEXT("r.Lumen.HardwareRayTracing"), 0); }
		}
		if (S.bAO) { UnsetCV(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO")); }
		else { SetCV(TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO"), 0); }
		if (S.Aniso > 0) { SetCV(TEXT("r.MaxAnisotropy"), S.Aniso); }
		else { UnsetCV(TEXT("r.MaxAnisotropy")); }
	}

	void ApplyAudio(UWorld* W)
	{
		const FState& S = St();
		if (W)
		{
			if (FAudioDeviceHandle D = W->GetAudioDevice()) { D->SetTransientPrimaryVolume(FMath::Clamp(S.VolumeGeneral, 0, 100) / 100.f); }
		}
		const float Bg = S.bBgSound ? 1.f : 0.f;
		if (!FMath::IsNearlyEqual(FApp::GetUnfocusedVolumeMultiplier(), Bg)) { FApp::SetUnfocusedVolumeMultiplier(Bg); }
	}

	void ApplyColor()
	{
		const FState& S = St();
		static const EColorVisionDeficiency Types[4] = { EColorVisionDeficiency::NormalVision, EColorVisionDeficiency::Protanope,
			EColorVisionDeficiency::Deuteranope, EColorVisionDeficiency::Tritanope };
		UWidgetBlueprintLibrary::SetColorVisionDeficiencyType(Types[FMath::Clamp(S.Cvd, 0, 3)], FMath::Clamp(S.CvdPct, 0, 100) / 100.f, true, false);
		if (GEngine) { GEngine->DisplayGamma = 2.2f + FMath::Clamp(S.Bright, -5, 5) * 0.1f; }
	}

	/** Limite d'images quand une autre fenêtre a le focus ; on rend la limite d'avant dès le retour (ou hors du jeu). */
	bool TickBackground(float)
	{
		static float Applied = -1.f, Before = 0.f;
		if (!GEngine) { return true; }
		const int32 Bg = GameWorld() ? St().BgFps : 0;
		const bool bLimit = Bg > 0 && !FApp::HasFocus();
		if (bLimit && !FMath::IsNearlyEqual(Applied, (float)Bg))
		{
			if (Applied < 0.f) { Before = GEngine->GetMaxFPS(); }
			GEngine->SetMaxFPS((float)Bg);
			Applied = (float)Bg;
		}
		else if (!bLimit && Applied >= 0.f)
		{
			GEngine->SetMaxFPS(Before);
			Applied = -1.f;
		}
		return true;
	}

	// ---------------------------------------------------------------- touches

	bool HasMod(const FEnhancedActionKeyMapping& M, UClass* C) { return M.Modifiers.ContainsByPredicate([C](const UInputModifier* X) { return X && X->IsA(C); }); }
	/** Sens d'une touche de déplacement d'après ses modificateurs : 0 haut, 1 bas, 2 gauche, 3 droite. */
	int32 MoveDir(const FEnhancedActionKeyMapping& M)
	{
		const bool bSwz = HasMod(M, UInputModifierSwizzleAxis::StaticClass()), bNeg = HasMod(M, UInputModifierNegate::StaticClass());
		return bSwz ? (bNeg ? 1 : 0) : (bNeg ? 2 : 3);
	}
	bool IsArrow(const FKey& K) { return K == EKeys::Up || K == EKeys::Down || K == EKeys::Left || K == EKeys::Right; }
	bool IsAction(const FEnhancedActionKeyMapping& M, const TCHAR* Name) { return Name && M.Action && M.Action->GetName() == Name; }

	/** Touches « principales » d'une ligne : celles qu'une réassignation remplace (flèches, Entrée, clic droit... restent). */
	TArray<int32> Primary(const TArray<FEnhancedActionKeyMapping>& Ms, int32 B, bool bPad)
	{
		TArray<int32> R;
		const FBindDef& D = BindDefs[B];
		if (!D.Action) { return R; }
		for (int32 I = 0; I < Ms.Num(); ++I)
		{
			const FEnhancedActionKeyMapping& M = Ms[I];
			if (!IsAction(M, D.Action) || M.Key.IsGamepadKey() != bPad) { continue; }
			if (D.Dir >= 0) { if (!bPad && !IsArrow(M.Key) && MoveDir(M) == D.Dir) { R.Add(I); } }   // Z et W ensemble (AZERTY, QWERTY)
			else { R.Add(I); break; }
		}
		return R;
	}

	/** Ligne de la liste à laquelle appartient une touche du contexte (PauseOwner pour la pause, -1 si aucune : sticks...). */
	int32 OwnerOf(const FEnhancedActionKeyMapping& M)
	{
		if (IsAction(M, TEXT("IA_Pause"))) { return PauseOwner; }
		for (int32 B = 0; B < NumBinds; ++B)
		{
			const FBindDef& D = BindDefs[B];
			if (!IsAction(M, D.Action)) { continue; }
			if (D.Dir >= 0 && (M.Key.IsGamepadKey() || MoveDir(M) != D.Dir)) { continue; }
			return B;
		}
		return -1;
	}

	AEONPlayerController* Controller(UWorld* W) { return W ? Cast<AEONPlayerController>(W->GetFirstPlayerController()) : nullptr; }
	const UInputMappingContext* ActiveContext(UWorld* W) { const AEONPlayerController* PC = Controller(W); return PC ? PC->GetActiveContext() : nullptr; }
	void RebuildInput(UWorld* W) { if (AEONPlayerController* PC = Controller(W ? W : GameWorld())) { PC->RebuildInputContext(); } }

	FKey StoredKey(int32 B, bool bPad)
	{
		if (B < 0 || B >= NumBinds) { return FKey(); }
		const FString* K = St().Binds.Find(BindKey(B, bPad));
		return K ? FKey(FName(**K)) : FKey();
	}

	/** Touches d'une ligne dans le contexte actif (la touche choisie par le joueur, sinon celles d'origine, Z / Q d'abord). */
	TArray<FKey> BindKeys(const UInputMappingContext* Ctx, int32 B, bool bPad)
	{
		TArray<FKey> R;
		const FKey Stored = StoredKey(B, bPad);
		if (Stored.IsValid()) { R.Add(Stored); return R; }
		if (!BindDefs[B].Action) { if (!bPad) { R.Add(EKeys::LeftShift); } return R; }
		if (!Ctx) { return R; }
		for (const int32 I : Primary(Ctx->GetMappings(), B, bPad)) { R.AddUnique(Ctx->GetMappings()[I].Key); }
		R.StableSort([](const FKey& X, const FKey& Y) { auto Rk = [](const FKey& K) { return K == EKeys::Z || K == EKeys::Q ? 0 : 1; }; return Rk(X) < Rk(Y); });
		return R;
	}

	// ---------------------------------------------------------------- lignes des onglets

	struct FDef
	{
		FString Label, Help;
		EONOptions::EKind Kind = EONOptions::EKind::Choice;
		TFunction<FString()> Val;
		TFunction<void(UWorld*, int32)> Chg;
		TFunction<void(UWorld*, float)> Frac;
		TFunction<float()> Fill;
		TFunction<bool()> On;
		TFunction<bool()> Enabled;
		TFunction<FString()> Why;
		TFunction<FString()> HelpFn;
		TFunction<bool()> Visible;
		TFunction<void(UWorld*)> Act;
		int32 Bind = -1;
		int32 Preview = 0;
	};

	int32 Cycle(int32 V, int32 Dir, int32 N) { return N > 0 ? ((V + Dir) % N + N) % N : 0; }
	FString Pct(int32 V) { return FString::Printf(TEXT("%d %%"), V); }
	FString YesNo(bool b) { return b ? TEXT("OUI") : TEXT("NON"); }

	FDef Toggle(const TCHAR* L, const TCHAR* H, bool FState::* M, TFunction<void(UWorld*)> Then = nullptr)
	{
		FDef D; D.Label = L; D.Help = H; D.Kind = EONOptions::EKind::Toggle;
		D.On = [M]() { return St().*M; };
		D.Val = [M]() { return YesNo(St().*M); };
		D.Chg = [M, Then](UWorld* W, int32) { St().*M = !(St().*M); Save(); if (Then) { Then(W); } };
		return D;
	}
	/** Deux choix nommés sur un booléen (ex. COURSE : MAINTENIR / BASCULER). */
	FDef Flag2(const TCHAR* L, const TCHAR* H, bool FState::* M, const TCHAR* OffName, const TCHAR* OnName, TFunction<void(UWorld*)> Then = nullptr)
	{
		FDef D; D.Label = L; D.Help = H;
		const FString OffS = OffName, OnS = OnName;
		D.Val = [M, OffS, OnS]() { return St().*M ? OnS : OffS; };
		D.Chg = [M, Then](UWorld* W, int32) { St().*M = !(St().*M); Save(); if (Then) { Then(W); } };
		return D;
	}
	FDef Slider(const TCHAR* L, const TCHAR* H, int32 FState::* M, int32 Lo, int32 Hi, int32 Step, TFunction<void(UWorld*)> Then = nullptr, TFunction<FString(int32)> Fmt = nullptr)
	{
		FDef D; D.Label = L; D.Help = H; D.Kind = EONOptions::EKind::Slider;
		D.Fill = [M, Lo, Hi]() { return float(St().*M - Lo) / float(FMath::Max(1, Hi - Lo)); };
		D.Val = [M, Fmt]() { return Fmt ? Fmt(St().*M) : Pct(St().*M); };
		D.Chg = [M, Lo, Hi, Step, Then](UWorld* W, int32 Dir) { St().*M = FMath::Clamp(St().*M + Dir * Step, Lo, Hi); Save(); if (Then) { Then(W); } };
		D.Frac = [M, Lo, Hi, Step, Then](UWorld* W, float T)
		{
			const int32 V = FMath::Clamp(Lo + FMath::RoundToInt(T * (Hi - Lo) / (float)Step) * Step, Lo, Hi);
			if (V == St().*M) { return; }
			St().*M = V; Save(); if (Then) { Then(W); }
		};
		return D;
	}
	FDef Choice(const TCHAR* L, const TCHAR* H, int32 FState::* M, TArray<FString> Names, TFunction<void(UWorld*)> Then = nullptr)
	{
		FDef D; D.Label = L; D.Help = H;
		D.Val = [M, Names]() { return Names.IsValidIndex(St().*M) ? Names[St().*M] : FString(TEXT("?")); };
		D.Chg = [M, Num = Names.Num(), Then](UWorld* W, int32 Dir) { St().*M = Cycle(St().*M, Dir, Num); Save(); if (Then) { Then(W); } };
		return D;
	}
	FDef Group(const TCHAR* L, const TCHAR* H, int32 Grp)
	{
		FDef D; D.Label = L; D.Help = H;
		D.Val = [Grp]() { return FString(Levels[GroupLevel(Grp)]); };
		D.Chg = [Grp](UWorld*, int32 Dir) { SetGroup(Grp, GroupLevel(Grp) + Dir); };
		return D;
	}

	TArray<FIntPoint> Resolutions()
	{
		TArray<FIntPoint> R;
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(R);
		R.RemoveAll([](const FIntPoint& P) { return P.X < 1280 || P.Y < 720; });
		if (R.IsEmpty()) { R = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 } }; }
		R.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });
		TArray<FIntPoint> U;
		for (const FIntPoint& P : R) { U.AddUnique(P); }
		return U;
	}

	const int32 FpsSteps[] = { 30, 60, 120, 144, 165, 240, 0 };
	const int32 BgSteps[] = { 0, 60, 30, 15 };
	const int32 NitsSteps[] = { 400, 600, 1000, 1400, 2000 };
	const int32 AnisoSteps[] = { 1, 2, 4, 8, 16 };

	int32 IndexIn(const int32* Steps, int32 N, int32 V, int32 Def) { for (int32 K = 0; K < N; ++K) { if (Steps[K] == V) { return K; } } return Def; }

	TArray<TArray<FDef>>& Defs()
	{
		static TArray<TArray<FDef>> T;
		if (!T.IsEmpty()) { return T; }
		T.SetNum(EONOptions::NumTabs);
		using EONOptions::EKind;
		auto ApplyGus = []() { if (UGameUserSettings* Gs = Gus()) { Gs->ApplySettings(false); } };

		// ================================================================ 0 AFFICHAGE
		TArray<FDef>& TabA = T[0];
		{
			FDef D; D.Label = TEXT("MODE D'AFFICHAGE");
			D.Help = TEXT("Plein écran : l'image la plus réactive.\nPlein écran fenêtré : passer d'une fenêtre à l'autre sans coupure.\nFenêtré : le jeu dans une fenêtre déplaçable.");
			D.Val = []() { const UGameUserSettings* Gs = Gus(); const EWindowMode::Type Wm = Gs ? Gs->GetFullscreenMode() : EWindowMode::Windowed;
				return FString(Wm == EWindowMode::Fullscreen ? TEXT("PLEIN ÉCRAN") : Wm == EWindowMode::WindowedFullscreen ? TEXT("PLEIN ÉCRAN FENÊTRÉ") : TEXT("FENÊTRÉ")); };
			D.Chg = [ApplyGus](UWorld*, int32 Dir) { if (UGameUserSettings* Gs = Gus()) {
				static const EWindowMode::Type Modes[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
				const int32 Cur = Gs->GetFullscreenMode() == EWindowMode::Fullscreen ? 0 : Gs->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? 1 : 2;
				Gs->SetFullscreenMode(Modes[Cycle(Cur, Dir, 3)]); ApplyGus(); } };
			TabA.Add(D);
		}
		{
			FDef D; D.Label = TEXT("RÉSOLUTION");
			D.Help = TEXT("Taille de l'image envoyée à l'écran.");
			D.Val = []() { const UGameUserSettings* Gs = Gus(); const bool bDesk = Gs && Gs->GetFullscreenMode() == EWindowMode::WindowedFullscreen;
				const FIntPoint Res = Gs ? (bDesk ? Gs->GetDesktopResolution() : Gs->GetScreenResolution()) : FIntPoint(1920, 1080);
				return FString::Printf(TEXT("%d × %d"), Res.X, Res.Y); };
			D.Enabled = []() { const UGameUserSettings* Gs = Gus(); return !Gs || Gs->GetFullscreenMode() != EWindowMode::WindowedFullscreen; };
			D.Why = []() { return FString(TEXT("En plein écran fenêtré, le jeu prend la résolution du bureau : passe en plein écran ou en fenêtré pour la choisir.")); };
			D.Chg = [ApplyGus](UWorld*, int32 Dir) { if (UGameUserSettings* Gs = Gus()) {
				const TArray<FIntPoint> Rs = Resolutions(); int32 Cur = Rs.IndexOfByKey(Gs->GetScreenResolution());
				Cur = Cur == INDEX_NONE ? Rs.Num() - 1 : Cycle(Cur, Dir, Rs.Num()); Gs->SetScreenResolution(Rs[Cur]); ApplyGus(); } };
			TabA.Add(D);
		}
		{
			FDef D; D.Label = TEXT("V-SYNC"); D.Kind = EKind::Toggle;
			D.Help = TEXT("Cale les images sur le rafraîchissement de l'écran : plus de déchirure horizontale, un peu plus de latence.");
			D.On = []() { const UGameUserSettings* Gs = Gus(); return Gs && Gs->IsVSyncEnabled(); };
			D.Val = [IsOn = D.On]() { return YesNo(IsOn()); };
			D.Chg = [ApplyGus](UWorld*, int32) { if (UGameUserSettings* Gs = Gus()) { Gs->SetVSyncEnabled(!Gs->IsVSyncEnabled()); ApplyGus(); } };
			TabA.Add(D);
		}
		{
			FDef D; D.Label = TEXT("LIMITE D'IMAGES");
			D.Help = TEXT("Nombre maximal d'images par seconde. Une limite proche du rafraîchissement de l'écran rend le jeu plus régulier et la carte graphique plus silencieuse.");
			D.Val = []() { const UGameUserSettings* Gs = Gus(); const int32 Fps = Gs ? FMath::RoundToInt(Gs->GetFrameRateLimit()) : 0;
				return Fps <= 0 ? FString(TEXT("ILLIMITÉE")) : FString::Printf(TEXT("%d / S"), Fps); };
			D.Chg = [ApplyGus](UWorld*, int32 Dir) { if (UGameUserSettings* Gs = Gus()) {
				const int32 Num = UE_ARRAY_COUNT(FpsSteps);
				const int32 Cur = IndexIn(FpsSteps, Num, FMath::RoundToInt(Gs->GetFrameRateLimit()), Num - 1);
				Gs->SetFrameRateLimit((float)FpsSteps[Cycle(Cur, Dir, Num)]); ApplyGus(); } };
			TabA.Add(D);
		}
		{
			FDef D; D.Label = TEXT("IMAGES EN ARRIÈRE-PLAN");
			D.Help = TEXT("Limite quand une autre fenêtre est devant le jeu : la carte graphique se repose pendant que tu fais autre chose.");
			D.Val = []() { const int32 Bg = St().BgFps; return Bg <= 0 ? FString(TEXT("SANS LIMITE")) : FString::Printf(TEXT("%d / S"), Bg); };
			D.Chg = [](UWorld*, int32 Dir) { const int32 Num = UE_ARRAY_COUNT(BgSteps);
				St().BgFps = BgSteps[Cycle(IndexIn(BgSteps, Num, St().BgFps, 0), Dir, Num)]; Save(); };
			TabA.Add(D);
		}
		{
			FDef D = Slider(TEXT("RÉSOLUTION DE RENDU"), TEXT("Part de la résolution calculée avant l'agrandissement par le TSR. Plus bas : plus d'images par seconde, image plus douce."),
				&FState::RenderPct, 50, 100, 5, [](UWorld* W) { ApplyUpscaler(W); });
			D.Enabled = []() { return UpUsesRenderPct(CurUp()); };
			D.Why = []() { return FString::Printf(TEXT("Avec %s, la résolution de rendu suit la qualité choisie dans l'onglet Graphismes."), UpNames[CurUp()]); };
			TabA.Add(D);
		}
		{
			FDef D = Slider(TEXT("LUMINOSITÉ"), TEXT("Règle jusqu'à ce que le carré de gauche de la mire soit à peine visible et que le troisième se détache nettement."),
				&FState::Bright, -5, 5, 1, [](UWorld*) { ApplyColor(); }, [](int32 V) { return V == 0 ? FString(TEXT("0")) : FString::Printf(TEXT("%+d"), V); });
			D.Preview = 1;
			TabA.Add(D);
		}
		{
			FDef D; D.Label = TEXT("HDR"); D.Kind = EKind::Toggle;
			D.Help = TEXT("Image à grande plage dynamique, pour les écrans HDR : lumières plus éclatantes, noirs plus profonds.");
			D.On = []() { const UGameUserSettings* Gs = Gus(); return Gs && Gs->IsHDREnabled(); };
			D.Val = [IsOn = D.On]() { return YesNo(IsOn()); };
			D.Enabled = []() { const UGameUserSettings* Gs = Gus(); return Gs && Gs->SupportsHDRDisplayOutput(); };
			D.Why = []() { return FString(TEXT("Cet écran (ou Windows) n'annonce pas le HDR : active-le dans les paramètres d'affichage de Windows pour pouvoir le choisir.")); };
			D.Chg = [](UWorld*, int32) { if (UGameUserSettings* Gs = Gus()) { Gs->EnableHDRDisplayOutput(!Gs->IsHDREnabled(), St().HdrNits); Gs->SaveSettings(); } };
			TabA.Add(D);
		}
		{
			FDef D; D.Label = TEXT("LUMINANCE HDR");
			D.Help = TEXT("Luminosité maximale de ton écran HDR, en nits (voir sa fiche technique).");
			D.Val = []() { return FString::Printf(TEXT("%d NITS"), St().HdrNits); };
			D.Visible = []() { const UGameUserSettings* Gs = Gus(); return Gs && Gs->IsHDREnabled(); };
			D.Chg = [](UWorld*, int32 Dir) { const int32 Num = UE_ARRAY_COUNT(NitsSteps);
				St().HdrNits = NitsSteps[FMath::Clamp(IndexIn(NitsSteps, Num, St().HdrNits, 2) + Dir, 0, Num - 1)]; Save();
				if (UGameUserSettings* Gs = Gus()) { Gs->EnableHDRDisplayOutput(true, St().HdrNits); Gs->SaveSettings(); } };
			TabA.Add(D);
		}
		TabA.Add(Choice(TEXT("DISTANCE DE LA CAMÉRA"), TEXT("Hauteur de la caméra au-dessus de Jira : plus loin, on voit plus de salle ; plus près, plus de détails. La molette zoome toujours en plus."),
			&FState::CamDist, { TEXT("PROCHE"), TEXT("NORMALE"), TEXT("LOINTAINE"), TEXT("TRÈS LOINTAINE") }, [](UWorld* W) { RefreshFeel(W); }));
		TabA.Add(Toggle(TEXT("COMPTEUR D'IMAGES"), TEXT("Affiche les images par seconde en haut à droite, sous la mini-carte."), &FState::bFps));

		// ================================================================ 1 GRAPHISMES
		TArray<FDef>& TabG = T[1];
		{
			FDef D; D.Label = TEXT("PRÉRÉGLAGE");
			D.Help = TEXT("Règle d'un coup tous les groupes de qualité. Changer un réglage plus bas le fait passer en « personnalisée ».");
			D.Val = []() { return OverallName(); };
			D.Chg = [](UWorld*, int32 Dir) {
				UGameUserSettings* Gs = Gus(); if (!Gs) { return; }
				EnsureScala();
				float Cn = 0.f, Cv = 100.f, Mn = 0.f, Mx = 0.f; Gs->GetResolutionScaleInformationEx(Cn, Cv, Mn, Mx);
				Gs->SetOverallScalabilityLevel(FMath::Clamp(GroupLevel(G_GI) + Dir, 0, 4));
				Gs->SetResolutionScaleValueEx(Cv);   // la résolution de rendu reste celle du joueur
				Gs->SaveSettings(); PushGroups(); Save(); };
			TabG.Add(D);
		}
		{
			FDef D; D.Label = TEXT("DÉTECTION AUTOMATIQUE"); D.Kind = EKind::Action;
			D.Help = TEXT("Unreal mesure le processeur et la carte graphique pendant quelques secondes, puis choisit les groupes de qualité.");
			D.Val = []() { return FString(TEXT("LANCER")); };
			D.Act = [](UWorld*) {
				UGameUserSettings* Gs = Gus(); if (!Gs) { return; }
				EnsureScala();
				float Cn = 0.f, Cv = 100.f, Mn = 0.f, Mx = 0.f; Gs->GetResolutionScaleInformationEx(Cn, Cv, Mn, Mx);
				Gs->RunHardwareBenchmark(); Gs->ApplyHardwareBenchmarkResults();
				Gs->SetResolutionScaleValueEx(Cv); Gs->SaveSettings(); PushGroups(); Save();
				UE_LOG(LogEON, Display, TEXT("EON options : detection automatique -> %s (CPU %.0f, GPU %.0f)"), *OverallName(), Gs->GetLastCPUBenchmarkResult(), Gs->GetLastGPUBenchmarkResult()); };
			TabG.Add(D);
		}
		{
			FDef D; D.Label = TEXT("MISE À L'ÉCHELLE");
			D.HelpFn = []() {
				auto Dispo = [](bool b) { return b ? TEXT("disponible") : TEXT("indisponible sur cette carte"); };
				return FString::Printf(TEXT("Calcule l'image plus petite, puis l'agrandit proprement : plus d'images par seconde.\n")
					TEXT("TSR : celle d'Unreal, sur toutes les cartes.\nDLSS et DLAA (NVIDIA RTX) : %s. DLAA ne réduit pas l'image, il l'affine.\n")
					TEXT("FSR (AMD, toutes les cartes) : %s.\nXeSS (Intel, cartes récentes) : %s.\nTAA, FXAA : anticrénelage simple, sans agrandissement.\nCarte graphique : %s"),
					Dispo(DlssOk()), Dispo(FsrOk()), Dispo(XessOk()), *GRHIAdapterName); };
			D.Val = []() { return FString(UpNames[CurUp()]); };
			D.Chg = [](UWorld* W, int32 Dir) {
				int32 U = CurUp();
				for (int32 K = 0; K < UP_COUNT; ++K) { U = Cycle(U, Dir, UP_COUNT); if (UpOk(U)) { break; } }
				St().Upscaler = U; Save(); ApplyUpscaler(W); };
			TabG.Add(D);
		}
		{
			FDef D; D.Label = TEXT("QUALITÉ DE LA MISE À L'ÉCHELLE");
			D.Help = TEXT("Performance max : l'image est calculée au tiers de sa taille, le plus fluide. Qualité : aux deux tiers, le plus fin.");
			D.Val = []() { return FString(QualNames[FMath::Clamp(St().UpQuality, 0, 3)]); };
			D.Enabled = []() { return UpHasQuality(CurUp()); };
			D.Why = []() { return FString::Printf(TEXT("%s n'a pas de mode de qualité : choisis DLSS, FSR ou XeSS dans la ligne au-dessus."), UpNames[CurUp()]); };
			D.Chg = [](UWorld* W, int32 Dir) { St().UpQuality = FMath::Clamp(St().UpQuality + Dir, 0, 3); Save(); ApplyUpscaler(W); };
			TabG.Add(D);
		}
		{
			FDef D = Toggle(TEXT("DLSS RAY RECONSTRUCTION"), TEXT("Le réseau de NVIDIA remplace le débruitage de Lumen : reflets et lumière indirecte plus nets, un peu plus coûteux."),
				&FState::bRR, [](UWorld* W) { ApplyUpscaler(W); });
			D.Visible = []() { return RrOk(); };
			D.Enabled = []() { return CurUp() == UP_DLSS || CurUp() == UP_DLAA; };
			D.Why = []() { return FString(TEXT("Seulement avec DLSS ou DLAA.")); };
			TabG.Add(D);
		}
		TabG.Add(Slider(TEXT("NETTETÉ"), TEXT("Accentue les contours après la mise à l'échelle (filtre RCAS avec FSR)."), &FState::Sharpen, 0, 100, 10, [](UWorld* W) { ApplyUpscaler(W); }));
		TabG.Add(Group(TEXT("QUALITÉ DE L'ANTICRÉNELAGE"), TEXT("Finesse du TSR et du TAA : moins de scintillement sur les arêtes fines."), G_AA));

		// ================================================================ 2 ÉCLAIRAGE
		TArray<FDef>& TabE = T[2];
		TabE.Add(Choice(TEXT("ILLUMINATION GLOBALE"), TEXT("Lumen : la lumière rebondit sur les murs et colore la salle.\nÉcran : approximation plus légère.\nAucune : seulement la lumière directe, salles plus sombres."),
			&FState::Gi, { TEXT("LUMEN"), TEXT("ÉCRAN"), TEXT("AUCUNE") }, [](UWorld* W) { ApplyPost(W); }));
		TabE.Add(Group(TEXT("QUALITÉ DE L'ILLUMINATION"), TEXT("Nombre de rayons et de sondes de Lumen : plus haut, moins de grain et de taches dans les coins."), G_GI));
		{
			FDef D; D.Label = TEXT("REFLETS");
			D.Help = TEXT("Lumen : reflets de toute la scène, même hors de l'écran (avec l'illumination Lumen seulement).\nÉcran : seulement ce qui est visible.\nAucun : le plus léger.");
			D.Val = []() { static const TCHAR* N[3] = { TEXT("LUMEN"), TEXT("ÉCRAN"), TEXT("AUCUN") }; return FString(N[FMath::Clamp(EffRefl(), 0, 2)]); };
			D.Chg = [](UWorld* W, int32 Dir) {
				int32 Rf = EffRefl();
				for (int32 K = 0; K < 3; ++K) { Rf = Cycle(Rf, Dir, 3); if (Rf != 0 || St().Gi == 0) { break; } }
				St().Refl = Rf; Save(); ApplyPost(W); };
			TabE.Add(D);
		}
		TabE.Add(Group(TEXT("QUALITÉ DES REFLETS"), TEXT("Précision des reflets sur les sols et les métaux."), G_REFL));
		{
			FDef D; D.Label = TEXT("RAY TRACING MATÉRIEL"); D.Kind = EKind::Toggle;
			D.Help = TEXT("Lumen lance de vrais rayons sur la carte graphique (RTX, RX 6000 et plus) : lumière et reflets plus justes. Sinon, Lumen passe en mode logiciel.");
			D.On = []() { return St().Hwrt != 0; };
			D.Val = []() { return YesNo(St().Hwrt != 0); };
			D.Enabled = []() { return HwrtPossible(); };
			D.Why = []() { return FString(TEXT("Cette carte graphique (ou ce mode DirectX) ne gère pas le ray tracing matériel.")); };
			D.Chg = [](UWorld*, int32) { St().Hwrt = St().Hwrt ? 0 : 1; Save(); ApplyLighting(); };
			TabE.Add(D);
		}
		{
			FDef D = Choice(TEXT("ÉCLAIRAGE DES RAYONS"), TEXT("Qualité : chaque rayon calcule la vraie lumière au point touché (reflets précis).\nRapide : il lit le cache de Lumen."),
				&FState::RayLight, { TEXT("QUALITÉ"), TEXT("RAPIDE") }, [](UWorld* W) { ApplyPost(W); });
			D.Enabled = []() { return St().Hwrt != 0 && HwrtPossible() && (St().Gi == 0 || EffRefl() == 0); };
			D.Why = []() { return FString(TEXT("Seulement avec le ray tracing matériel et Lumen.")); };
			TabE.Add(D);
		}
		TabE.Add(Group(TEXT("QUALITÉ DES OMBRES"), TEXT("Définition des ombres et leur douceur."), G_SHADOW));
		TabE.Add(Toggle(TEXT("OCCLUSION AMBIANTE"), TEXT("Assombrit les coins et le contact entre les objets : plus de relief."), &FState::bAO, [](UWorld* W) { ApplyLighting(); ApplyPost(W); }));

		// ================================================================ 3 DÉTAILS
		TArray<FDef>& TabD = T[3];
		TabD.Add(Group(TEXT("TEXTURES"), TEXT("Résolution des textures gardées en mémoire vidéo. Baisse-la si la carte graphique a peu de mémoire."), G_TEX));
		{
			FDef D; D.Label = TEXT("FILTRAGE ANISOTROPE");
			D.Help = TEXT("Netteté des textures vues en biais (sols lointains).");
			D.Val = []() { return FString::Printf(TEXT("× %d"), St().Aniso > 0 ? St().Aniso : GetCV(TEXT("r.MaxAnisotropy"), 8)); };
			D.Chg = [](UWorld*, int32 Dir) { const int32 Num = UE_ARRAY_COUNT(AnisoSteps);
				const int32 Cur = St().Aniso > 0 ? St().Aniso : GetCV(TEXT("r.MaxAnisotropy"), 8);
				St().Aniso = AnisoSteps[FMath::Clamp(IndexIn(AnisoSteps, Num, Cur, 3) + Dir, 0, Num - 1)]; Save(); ApplyLighting(); };
			TabD.Add(D);
		}
		TabD.Add(Group(TEXT("DISTANCE D'AFFICHAGE"), TEXT("Distance à laquelle les objets passent à des versions plus simples."), G_VIEW));
		TabD.Add(Group(TEXT("EFFETS"), TEXT("Particules, étincelles, fumées : nombre et finesse."), G_FX));
		TabD.Add(Group(TEXT("POST-TRAITEMENT"), TEXT("Qualité du bloom, du flou et des filtres d'image."), G_PP));
		TabD.Add(Group(TEXT("OMBRAGE DES MATÉRIAUX"), TEXT("Précision du calcul de la lumière sur les matériaux."), G_SHADING));
		TabD.Add(Toggle(TEXT("BLOOM"), TEXT("Halo autour des lumières fortes et des néons (et coulure lumineuse du filtre Film)."), &FState::bBloom, [](UWorld* W) { ApplyPost(W); }));
		TabD.Add(Toggle(TEXT("VIGNETTAGE"), TEXT("Bords de l'image légèrement assombris."), &FState::bVignette, [](UWorld* W) { ApplyPost(W); }));
		TabD.Add(Toggle(TEXT("GRAIN"), TEXT("Grain de pellicule du filtre Film d'EON."), &FState::bGrain, [](UWorld* W) { ApplyPost(W); }));
		TabD.Add(Toggle(TEXT("ABERRATION CHROMATIQUE"), TEXT("Franges rouge et bleue sur les bords des formes, comme une vieille optique."), &FState::bChroma, [](UWorld* W) { ApplyPost(W); }));

		// ================================================================ 4 AUDIO
		TArray<FDef>& TabS = T[4];
		TabS.Add(Slider(TEXT("VOLUME GÉNÉRAL"), TEXT("Volume de tout le jeu."), &FState::VolumeGeneral, 0, 100, 5, [](UWorld* W) { ApplyAudio(W); }));
		TabS.Add(Slider(TEXT("EFFETS SONORES"), TEXT("Tirs, impacts, ennemis, portes et objets, par rapport au volume général."), &FState::Effects, 0, 100, 5,
			[](UWorld* W) { RefreshFeel(W); }));
		TabS.Add(Toggle(TEXT("SON SPATIALISÉ"), TEXT("Les ennemis s'entendent à gauche ou à droite selon leur place autour de Jira."), &FState::bSpatial, [](UWorld* W) { RefreshFeel(W); }));
		TabS.Add(Toggle(TEXT("SON EN ARRIÈRE-PLAN"), TEXT("Garder le son quand une autre fenêtre est devant le jeu."), &FState::bBgSound, [](UWorld* W) { ApplyAudio(W); }));

		// ================================================================ 5 COMMANDES
		TArray<FDef>& TabC = T[5];
		TabC.Add(Choice(TEXT("APPAREIL"), TEXT("Automatique : les invites et la visée suivent le dernier appareil touché.\nSinon, le jeu reste sur l'appareil choisi."),
			&FState::Device, { TEXT("AUTOMATIQUE"), TEXT("CLAVIER-SOURIS"), TEXT("MANETTE") }));
		TabC.Add(Choice(TEXT("STYLE DES BOUTONS"), TEXT("Noms et symboles des boutons de manette dans les invites."), &FState::PadStyle, { TEXT("XBOX"), TEXT("PLAYSTATION"), TEXT("NINTENDO") }));
		TabC.Add(Choice(TEXT("VISÉE ASSISTÉE"), TEXT("Les tirs se recentrent sur l'ennemi le plus proche de la visée."), &FState::Assist,
			{ TEXT("AUCUNE"), TEXT("FAIBLE"), TEXT("NORMALE"), TEXT("FORTE") }, [](UWorld* W) { RefreshFeel(W); }));
		TabC.Add(Slider(TEXT("ZONE MORTE DES STICKS"), TEXT("Course du stick ignorée autour du centre. Monte-la si Jira bouge ou vise toute seule (manette usée)."), &FState::DeadZone, 0, 40, 5,
			[](UWorld* W) { RebuildInput(W); }, [](int32 V) { return V <= 0 ? FString(TEXT("AUCUNE")) : FString::Printf(TEXT("%d %%"), V); }));
		TabC.Add(Toggle(TEXT("TIR AU STICK"), TEXT("Pousser le stick droit à fond tire, sans gâchette."), &FState::bStickFire));
		TabC.Add(Flag2(TEXT("COURSE"), TEXT("Maintenir : on court tant que la touche est enfoncée.\nBasculer : un appui pour courir, un autre pour marcher."),
			&FState::bRunToggle, TEXT("MAINTENIR"), TEXT("BASCULER")));
		{
			FDef D; D.Label = TEXT("TOUCHES"); D.Kind = EKind::Section;
			D.Help = TEXT("CLAVIER-SOURIS|MANETTE");   // titres des deux colonnes
			TabC.Add(D);
		}
		for (int32 B = 0; B < NumBinds; ++B)
		{
			FDef D; D.Label = BindDefs[B].Label; D.Kind = EKind::Bind; D.Bind = B;
			D.Help = TEXT("Choisis la colonne avec gauche / droite, valide, puis appuie sur la nouvelle touche. Échap (ou Menu) : annuler.\nUne touche déjà prise ailleurs passe en rouge.");
			TabC.Add(D);
		}
		{
			FDef D; D.Label = TEXT("RÉTABLIR LES TOUCHES"); D.Kind = EKind::Action;
			D.Help = TEXT("Remet toutes les touches et tous les boutons d'origine.");
			D.Val = []() { return FString(TEXT("RÉTABLIR")); };
			D.Act = [](UWorld* W) { St().Binds.Reset(); Save(); RebuildInput(W); UE_LOG(LogEON, Display, TEXT("EON options : touches d'origine")); };
			TabC.Add(D);
		}

		// ================================================================ 6 JOUABILITÉ
		TArray<FDef>& TabJ = T[6];
		TabJ.Add(Slider(TEXT("TREMBLEMENTS DE CAMÉRA"), TEXT("Secousses aux impacts, explosions et coups reçus."), &FState::Shake, 0, 100, 10, [](UWorld* W) { RefreshFeel(W); }));
		TabJ.Add(Toggle(TEXT("ARRÊT SUR IMAGE"), TEXT("Le jeu se fige une fraction de seconde aux coups forts (hit-stop)."), &FState::bHitstop));
		TabJ.Add(Toggle(TEXT("CHIFFRES DE DÉGÂTS"), TEXT("Dégâts affichés en lettres manga au-dessus des ennemis."), &FState::bDmgNums, [](UWorld* W) { RefreshFeel(W); }));
		TabJ.Add(Slider(TEXT("OPACITÉ DU HUD"), TEXT("Transparence de la vie, de l'énergie, de la mini-carte et des capacités."), &FState::HudOpacity, 30, 100, 10));
		TabJ.Add(Slider(TEXT("TAILLE DU HUD"), TEXT("Taille des éléments du HUD dans les coins de l'écran."), &FState::HudScale, 70, 130, 10));
		TabJ.Add(Flag2(TEXT("MINI-CARTE"), TEXT("Fixe : la carte garde le haut de l'écran.\nTournante : elle tourne avec la visée de Jira, qui pointe toujours vers le haut."),
			&FState::bMapRotate, TEXT("FIXE"), TEXT("TOURNANTE")));

		// ================================================================ 7 ACCESSIBILITÉ
		TArray<FDef>& TabX = T[7];
		TabX.Add(Choice(TEXT("DALTONISME"), TEXT("Corrige les couleurs de tout l'écran pour le type de daltonisme choisi."), &FState::Cvd,
			{ TEXT("AUCUN"), TEXT("PROTANOPIE"), TEXT("DEUTÉRANOPIE"), TEXT("TRITANOPIE") }, [](UWorld*) { ApplyColor(); }));
		{
			FDef D = Slider(TEXT("FORCE DE LA CORRECTION"), TEXT("Intensité de la correction des couleurs."), &FState::CvdPct, 10, 100, 10, [](UWorld*) { ApplyColor(); });
			D.Enabled = []() { return St().Cvd != 0; };
			D.Why = []() { return FString(TEXT("Choisis d'abord un type de daltonisme.")); };
			TabX.Add(D);
		}
		TabX.Add(Choice(TEXT("TAILLE DU TEXTE"), TEXT("Taille du texte des dialogues."), &FState::TextSize, { TEXT("NORMALE"), TEXT("GRANDE"), TEXT("TRÈS GRANDE") }));
		TabX.Add(Choice(TEXT("VITESSE DU TEXTE"), TEXT("Vitesse d'apparition des lettres dans les dialogues."), &FState::TextSpeed, { TEXT("LENTE"), TEXT("NORMALE"), TEXT("RAPIDE"), TEXT("INSTANTANÉE") }));
		TabX.Add(Toggle(TEXT("CONTRASTE ÉLEVÉ"), TEXT("Fonds de l'interface opaques, textes et traits plus lumineux."), &FState::bContrast));
		TabX.Add(Toggle(TEXT("RÉDUIRE LES FLASHS"), TEXT("Supprime les flashs blancs des coups et des tirs, et les clignotements de l'interface."), &FState::bNoFlash, [](UWorld* W) { RefreshFeel(W); }));
		TabX.Add(Toggle(TEXT("APPUYER AU LIEU DE MAINTENIR"), TEXT("Tir et Onde : un appui démarre, un second appui arrête."), &FState::bHoldToggle));
		TabX.Add(Toggle(TEXT("MODE FACILE"), TEXT("Jira encaisse moitié moins de dégâts (deux coups d'un cœur n'en retirent qu'un) et les chutes ne coûtent plus de vie."), &FState::bEasy, [](UWorld* W) { RefreshFeel(W); }));
		return T;
	}

	/** Lignes visibles d'un onglet (indices dans Defs). */
	TArray<int32> VisibleRows(int32 Tab)
	{
		TArray<int32> R;
		const TArray<TArray<FDef>>& T = Defs();
		if (!T.IsValidIndex(Tab)) { return R; }
		for (int32 I = 0; I < T[Tab].Num(); ++I) { if (!T[Tab][I].Visible || T[Tab][I].Visible()) { R.Add(I); } }
		return R;
	}

	const FDef* DefAt(int32 Tab, int32 Row)
	{
		const TArray<int32> V = VisibleRows(Tab);
		return V.IsValidIndex(Row) ? &Defs()[Tab][V[Row]] : nullptr;
	}
	bool Usable(const FDef* D) { return D && D->Kind != EONOptions::EKind::Section && (!D->Enabled || D->Enabled()); }
}

// ==================================================================== API

namespace EONOptions
{
	FString TabName(int32 Tab)
	{
		static const TCHAR* N[NumTabs] = { TEXT("AFFICHAGE"), TEXT("GRAPHISMES"), TEXT("ÉCLAIRAGE"), TEXT("DÉTAILS"), TEXT("AUDIO"), TEXT("COMMANDES"), TEXT("JOUABILITÉ"), TEXT("ACCESSIBILITÉ") };
		return Tab >= 0 && Tab < NumTabs ? N[Tab] : TEXT("");
	}

	TArray<FRow> Rows(int32 Tab)
	{
		TArray<FRow> Out;
		const UInputMappingContext* Ctx = ActiveContext(GameWorld());
		// Qui utilise chaque touche (conflits), clavier et manette : contexte actif, course, pause.
		TMap<FKey, TArray<int32>> Users;
		if (Tab == 5)
		{
			if (Ctx)
			{
				for (const FEnhancedActionKeyMapping& M : Ctx->GetMappings())
				{
					const int32 Owner = OwnerOf(M);
					if (Owner >= 0) { Users.FindOrAdd(M.Key).AddUnique(Owner); }
				}
			}
			if (const int32 Run = RunBind(); Run != INDEX_NONE) { for (const FKey& K : BindKeys(Ctx, Run, false)) { Users.FindOrAdd(K).AddUnique(Run); } }
		}
		for (const int32 I : VisibleRows(Tab))
		{
			const FDef& D = Defs()[Tab][I];
			FRow R;
			R.Label = D.Label;
			R.Help = D.HelpFn ? D.HelpFn() : D.Help;
			R.Kind = D.Kind;
			R.Preview = D.Preview;
			R.bEnabled = D.Kind != EKind::Section && (!D.Enabled || D.Enabled());
			if (D.Kind != EKind::Section && !R.bEnabled && D.Why) { R.Help = D.Why(); }
			if (D.Val) { R.Value = D.Val(); }
			if (D.Fill) { R.Fill = FMath::Clamp(D.Fill(), 0.f, 1.f); }
			if (D.On) { R.bOn = D.On(); }
			if (D.Kind == EKind::Section)
			{
				FString Left, Right;
				if (D.Help.Split(TEXT("|"), &Left, &Right)) { R.KeyKb = Left; R.KeyPad = Right; R.bHasKb = R.bHasPad = true; }
				R.Help.Reset();
			}
			if (D.Bind >= 0)
			{
				const FBindDef& Bd = BindDefs[D.Bind];
				R.bHasKb = Bd.bKb;
				R.bHasPad = Bd.bPad;
				for (const bool bPad : { false, true })
				{
					if (!(bPad ? Bd.bPad : Bd.bKb)) { continue; }
					TArray<FString> Names;
					for (const FKey& K : BindKeys(Ctx, D.Bind, bPad)) { Names.AddUnique(KeyLabel(K)); }
					(bPad ? R.KeyPad : R.KeyKb) = FString::Join(Names, TEXT(" / "));
					// Conflit : une touche de cette ligne (même secondaire : flèches, clic droit...) sert aussi ailleurs.
					TArray<FString> With;
					for (const TPair<FKey, TArray<int32>>& U : Users)
					{
						if (U.Key.IsGamepadKey() != bPad || !U.Value.Contains(D.Bind)) { continue; }
						for (const int32 O : U.Value) { if (O != D.Bind) { With.AddUnique(OwnerLabel(O)); } }
					}
					(bPad ? R.bConflictPad : R.bConflictKb) = With.Num() > 0;
					(bPad ? R.ConflictPadWith : R.ConflictKbWith) = FString::Join(With, TEXT(", "));
				}
			}
			Out.Add(R);
		}
		return Out;
	}

	void Change(UWorld* W, int32 Tab, int32 Row, int32 Dir)
	{
		const FDef* D = DefAt(Tab, Row);
		if (!Usable(D) || !D->Chg) { return; }
		D->Chg(W, Dir);
		UE_LOG(LogEON, Display, TEXT("EON option %s : %s"), *D->Label, D->Val ? *D->Val() : TEXT("-"));
	}

	void SetFraction(UWorld* W, int32 Tab, int32 Row, float T)
	{
		const FDef* D = DefAt(Tab, Row);
		if (!Usable(D) || !D->Frac) { return; }
		const FString Before = D->Val ? D->Val() : FString();
		D->Frac(W, FMath::Clamp(T, 0.f, 1.f));
		if (D->Val && D->Val() != Before) { UE_LOG(LogEON, Display, TEXT("EON option %s : %s (souris)"), *D->Label, *D->Val()); }
	}

	bool Activate(UWorld* W, int32 Tab, int32 Row, bool bPad)
	{
		const FDef* D = DefAt(Tab, Row);
		if (!Usable(D)) { return false; }
		if (D->Kind == EKind::Bind) { return bPad ? BindDefs[D->Bind].bPad : BindDefs[D->Bind].bKb; }
		if (D->Act) { D->Act(W); return false; }
		if (D->Kind == EKind::Toggle || D->Kind == EKind::Choice) { Change(W, Tab, Row, 1); }
		return false;
	}

	void Rebind(UWorld* W, int32 Tab, int32 Row, bool bPad, const FKey& Key)
	{
		const FDef* D = DefAt(Tab, Row);
		if (!D || D->Bind < 0 || !Key.IsValid()) { return; }
		St().Binds.Add(BindKey(D->Bind, bPad), Key.GetFName().ToString());
		Save();
		RebuildInput(W);
		UE_LOG(LogEON, Display, TEXT("EON option touche %s (%s) : %s"), BindDefs[D->Bind].Id, bPad ? TEXT("manette") : TEXT("clavier"), *Key.GetFName().ToString());
	}

	void ApplyWorld(UWorld* W)
	{
		if (!W || !W->IsGameWorld()) { return; }
		static FTSTicker::FDelegateHandle Bg;
		if (!Bg.IsValid()) { Bg = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickBackground), 0.25f); }
		if (St().bScala) { PushGroups(); }
		ApplyUpscaler(W);
		ApplyLighting();
		ApplyColor();
		ApplyAudio(W);
		ApplyPost(W);
		RefreshFeel(W);
	}

	void ApplyFeel(FEONGameFeelRow& F)
	{
		const FState& S = St();
		static const float CamMul[4] = { 0.8f, 1.f, 1.25f, 1.5f };
		static const float AssistMul[4] = { 0.f, 0.5f, 1.f, 2.f };
		F.Shake *= FMath::Clamp(S.Shake, 0, 100) / 100.f;
		F.DmgNums = F.DmgNums && S.bDmgNums;
		F.AimAssist *= AssistMul[FMath::Clamp(S.Assist, 0, 3)];
		const float Fx = FMath::Clamp(S.Effects, 0, 100) / 100.f;
		F.VolJira *= Fx; F.VolImpacts *= Fx; F.VolEnnemis *= Fx; F.VolMonde *= Fx;
		F.SonSpatial = F.SonSpatial && S.bSpatial;
		F.CamViewTiles = FMath::Clamp(F.CamViewTiles * CamMul[FMath::Clamp(S.CamDist, 0, 3)], F.CamZoomMinTiles, F.CamZoomMaxTiles);
		if (S.bNoFlash) { F.HitFlash = false; F.Flash = false; }
		if (S.bEasy) { F.FallDmg = 0.f; }
	}

	int32 Device() { return St().Device; }
	int32 PadStyle() { return St().PadStyle; }
	bool RunToggle() { return St().bRunToggle; }
	bool StickFire() { return St().bStickFire; }
	bool HitstopOn() { return St().bHitstop; }
	bool ShowFps() { return St().bFps; }
	float HudOpacity() { return FMath::Clamp(St().HudOpacity, 30, 100) / 100.f; }
	float HudScale() { return FMath::Clamp(St().HudScale, 70, 130) / 100.f; }
	bool MapRotates() { return St().bMapRotate; }
	int32 TextSize() { return FMath::Clamp(St().TextSize, 0, 2); }
	float TextSpeed() { static const float Sp[4] = { 0.5f, 1.f, 2.f, 0.f }; return Sp[FMath::Clamp(St().TextSpeed, 0, 3)]; }
	bool HighContrast() { return St().bContrast; }
	bool ReduceFlashes() { return St().bNoFlash; }
	bool HoldToggle() { return St().bHoldToggle; }
	float StickDeadZone() { return FMath::Clamp(St().DeadZone, 0, 40) / 100.f; }
	bool EasyMode() { return St().bEasy; }

	int32 EasyDamage(int32 Raw)
	{
		static float Carry = 0.f;
		if (!St().bEasy || Raw <= 0) { return Raw; }
		Carry += Raw * 0.5f;
		const int32 Loss = FMath::FloorToInt(Carry + 0.001f);
		Carry -= Loss;
		return Loss;
	}

	FKey RunKey()
	{
		const FKey K = StoredKey(RunBind(), false);
		return K.IsValid() ? K : EKeys::LeftShift;
	}

	UInputMappingContext* BuildMappingContext(UObject* Outer, const UInputMappingContext* Source)
	{
		if (!Source) { return nullptr; }
		UInputMappingContext* C = DuplicateObject<UInputMappingContext>(Source, Outer ? Outer : GetTransientPackage());
		TArray<FEnhancedActionKeyMapping> Ms = C->GetMappings();
		TArray<int32> Drop;
		for (int32 B = 0; B < NumBinds; ++B)
		{
			const FBindDef& D = BindDefs[B];
			if (!D.Action) { continue; }
			for (const bool bPad : { false, true })
			{
				const FKey NewKey = StoredKey(B, bPad);
				if (!NewKey.IsValid()) { continue; }
				const TArray<int32> Idx = Primary(Ms, B, bPad);
				if (Idx.IsEmpty())
				{
					// Pas de touche d'origine pour cet appareil (ex. greffe suivante au clavier) : nouvelle ligne pour l'action.
					const FEnhancedActionKeyMapping* Any = Ms.FindByPredicate([&D](const FEnhancedActionKeyMapping& X) { return IsAction(X, D.Action); });
					if (Any && D.Dir < 0) { Ms.Add(FEnhancedActionKeyMapping(Any->Action, NewKey)); }
					continue;
				}
				// On garde une ligne (ses modificateurs portent le sens du déplacement) ; les autres lettres (QWERTY) partent.
				int32 Keep = Idx[0];
				for (const int32 I : Idx) { if (Ms[I].Key == NewKey) { Keep = I; break; } }
				Ms[Keep].Key = NewKey;
				for (const int32 I : Idx) { if (I != Keep) { Drop.AddUnique(I); } }
			}
		}
		Drop.Sort([](int32 X, int32 Y) { return X > Y; });
		for (const int32 I : Drop) { Ms.RemoveAt(I); }
		// Zone morte du stick de déplacement (celui de visée garde AimDeadZone, relevée par le contrôleur).
		if (const float Dz = StickDeadZone(); Dz > 0.f)
		{
			for (FEnhancedActionKeyMapping& M : Ms)
			{
				if (!IsAction(M, TEXT("IA_Move")) || M.Key != EKeys::Gamepad_Left2D) { continue; }
				UInputModifierDeadZone* Z = NewObject<UInputModifierDeadZone>(C);
				Z->LowerThreshold = Dz;
				Z->Type = EDeadZoneType::Radial;
				M.Modifiers.Add(Z);
			}
		}
		// Jamais UnmapKey : il retirerait la touche de toutes les lignes de l'action (les autres sens du déplacement).
		C->UnmapAll();
		for (const FEnhancedActionKeyMapping& M : Ms) { C->MapKey(M.Action, M.Key) = M; }
		return C;
	}

	FString KeyLabel(const FKey& K)
	{
		if (!K.IsValid()) { return FString(); }
		const int32 Ps = FMath::Clamp(St().PadStyle, 0, 2);
		struct FPadName { FKey Key; const TCHAR* N[3]; };
		static const FPadName Pad[] = {
			{ EKeys::Gamepad_FaceButton_Bottom, { TEXT("A"), TEXT("PS_CROIX"), TEXT("B") } },
			{ EKeys::Gamepad_FaceButton_Right, { TEXT("B"), TEXT("PS_ROND"), TEXT("A") } },
			{ EKeys::Gamepad_FaceButton_Left, { TEXT("X"), TEXT("PS_CARRE"), TEXT("Y") } },
			{ EKeys::Gamepad_FaceButton_Top, { TEXT("Y"), TEXT("PS_TRIANGLE"), TEXT("X") } },
			{ EKeys::Gamepad_LeftShoulder, { TEXT("LB"), TEXT("L1"), TEXT("L") } },
			{ EKeys::Gamepad_RightShoulder, { TEXT("RB"), TEXT("R1"), TEXT("R") } },
			{ EKeys::Gamepad_LeftTrigger, { TEXT("LT"), TEXT("L2"), TEXT("ZL") } },
			{ EKeys::Gamepad_RightTrigger, { TEXT("RT"), TEXT("R2"), TEXT("ZR") } },
			{ EKeys::Gamepad_LeftTriggerAxis, { TEXT("LT"), TEXT("L2"), TEXT("ZL") } },
			{ EKeys::Gamepad_RightTriggerAxis, { TEXT("RT"), TEXT("R2"), TEXT("ZR") } },
			{ EKeys::Gamepad_LeftThumbstick, { TEXT("L3"), TEXT("L3"), TEXT("L3") } },
			{ EKeys::Gamepad_RightThumbstick, { TEXT("R3"), TEXT("R3"), TEXT("R3") } },
			{ EKeys::Gamepad_DPad_Up, { TEXT("↑"), TEXT("↑"), TEXT("↑") } },
			{ EKeys::Gamepad_DPad_Down, { TEXT("↓"), TEXT("↓"), TEXT("↓") } },
			{ EKeys::Gamepad_DPad_Left, { TEXT("←"), TEXT("←"), TEXT("←") } },
			{ EKeys::Gamepad_DPad_Right, { TEXT("→"), TEXT("→"), TEXT("→") } },
			{ EKeys::Gamepad_Special_Left, { TEXT("VUE"), TEXT("CRÉER"), TEXT("−") } },
			{ EKeys::Gamepad_Special_Right, { TEXT("MENU"), TEXT("OPTIONS"), TEXT("+") } },
			{ EKeys::Gamepad_Left2D, { TEXT("STICK G"), TEXT("STICK G"), TEXT("STICK G") } },
			{ EKeys::Gamepad_Right2D, { TEXT("STICK D"), TEXT("STICK D"), TEXT("STICK D") } },
		};
		for (const FPadName& P : Pad) { if (P.Key == K) { return P.N[Ps]; } }
		static const TMap<FName, const TCHAR*> Kb = {
			{ EKeys::SpaceBar.GetFName(), TEXT("ESPACE") }, { EKeys::Enter.GetFName(), TEXT("ENTRÉE") }, { EKeys::Escape.GetFName(), TEXT("ÉCHAP") },
			{ EKeys::Tab.GetFName(), TEXT("TAB") }, { EKeys::LeftShift.GetFName(), TEXT("MAJ") }, { EKeys::RightShift.GetFName(), TEXT("MAJ D") },
			{ EKeys::LeftControl.GetFName(), TEXT("CTRL") }, { EKeys::RightControl.GetFName(), TEXT("CTRL D") }, { EKeys::LeftAlt.GetFName(), TEXT("ALT") },
			{ EKeys::RightAlt.GetFName(), TEXT("ALT GR") }, { EKeys::BackSpace.GetFName(), TEXT("RETOUR") }, { EKeys::Delete.GetFName(), TEXT("SUPPR") },
			{ EKeys::Insert.GetFName(), TEXT("INSER") }, { EKeys::Home.GetFName(), TEXT("DÉBUT") }, { EKeys::End.GetFName(), TEXT("FIN") },
			{ EKeys::PageUp.GetFName(), TEXT("PAGE ↑") }, { EKeys::PageDown.GetFName(), TEXT("PAGE ↓") }, { EKeys::CapsLock.GetFName(), TEXT("VERR MAJ") },
			{ EKeys::Up.GetFName(), TEXT("↑") }, { EKeys::Down.GetFName(), TEXT("↓") }, { EKeys::Left.GetFName(), TEXT("←") }, { EKeys::Right.GetFName(), TEXT("→") },
			{ EKeys::LeftMouseButton.GetFName(), TEXT("CLIC G") }, { EKeys::RightMouseButton.GetFName(), TEXT("CLIC D") },
			{ EKeys::MiddleMouseButton.GetFName(), TEXT("CLIC M") }, { EKeys::ThumbMouseButton.GetFName(), TEXT("SOURIS 4") },
			{ EKeys::ThumbMouseButton2.GetFName(), TEXT("SOURIS 5") },
			{ EKeys::One.GetFName(), TEXT("1") }, { EKeys::Two.GetFName(), TEXT("2") }, { EKeys::Three.GetFName(), TEXT("3") }, { EKeys::Four.GetFName(), TEXT("4") },
			{ EKeys::Five.GetFName(), TEXT("5") }, { EKeys::Six.GetFName(), TEXT("6") }, { EKeys::Seven.GetFName(), TEXT("7") }, { EKeys::Eight.GetFName(), TEXT("8") },
			{ EKeys::Nine.GetFName(), TEXT("9") }, { EKeys::Zero.GetFName(), TEXT("0") },
		};
		if (const TCHAR* const* N = Kb.Find(K.GetFName())) { return *N; }
		return K.GetDisplayName(false).ToString().ToUpper();
	}

	FString ActionLabel(const UInputMappingContext* Ctx, const UInputAction* Action, bool bPad)
	{
		if (!Ctx || !Action) { return FString(); }
		for (const FEnhancedActionKeyMapping& M : Ctx->GetMappings())
		{
			if (M.Action == Action && M.Key.IsGamepadKey() == bPad) { return KeyLabel(M.Key); }
		}
		return FString();
	}
}
