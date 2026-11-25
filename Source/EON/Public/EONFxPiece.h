// EON — effets en maillage (Vincent, 24/11 : coder les effets « ABS » du PDF des effets spéciaux).
// Un AEONFxPiece est un petit acteur jetable qui vit Life secondes et s'efface : plans lumineux à plat (M_EON_Glow,
// toutes les formes : flash de bouche, annonces au sol, flaques), lumière ponctuelle qui décroît, fantôme de Jira
// (copie de sa pose) ou douille qui tombe et rebondit. Les fonctions d'appel sont dans EONFx.h.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONFxPiece.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UPoseableMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(NotBlueprintable, Transient)
class EON_API AEONFxPiece : public AActor
{
	GENERATED_BODY()

public:
	AEONFxPiece();
	virtual void Tick(float Dt) override;

	/** Plan lumineux à plat (plan moteur 100 × 100 cm), centré en Loc (relatif), tourné de Yaw, de taille en cm. */
	UStaticMeshComponent* AddPlane(UMaterialInstanceDynamic* Mid, const FVector& Loc, float Yaw, float SizeXCm, float SizeYCm);
	/** bShadows : ombres nettes (source ponctuelle), pour le flash des explosions. */
	UPointLightComponent* AddLight(const FLinearColor& Col, float Intensity, float RadiusCm, bool bShadows = false);
	UPoseableMeshComponent* AddGhost(class USkeletalMeshComponent* Src, UMaterialInstanceDynamic* Mid);
	/** Maillage (boule de charge, M_EON_Dome) : grandit de Grow0 à Grow1 (échelle relative) sur GrowT s. */
	UStaticMeshComponent* AddMesh(class UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Scale);

	float Life = 0.2f;
	float Age = 0.f;
	/** Opacité de départ ; l'opacité décroît linéairement jusqu'à 0 sur Life (sauf bNoFade). */
	float Opacity0 = 1.f;
	bool bNoFade = false;
	/** Annonces : Remplissage de 0 à 1 sur Life (cercle, cône, ligne qui se remplissent). */
	bool bFill = false;
	/** Animé à 12 i/s (Vincent, 24/11 : effets « en 2 » comme les persos) : l'âge visible avance par bonds de 1/12 s. */
	bool bPas12 = false;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Grow;
	FVector Grow0 = FVector::OneVector, Grow1 = FVector::OneVector;
	float GrowT = 0.15f;
	/** Appelé à la fin de la vie, juste avant la destruction (la boule de charge lance l'explosion). */
	TFunction<void(AEONFxPiece*)> OnEnd;

	/** Douille : vitesse en cm/s, sol en Z monde. Reste ensuite RestLife s, s'efface la dernière seconde. */
	bool bShell = false;
	FVector ShellVel = FVector::ZeroVector;
	float FloorZ = 0.f;
	bool bRest = false;
	float RestLife = 4.f;
	/** Douille à 12 i/s (Vincent, 25/11 : celluloïd « en 2 » comme le reste) : la chute est calculée à chaque image
	 *  dans ShellPos/ShellYaw, mais la douille visible ne bouge que par bonds de 1/12 s. */
	FVector ShellPos = FVector::ZeroVector;
	float ShellYaw = 0.f;
	float ShellAcc = 1.f / 12.f;

	/** Planche (M_EON_Planche) : joue FlipN images sur Life (paramètre Image). */
	bool bFlip = false;
	int32 FlipN = 8;
	/** N'apparaît qu'après Delay s (tache au sol posée à l'arrivée des gouttes). */
	float Delay = 0.f;
	/** Tache au sol : opacité pleine, puis érosion la dernière seconde. */
	bool bTacheSol = false;
	/** Plan tourné vers la caméra, son axe +X (sens de l'image) aligné sur FaceDir projeté à l'écran. L'ancre (point
	 *  d'impact) est à FaceAnchor × FaceSize de son centre, le long de -X ; FaceLift cm vers la caméra (devant le corps). */
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> FacePlane;
	FVector FaceDir = FVector::ForwardVector;
	float FaceSize = 100.f;
	float FaceAnchor = 0.f;
	float FaceLift = 0.f;
	void UpdateFace();

	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Mids;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
	float Light0 = 0.f;

	/** Douilles en jeu (le proto en garde 160 au plus ; ici 80). */
	static TArray<TWeakObjectPtr<AEONFxPiece>> Shells;

private:
	void TickShell(float Dt);
};
