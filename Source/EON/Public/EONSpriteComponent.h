// EON — placeholder pixel en billboard (07_ASSETS_3D.md §6).
// Prend un nom de sprite (DT_Sprites, issu de PLACEHOLDERS_PIXEL/_index.json),
// charge /Game/EON/Placeholders/T_<nom>, se dimensionne à 1 px = 6,25 cm,
// pose le bas du sprite aux pieds et se tourne toujours vers la caméra.
// Le gameplay ne dépend jamais de ce composant : on le remplace par le vrai mesh sans rien changer.
#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "EONSpriteComponent.generated.h"

class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (EON), meta = (BlueprintSpawnableComponent))
class EON_API UEONSpriteComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UEONSpriteComponent();

	/** Nom du sprite (ex. jira_a0, goule, boss). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Sprite")
	FName SpriteName;

	/** Frame affichée (0..N-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Sprite")
	int32 Frame = 0;

	/** Les sprites regardent à droite : true = retourné (regarde à gauche). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Sprite")
	bool bFlipX = false;

	/** Facteur d'affichage du proto (ex. 1,75 pour le Tentaculaire). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Sprite")
	float DisplayScale = 1.f;

	/** Animation automatique entre les frames (images/s, 0 = fixe). Proto : 4 à 6 en déplacement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Sprite")
	float AnimFps = 0.f;

	/** Teinte (élites, Cobaye…). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EON|Sprite")
	FLinearColor Tint = FLinearColor::White;

	/** Recharge le sprite après un changement de nom. */
	UFUNCTION(BlueprintCallable, Category = "EON|Sprite")
	void RefreshSprite();

	UFUNCTION(BlueprintPure, Category = "EON|Sprite")
	FVector2D GetSizeCm() const { return SizeCm; }

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& E) override;
#endif

private:
	void ApplyParams();
	void FaceCamera(const FVector& CamForward, const FVector& CamUp);

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Mid;
	FVector2D SizeCm = FVector2D(100.f, 100.f);
	int32 Frames = 1;
	FVector FootLocal = FVector::ZeroVector;
	bool bFootCaptured = false;
	float AnimTime = 0.f;
};
