#include "EONPlayerController.h"
#include "EONUI.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONFloorManager.h"
#include "EONCombatDirector.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "EONPlayerCameraManager.h"
#include "EONRoomBounds.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "Misc/App.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

/** Écoute toutes les entrées Slate pour savoir quel périphérique a servi en dernier. */
class FEONDeviceInputProcessor : public IInputProcessor
{
public:
	explicit FEONDeviceInputProcessor(AEONPlayerController* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& E) override
	{
		Notify(E.GetKey().IsGamepadKey());
		return false;
	}

	virtual bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent& E) override
	{
		// Un stick au repos envoie du bruit : on ne bascule qu'au-delà de 0,25.
		if (E.GetKey().IsGamepadKey() && FMath::Abs(E.GetAnalogValue()) > 0.25f) { Notify(true); }
		return false;
	}

	virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& E) override
	{
		if (E.GetCursorDelta().SizeSquared() > 4.f) { Notify(false); }
		return false;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent&) override
	{
		Notify(false);
		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("EONDeviceInput"); }

private:
	void Notify(bool bGamepad)
	{
		if (AEONPlayerController* PC = Owner.Get()) { PC->NotifyDeviceUsed(bGamepad); }
	}

	TWeakObjectPtr<AEONPlayerController> Owner;
};

AEONPlayerController::AEONPlayerController()
{
	// Écran de niveau : le jeu est en pause, PlayerTick doit continuer (choix, pilote automatique).
	bShouldPerformFullTickWhenPaused = true;
	PlayerCameraManagerClass = AEONPlayerCameraManager::StaticClass();
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	const FString Base = TEXT("/Game/EON/Input/");
	auto IA = [&Base](const TCHAR* Name) { return TSoftObjectPtr<UInputAction>(FSoftObjectPath(Base + Name + TEXT(".") + Name)); };
	MappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(Base + TEXT("IMC_EON.IMC_EON")));
	IA_Move = IA(TEXT("IA_Move"));
	IA_Aim = IA(TEXT("IA_Aim"));
	IA_Fire = IA(TEXT("IA_Fire"));
	IA_Dash = IA(TEXT("IA_Dash"));
	IA_Spell = IA(TEXT("IA_Spell"));
	IA_Holo = IA(TEXT("IA_Holo"));
	IA_Ultimate = IA(TEXT("IA_Ultimate"));
	IA_Interact = IA(TEXT("IA_Interact"));
	IA_Map = IA(TEXT("IA_Map"));
	IA_Pause = IA(TEXT("IA_Pause"));
	IA_GreffeNext = IA(TEXT("IA_GreffeNext"));
	IA_GreffePrev = IA(TEXT("IA_GreffePrev"));
	for (int32 i = 1; i <= 5; ++i)
	{
		IA_GreffeSlots.Add(IA(*FString::Printf(TEXT("IA_Greffe%d"), i)));
	}
}

void AEONPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Hub 3D (carte au C.O.M « COMv1 ») : il devient la salle centrale de l'étage ; les salles générées sont construites à 120 m à l'est.
	if (GetWorld() && GetWorld()->IsGameWorld() && !AEONFloorManager::Get(this))
	{
		for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
		{
			const UStaticMesh* M = It->GetStaticMeshComponent() ? It->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
			if (!M || !M->GetName().StartsWith(TEXT("COMv1"))) { continue; }
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AEONFloorManager>(AEONFloorManager::StaticClass(), It->GetActorLocation() + FVector(12000.f, 0.f, 0.f), FRotator::ZeroRotator, P);
			break;
		}
	}

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(Mode);

	if (IsLocalController() && FSlateApplication::IsInitialized())
	{
		DeviceProcessor = MakeShared<FEONDeviceInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(DeviceProcessor);
	}
}

void AEONPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (DeviceProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(DeviceProcessor);
	}
	DeviceProcessor.Reset();
	Super::EndPlay(Reason);
}

