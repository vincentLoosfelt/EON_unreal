// EON — tirs ennemis (ebullets) et œufs d'Ubume (eggs) du proto, jalon 3.
// Ni l'un ni l'autre ne bouge seul : AEONCombatDirector appelle Step() dans l'ordre de ses listes,
// comme la boucle update() du proto. Le gameplay reste en px salle ; l'acteur ne fait que suivre.
// Visuel : sphère M_Flat (placeholder, MANQUANTS.md §2).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EONEnemyProjectile.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AEONTileRoom;

/** Balle ennemie : x, y en px « écran » du proto (y = sol − hauteur), vx, vy, life, big. */
UCLASS(NotBlueprintable)
class EON_API AEONEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEONEnemyProjectile();

	/** ScreenPx = position du proto (y déjà remontée de HeightPx) ; HeightPx = hauteur d'affichage au-dessus du sol. */
	void Init(AEONTileRoom* InRoom, const FVector2D& InScreenPx, const FVector2D& InVelPx, float InLife, bool bInBig, float InHeightPx);
	/** Un pas : avance, vieillit. Vrai = à retirer (vie écoulée ou mur, testé en y écran comme le proto). */
	bool Step(float Dt);

	FVector2D Pos = FVector2D::ZeroVector;   // px écran du proto
	FVector2D Vel = FVector2D::ZeroVector;
	float Life = 3.f;
	bool bBig = false;
	float HeightPx = 9.f;

private:
	void Place();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Mid;
	TWeakObjectPtr<AEONTileRoom> Room;
};

/** Œuf d'Ubume : vole de la ponte au point visé (0,55 s), puis éclot en petite Nuke (gouleP) à 2 s. */
UCLASS(NotBlueprintable)
class EON_API AEONEnemyEgg : public AActor
{
	GENERATED_BODY()

public:
	AEONEnemyEgg();

	void Init(AEONTileRoom* InRoom, const FVector2D& InFromScreenPx, const FVector2D& InToPx);
	/** Vrai = l'œuf a fini (éclos ou perdu). bHatch dit s'il faut faire naître une gouleP en To. */
	bool Step(float Dt, bool& bHatch);

	FVector2D From = FVector2D::ZeroVector;
	FVector2D To = FVector2D::ZeroVector;
	float T = 0.f;
	static constexpr float FlyTime = 0.55f;
	static constexpr float HatchTime = 2.0f;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Mid;
	TWeakObjectPtr<AEONTileRoom> Room;
};
