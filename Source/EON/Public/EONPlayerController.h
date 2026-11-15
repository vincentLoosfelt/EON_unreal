// EON — contrôleur : entrées clavier/souris et manette (Enhanced Input),
// détection du dernier périphérique utilisé, visée au sol.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "EONPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AEONCharacter;
class FEONDeviceInputProcessor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEONOnInputDeviceChanged, bool, bUsingGamepad);

UCLASS(Blueprintable)
class EON_API AEONPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEONPlayerController();

	/** true = invites en icônes Xbox, false = clavier. */
	UFUNCTION(BlueprintPure, Category = "EON|Entrées")
	bool IsUsingGamepad() const { return bUsingGamepad; }

	UPROPERTY(BlueprintAssignable, Category = "EON|Entrées")
	FEONOnInputDeviceChanged OnInputDeviceChanged;

	/** Pose sur IA_Move les modificateurs des touches (ZQSD, WASD, flèches) : haut = +Y, gauche = -X. Renvoie le nombre corrigé. */
	UFUNCTION(BlueprintCallable, Category = "EON|Entrées")
	static int32 RepairMoveMappings(UInputMappingContext* IMC, UInputAction* MoveAction);

	/** Appelé par l'écouteur Slate (thread jeu). */
	void NotifyDeviceUsed(bool bGamepad);

	/** Affiche les infos de debug du jalon 0 à l'écran. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Debug")
	bool bShowDebug = true;

	// --- Assets d'entrée (créés par Saved/Scripts/eon_inputs.py)
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Move;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Aim;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Fire;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Dash;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Spell;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Holo;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Ultimate;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Interact;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Map;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_Pause;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_GreffeNext;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TSoftObjectPtr<UInputAction> IA_GreffePrev;
	UPROPERTY(EditDefaultsOnly, Category = "EON|Entrées") TArray<TSoftObjectPtr<UInputAction>> IA_GreffeSlots;

	/** Zone morte du stick de visée (proto : 0,18). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Entrées") float AimDeadZone = 0.18f;

	/** Dernière action reçue (debug). */
	UPROPERTY(BlueprintReadOnly, Category = "EON|Debug") FString LastAction;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void OnMove(const FInputActionValue& V);
	void OnMoveStop(const FInputActionValue& V);
	void OnAim(const FInputActionValue& V);
	void OnAimStop(const FInputActionValue& V);
	void OnNamedAction(const FInputActionValue& V, FString Name);
	void OnSpellStop(const FInputActionValue& V);
	void OnFireStart(const FInputActionValue& V);
	void OnFireStop(const FInputActionValue& V);
	void OnDashStart(const FInputActionValue& V);
	void OnDashStop(const FInputActionValue& V);

	void UpdateMouseAim(AEONCharacter* Jira);
	void UpdateStickAim(AEONCharacter* Jira);
	void DrawDebug() const;

	TSharedPtr<FEONDeviceInputProcessor> DeviceProcessor;
	bool bUsingGamepad = false;
	FVector2D StickAim = FVector2D::ZeroVector;
	bool bFireInput = false;
	bool bDashInput = false;
	/** Dernier angle « franc » du stick (poussé à plus de 55 %) et son heure. */
	float StickAngle = 0.f;
	float StickAngleTime = -9.f;
	bool bHaveStickAngle = false;
};