int32 AEONPlayerController::RepairMoveMappings(UInputMappingContext* IMC, UInputAction* MoveAction)
{
	if (!IMC || !MoveAction) { return 0; }
	// Le déplacement est un axe 2D : X = droite, Y = haut de l'écran.
	auto Up = [](const FKey& K) { return K == EKeys::W || K == EKeys::Z || K == EKeys::Up; };
	auto Down = [](const FKey& K) { return K == EKeys::S || K == EKeys::Down; };
	auto Left = [](const FKey& K) { return K == EKeys::A || K == EKeys::Q || K == EKeys::Left; };
	int32 Fixed = 0;
	for (int32 I = 0; I < IMC->GetMappings().Num(); ++I)
	{
		FEnhancedActionKeyMapping& M = IMC->GetMapping(I);
		if (M.Action != MoveAction || M.Key.IsGamepadKey()) { continue; }
		const bool bSwz = Up(M.Key) || Down(M.Key), bNeg = Down(M.Key) || Left(M.Key);
		const bool bHasSwz = M.Modifiers.ContainsByPredicate([](const UInputModifier* X) { return X && X->IsA<UInputModifierSwizzleAxis>(); });
		const bool bHasNeg = M.Modifiers.ContainsByPredicate([](const UInputModifier* X) { return X && X->IsA<UInputModifierNegate>(); });
		if (bSwz == bHasSwz && bNeg == bHasNeg) { continue; }
		M.Modifiers.Reset();
		if (bSwz)
		{
			UInputModifierSwizzleAxis* S = NewObject<UInputModifierSwizzleAxis>(IMC);
			S->Order = EInputAxisSwizzle::YXZ;
			M.Modifiers.Add(S);
		}
		if (bNeg) { M.Modifiers.Add(NewObject<UInputModifierNegate>(IMC)); }
		++Fixed;
	}
	return Fixed;
}

void AEONPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (UInputMappingContext* IMC = MappingContext.LoadSynchronous())
		{
			// Filet : si l'asset a perdu ses modificateurs, toutes les touches pousseraient vers la droite.
			if (const int32 N = RepairMoveMappings(IMC, IA_Move.LoadSynchronous()))
			{
				UE_LOG(LogEON, Warning, TEXT("IMC_EON : %d touches de deplacement corrigees en memoire (asset a resauver)"), N);
			}
			Sub->AddMappingContext(IMC, 0);
		}
		else
		{
			UE_LOG(LogEON, Warning, TEXT("IMC_EON introuvable : lancer Saved/Scripts/eon_inputs.py"));
		}
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) { return; }

	if (UInputAction* A = IA_Move.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Triggered, this, &AEONPlayerController::OnMove);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AEONPlayerController::OnMoveStop);
	}
	if (UInputAction* A = IA_Aim.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Triggered, this, &AEONPlayerController::OnAim);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AEONPlayerController::OnAimStop);
	}

	// Jalon 1 : tir et dash sont branchés sur Jira.
	if (UInputAction* A = IA_Fire.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &AEONPlayerController::OnFireStart);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AEONPlayerController::OnFireStop);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &AEONPlayerController::OnFireStop);
	}
	if (UInputAction* A = IA_Dash.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Started, this, &AEONPlayerController::OnDashStart);
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AEONPlayerController::OnDashStop);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &AEONPlayerController::OnDashStop);
	}

	// Les autres actions ne font qu'afficher leur nom (preuve que l'entrée arrive).
	auto BindNamed = [this, EIC](const TSoftObjectPtr<UInputAction>& Soft, const FString& Name)
	{
		if (UInputAction* A = Soft.LoadSynchronous())
		{
			// Écran de niveau (jeu en pause) : tir, interaction et greffes servent à choisir une carte.
			if (Name == TEXT("Tir") || Name == TEXT("Interagir") || Name.StartsWith(TEXT("Greffe"))) { A->bTriggerWhenPaused = true; }
			EIC->BindAction(A, ETriggerEvent::Started, this, &AEONPlayerController::OnNamedAction, Name);
		}
	};
	BindNamed(IA_Fire, TEXT("Tir"));
	BindNamed(IA_Dash, TEXT("Dash"));
	BindNamed(IA_Spell, TEXT("Onde"));
	if (UInputAction* A = IA_Spell.LoadSynchronous())
	{
		EIC->BindAction(A, ETriggerEvent::Completed, this, &AEONPlayerController::OnSpellStop);
		EIC->BindAction(A, ETriggerEvent::Canceled, this, &AEONPlayerController::OnSpellStop);
	}
	BindNamed(IA_Holo, TEXT("Hologramme"));
	BindNamed(IA_Ultimate, TEXT("Ultime"));
	BindNamed(IA_Interact, TEXT("Interagir"));
	BindNamed(IA_Map, TEXT("Carte"));
	BindNamed(IA_Pause, TEXT("Pause"));
	BindNamed(IA_GreffeNext, TEXT("Greffe suivante"));
	BindNamed(IA_GreffePrev, TEXT("Greffe précédente"));
	for (int32 i = 0; i < IA_GreffeSlots.Num(); ++i)
	{
		BindNamed(IA_GreffeSlots[i], FString::Printf(TEXT("Greffe %d"), i + 1));
	}
}

