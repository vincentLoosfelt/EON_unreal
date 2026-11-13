#include "EONPlayerController.h"
#include "EON.h"
#include "EONCharacter.h"
#include "EONPlayerCameraManager.h"
#include "EONRoomBounds.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
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

void AEONPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (UInputMappingContext* IMC = MappingContext.LoadSynchronous())
		{
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

	// Jalon 0 : les autres actions ne font qu'afficher leur nom (preuve que l'entrée arrive).
	auto BindNamed = [this, EIC](const TSoftObjectPtr<UInputAction>& Soft, const FString& Name)
	{
		if (UInputAction* A = Soft.LoadSynchronous())
		{
			EIC->BindAction(A, ETriggerEvent::Started, this, &AEONPlayerController::OnNamedAction, Name);
		}
	};
	BindNamed(IA_Fire, TEXT("Tir"));
	BindNamed(IA_Dash, TEXT("Dash"));
	BindNamed(IA_Spell, TEXT("Onde"));
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

void AEONPlayerController::OnNamedAction(const FInputActionValue&, FString Name)
{
	LastAction = Name;
	UE_LOG(LogEON, Display, TEXT("EON_ACTION %s (%s)"), *Name, bUsingGamepad ? TEXT("manette") : TEXT("clavier"));
}

void AEONPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	AEONCharacter* Jira = Cast<AEONCharacter>(GetPawn());
	if (!Jira) { return; }

	if (bUsingGamepad)
	{
		// Stick : haut = -Y monde. La mémoire de l'angle « franc » (0,55) vient au jalon 1.
		if (StickAim.Size() > AimDeadZone)
		{
			Jira->SetAimAngle(FMath::Atan2(-StickAim.Y, StickAim.X));
		}
	}
	else
	{
		UpdateMouseAim(Jira);
	}

	if (bShowDebug) { DrawDebug(); }
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
		TEXT("EON J0 | entrée : %s | dernière action : %s\nvitesse %.0f px/s | visée %.0f° | vue %.1f m | salle %s | Lookahead %s px  Shake %s"),
		bUsingGamepad ? TEXT("MANETTE") : TEXT("CLAVIER/SOURIS"),
		LastAction.IsEmpty() ? TEXT("-") : *LastAction,
		EON::CmToPx(Vel.Size2D()),
		Jira ? FMath::RadiansToDegrees(Jira->GetAimAngle()) : 0.f,
		Cam ? Cam->GetViewWidthCm() / 100.f : 0.f,
		Room ? *FString::Printf(TEXT("%dx%d"), Room->SizeTiles.X, Room->SizeTiles.Y) : TEXT("aucune"),
		T ? *T->GetValueAsString(TEXT("Lookahead")) : TEXT("?"),
		T ? *T->GetValueAsString(TEXT("Shake")) : TEXT("?"));
	GEngine->AddOnScreenDebugMessage(9001, 0.f, FColor(62, 242, 224), Msg);
}
