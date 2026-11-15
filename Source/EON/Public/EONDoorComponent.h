// EON — porte posée à la main dans une salle dessinée (Blueprint enfant d'AEONTileRoom).
// La flèche donne l'orientation : vers le haut de l'écran (−Y) = porte nord, vers le bas (+Y) = sud, +X = est, −X = ouest.
//  - Extérieure : sortie vers la case voisine de l'étage, calée sur le bord vers lequel pointe la flèche, à l'endroit où
//    elle est posée le long de ce bord. 4 au maximum, une par côté. Une porte sans voisin dans l'étage devient un mur.
//  - Intérieure : simple obstacle ouvrable (Interagir), en travers du passage indiqué par la flèche, sur Largeur tuiles.
// La flèche n'apparaît que dans l'éditeur.
#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "EONDoorComponent.generated.h"

UCLASS(ClassGroup = (EON), meta = (BlueprintSpawnableComponent, DisplayName = "Porte EON"))
class EON_API UEONDoorComponent : public UArrowComponent
{
	GENERATED_BODY()

public:
	UEONDoorComponent();

	/** Coché : sortie de la salle vers la case voisine. Décoché : porte intérieure, obstacle qu'on ouvre avec Interagir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EON|Porte") bool bExterieure = true;
	/** Porte intérieure : largeur en tuiles (1 tuile = 1 m). Les portes extérieures font toujours 2 tuiles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EON|Porte", meta = (ClampMin = "1", ClampMax = "8", EditCondition = "!bExterieure")) int32 Largeur = 2;

	/** Direction (masque EONDoor N/S/E/W) d'après un lacet relatif à la salle. */
	static uint8 DirFromYaw(float Yaw);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& E) override;
#endif
	virtual void OnRegister() override;

private:
	void ApplyLook();
};