void AEONPlayerController::NotifyDeviceUsed(bool bGamepad)
{
	if (bGamepad == bUsingGamepad) { return; }
	bUsingGamepad = bGamepad;
	UE_LOG(LogEON, Display, TEXT("EON_DEVICE %s"), bGamepad ? TEXT("manette") : TEXT("clavier-souris"));
	OnInputDeviceChanged.Broadcast(bUsingGamepad);
}

void AEONPlayerController::OnMove(const FInputActionValue& V)
{
	if (DevMenu.IsOpen()) { return; }   // les flèches servent au menu ; ZQSD est lu dans PlayerTick
	if (AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn())) { Jira->SetMoveInput(V.Get<FVector2D>()); }
}

void AEONPlayerController::OnMoveStop(const FInputActionValue&)
{
	if (AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn())) { Jira->SetMoveInput(FVector2D::ZeroVector); }
}

void AEONPlayerController::OnAim(const FInputActionValue& V)
{
	StickAim = V.Get<FVector2D>();
}

void AEONPlayerController::OnAimStop(const FInputActionValue&)
{
	StickAim = FVector2D::ZeroVector;
}

void AEONPlayerController::OnFireStart(const FInputActionValue&) { bFireInput = true; }
void AEONPlayerController::OnFireStop(const FInputActionValue&) { bFireInput = false; }

void AEONPlayerController::OnDashStart(const FInputActionValue&)
{
	bDashInput = true;
	if (const AEONFloorManager* FM = AEONFloorManager::Get(this); FM && FM->IsCinematic()) { return; }
	if (AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn()))
	{
		Jira->SetDashHeld(true);
		Jira->PressDash();
	}
}

void AEONPlayerController::OnDashStop(const FInputActionValue&)
{
	bDashInput = false;
	if (AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn())) { Jira->SetDashHeld(false); }
}

void AEONPlayerController::OnNamedAction(const FInputActionValue&, FString Name)
{
	LastAction = Name;
	UE_LOG(LogEON, Display, TEXT("EON_ACTION %s (%s)"), *Name, bUsingGamepad ? TEXT("manette") : TEXT("clavier"));
	// Jalon 4 : sorts, ultime et greffes.
	AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn());
	if (!Jira) { return; }
	// Terminal, dialogue ou écran titre ouverts : ils lisent leurs touches eux-mêmes (TickUI).
	if (const UEONUISubsystem* UI = UEONUISubsystem::Get(this); UI && (UI->IsBlocking() || (Name == TEXT("Interagir") && UI->JustClosed()))) { return; }
	if (const AEONFloorManager* FM = AEONFloorManager::Get(this); FM && FM->IsCinematic()) { return; }
	// Choix de niveau : [1] [2] [3] directement, ou greffe suivante / précédente puis tir ou Interagir.
	if (Jira->IsLevelUpOpen())
	{
		if (Name == TEXT("Greffe suivante")) { Jira->LevelNav(1); }
		else if (Name == TEXT("Greffe précédente")) { Jira->LevelNav(-1); }
		else if (Name.StartsWith(TEXT("Greffe "))) { Jira->PickLevel(FCString::Atoi(*Name.RightChop(7)) - 1); }
		else if (Name == TEXT("Tir") || Name == TEXT("Interagir")) { Jira->PickLevel(Jira->GetLevelSel()); }
		return;
	}
	if (DevMenu.IsOpen() && (Name == TEXT("Interagir") || Name == TEXT("Pause"))) { return; }   // Entrée et Échap servent au menu
	if (Name == TEXT("Interagir")) { Interact(); }
	else if (Name == TEXT("Onde")) { Jira->SetSpellHeld(true); Jira->PressSpell(); }
	else if (Name == TEXT("Hologramme")) { Jira->PressHolo(); }
	else if (Name == TEXT("Ultime")) { Jira->PressUlt(); }
	else if (Name == TEXT("Greffe suivante")) { Jira->CycleGreffe(1); }
	else if (Name == TEXT("Greffe précédente")) { Jira->CycleGreffe(-1); }
	else if (Name.StartsWith(TEXT("Greffe "))) { Jira->SelectGreffeSlot(FCString::Atoi(*Name.RightChop(7)) - 1); }
}

