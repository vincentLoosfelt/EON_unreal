// EON — cuve de réveil du Kibo (PODS du proto, salle de réveil, 2 × 3 tuiles).
// Maillage Cuve_1 (Unity) séparé dans Blender en corps + tube de verre (blender_cuve_reveil.py), enfoncé dans le sol
// pour que le tube parte du niveau des pieds. États du proto (drawPods) :
//   scellée : liquide vert, silhouette sombre, voyant rouge qui clignote ;
//   brisée  : tube cassé (éclats sur le bord), flaque verte au pied ;
//   Jira    : liquide, Jira dedans ; cinématique de réveil (vidange, ouverture, vapeur) ;
//   ouverte : la cuve de Jira après le réveil, vide et ouverte.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONWakePod.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UENUM()
enum class EEONPodState : uint8 { Sealed, Broken, Jira, Open };

UCLASS()
class EON_API AEONWakePod : public AActor
{
	GENERATED_BODY()

public:
	AEONWakePod();

	void Init(EEONPodState InState);
	/** Cinématique de réveil (cuve de Jira) : vidange de 0,6 à 1,6 s (son hiss), ouverture à 1,7 s (son door, vapeur),
	 *  Jira sort à 2,6 s (son lift, bouffée de vapeur). Le dialogue et les secousses restent au FloorManager. */
	void StartWake();
	EEONPodState GetState() const { return State; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Corps;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Verre;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Liquide;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Tete;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Torse;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Jambes;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flaque;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lueur;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Voyant;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Eclats;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> LiquideMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FlaqueMID;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> VerreMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> VapeurMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> RougeMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> EteintMat;

	struct FPuff
	{
		TObjectPtr<UStaticMeshComponent> C;
		TObjectPtr<UMaterialInstanceDynamic> M;
		FVector V = FVector::ZeroVector;
		float Life = 0.f, Max = 1.f, S0 = 0.3f, S1 = 1.f;
	};
	TArray<FPuff> Puffs;

	EEONPodState State = EEONPodState::Sealed;
	float Time = 0.f;
	float WakeT = -1.f;
	float Phase = 0.f;
	float PuffAcc = 0.f;
	bool bRedOn = false;

	void SetDrain(float D);
	void SetOpen(float K);
	void EmitPuff(bool bBurst);
	void SetLight(const FColor& C, float K);
};
