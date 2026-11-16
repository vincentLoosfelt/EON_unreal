// EON — PNJ des salles d'event (cartographe, puis gamine, Ulysse, vieille dame) : placeholder en formes simples
// (un corps cylindrique et une tête sphérique, aux couleurs du sprite du proto). La logique (dialogues, PV, états) est
// dans AEONFloorManager ; cet acteur ne fait que s'afficher. Les meshes et le matériau se changent dans un Blueprint enfant.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONNpc.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class EON_API AEONNpc : public AActor
{
	GENERATED_BODY()

public:
	AEONNpc();

	/** Taille du sprite du proto (px) : hauteur des pieds au sommet, largeur du corps. */
	void Setup(float HeightPx, float WidthPx, const FLinearColor& BodyCol, const FLinearColor& HeadCol);
	/** À terre (mort, recroquevillé) : couché sur le côté. */
	void SetDown(bool bDown);
	/** Flash blanc d'un coup reçu (0,15 s). */
	void Flash(float Seconds) { FlashT = Seconds; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|PNJ") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|PNJ") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EON|PNJ") TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|PNJ") TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMID;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HeadMID;
	FLinearColor BodyC = FLinearColor::Gray, HeadC = FLinearColor::White;
	float FlashT = 0.f;
	bool bFlashOn = false;
	bool bIsDown = false;
	void ApplyColors(bool bWhite);
};