void AEONPlayerController::OnSpellStop(const FInputActionValue&)
{
	if (AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn())) { Jira->SetSpellHeld(false); }
}

void AEONPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn());
	if (!Jira) { return; }
	DevMenu.Tick(this, FMath::Min((float)FApp::GetDeltaTime(), 0.1f));
	ApplyPostProcess();
	// Jalon 8 : écran titre, dialogues, terminal du C.O.M (temps réel, jeu en pause).
	if (UEONUISubsystem* UI = UEONUISubsystem::Get(this))
	{
		UI->TickUI(this, FMath::Min((float)FApp::GetDeltaTime(), 0.1f));
		if (UI->IsBlocking()) { Jira->SetFireHeld(false); Jira->SetMoveInput(FVector2D::ZeroVector); return; }
	}
	// Cinématique de réveil du Kibo : Jira ne répond pas.
	if (const AEONFloorManager* FM = AEONFloorManager::Get(this); FM && FM->IsCinematic()) { Jira->SetFireHeld(false); Jira->SetMoveInput(FVector2D::ZeroVector); return; }
	if (DevMenu.IsOpen())
	{
		// Menu ouvert : Jira se déplace en ZQSD / WASD seulement.
		const float X = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - ((IsInputKeyDown(EKeys::Q) || IsInputKeyDown(EKeys::A)) ? 1.f : 0.f);
		const float Y = ((IsInputKeyDown(EKeys::Z) || IsInputKeyDown(EKeys::W)) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
		Jira->SetMoveInput(FVector2D(X, Y).GetSafeNormal());
	}
	Jira->TickLevelUp(DeltaTime);
	if (Jira->IsLevelUpOpen()) { Jira->SetFireHeld(false); return; }

	bool bFire = bFireInput;
	if (bUsingGamepad)
	{
		UpdateStickAim(Jira);
		// Pousser le stick droit à fond tire aussi (am > 0,75 dans le proto).
		if (StickAim.Size() > 0.75f) { bFire = true; }
	}
	else
	{
		UpdateMouseAim(Jira);
	}
	Jira->SetFireHeld(bFire);

	if (const UEONTuningSubsystem* TS = UEONTuningSubsystem::Get(this); TS && TS->GetGameFeel().DebugText) { DrawDebug(); }
}

void AEONPlayerController::ApplyPostProcess()
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	if (!T || !GetWorld()) { return; }
	const FEONGameFeelRow& F = T->GetGameFeel();
	const int32 State = (F.PPPaint ? 1 : 0) | (F.PPAll ? 2 : 0);
	if (State == PPState && PPWorld.Get() == GetWorld()) { return; }
	PPState = State;
	PPWorld = GetWorld();
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		It->bEnabled = F.PPAll;
		for (FWeightedBlendable& B : It->Settings.WeightedBlendables.Array) { B.Weight = F.PPPaint ? 1.f : 0.f; }
	}
}

void AEONPlayerController::UpdateStickAim(AEONCharacter* Jira)
{
	// Visée au stick du proto : on mémorise le dernier angle franc (stick poussé à plus de 55 %)
	// et on le garde 0,45 s après le relâchement. Sans ça, le retour du stick au centre
	// ferait tirer dans le sens de la marche.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Am = StickAim.Size() > AimDeadZone ? StickAim.Size() : 0.f;
	const float StickNow = FMath::Atan2(-StickAim.Y, StickAim.X);

	if (Am > 0.55f)
	{
		StickAngle = StickNow;
		StickAngleTime = Now;
		bHaveStickAngle = true;
	}
	const bool bHoldAim = (Now - StickAngleTime) < 0.45f;

	if (Am > 0.3f && !(bHoldAim && Am < 0.55f))
	{
		Jira->SetAimAngle(Am > 0.55f ? StickAngle : StickNow);
	}
	else if (bHoldAim && bHaveStickAngle)
	{
		Jira->SetAimAngle(StickAngle);
	}
	else if (Jira->GetMoveInput().Size() > 0.3f)
	{
		// Pas de visée : Jira regarde dans le sens de la marche.
		Jira->SetAimAngle(FMath::Atan2(-Jira->GetMoveInput().Y, Jira->GetMoveInput().X));
	}
}

void AEONPlayerController::UpdateMouseAim(AEONCharacter* Jira)
{
	FVector Origin, Dir;
	if (!DeprojectMousePositionToWorld(Origin, Dir) || FMath::IsNearlyZero(Dir.Z)) { return; }
	// Curseur projeté sur le plan horizontal à hauteur du bras.
	const FVector Arm = Jira->GetArmLocation();
	const float T = (Arm.Z - Origin.Z) / Dir.Z;
	if (T <= 0.f) { return; }
	const FVector Hit = Origin + Dir * T;
	const FVector2D D(Hit.X - Arm.X, Hit.Y - Arm.Y);
	if (!D.IsNearlyZero()) { Jira->SetAimAngle(FMath::Atan2(D.Y, D.X)); }
}

void AEONPlayerController::DrawDebug() const
{
	if (!GEngine) { return; }
	const AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn());
	const AEONPlayerCameraManager* Cam = Cast<AEONPlayerCameraManager>(PlayerCameraManager);
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	const AEONRoomBounds* Room = Jira ? AEONRoomBounds::FindRoomAt(this, Jira->GetActorLocation()) : nullptr;

	const FVector Vel = Jira ? Jira->GetVelocity() : FVector::ZeroVector;
	const FString Msg = FString::Printf(
		TEXT("EON J1 | entrée : %s | dernière action : %s\nénergie %.0f/%.0f | dash %d/%d%s%s\nvitesse %.0f px/s | visée %.0f° | vue %.1f m | salle %s | Lookahead %s px  Shake %s"),
		bUsingGamepad ? TEXT("MANETTE") : TEXT("CLAVIER/SOURIS"),
		LastAction.IsEmpty() ? TEXT("-") : *LastAction,
		Jira ? Jira->GetEnergy() : 0.f,
		Jira ? Jira->GetEnergyMax() : 0.f,
		Jira ? Jira->GetDashCharges() : 0,
		Jira ? Jira->GetDashMax() : 0,
		(Jira && Jira->IsDashing()) ? TEXT(" DASH") : TEXT(""),
		(Jira && Jira->IsRunning()) ? TEXT(" COURSE") : TEXT(""),
		EON::CmToPx(Vel.Size2D()),
		Jira ? FMath::RadiansToDegrees(Jira->GetAimAngle()) : 0.f,
		Cam ? Cam->GetViewWidthCm() / 100.f : 0.f,
		Room ? *FString::Printf(TEXT("%dx%d"), Room->SizeTiles.X, Room->SizeTiles.Y) : TEXT("aucune"),
		T ? *T->GetValueAsString(TEXT("Lookahead")) : TEXT("?"),
		T ? *T->GetValueAsString(TEXT("Shake")) : TEXT("?"));
	GEngine->AddOnScreenDebugMessage(9001, 0.f, FColor(62, 242, 224), Msg);
}

void AEONPlayerController::Interact()
{
	if (AEONFloorManager* F = AEONFloorManager::Get(this)) { F->TryInteract(); }
}

namespace
{
	FAutoConsoleCommandWithWorld GEONCmdInteragir(TEXT("eon.interagir"), TEXT("eon.interagir : comme la touche Interagir (tests sans clavier)"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W)
		{
			if (AEONPlayerController* PC = W ? Cast<AEONPlayerController>(W->GetFirstPlayerController()) : nullptr) { PC->Interact(); }
		}));
}
