#include "EONPlayerCameraManager.h"
#include "EONCharacter.h"
#include "EONRoomBounds.h"
#include "EONBoss.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"
#include "EONFloorManager.h"
#include "EONPlayerController.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

namespace
{
	/** camClamp du proto : centre si la salle est plus petite que la vue, sinon borne. */
	float CamClampCenter(float C, float Min, float Max, float Half)
	{
		const float Size = Max - Min;
		if (Size <= Half * 2.f) { return (Min + Max) * 0.5f; }
		return FMath::Clamp(C, Min + Half, Max - Half);
	}
}

void AEONPlayerCameraManager::AddKick(FVector Direction, float Px)
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	const float CamKick = T ? T->GetGameFeel().CamKick : 1.f;
	const FVector2D D = FVector2D(Direction.X, Direction.Y).GetSafeNormal();
	Kick += D * EON::PxToCm(Px) * CamKick;
}

void AEONPlayerCameraManager::AddTrauma(float Amount)
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	const float ShakeSetting = T ? T->GetGameFeel().Shake : 1.f;
	Trauma = FMath::Min(1.f, Trauma + Amount * ShakeSetting);
}

void AEONPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(this);
	const FEONGameFeelRow Feel = T ? T->GetGameFeel() : FEONGameFeelRow();

	AActor* Target = OutVT.Target;
	if (!Target)
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	const float Dt = FMath::Min(DeltaTime, 0.1f);
	const FVector P = Target->GetActorLocation();

	// --- cible : Jira + anticipation vers la visée (×0,7 sur l'axe vertical de l'écran)
	FVector2D Focus(P.X, P.Y);
	if (const AEONCharacter* Jira = Cast<AEONCharacter>(Target))
	{
		if (!Jira->IsFallingIntoVoid())
		{
			const float La = EON::PxToCm(Feel.Lookahead);
			const float A = Jira->GetAimAngle();
			Focus += FVector2D(FMath::Cos(A) * La, FMath::Sin(A) * La * 0.7f);
		}
	}
	if (const AEONBoss* Boss = AEONBoss::GetActive(this))
	{
		const FVector Bl = Boss->IsInIntro() ? Boss->GetTankWorld() : Boss->GetActorLocation();
		Focus = Boss->IsInIntro() ? FVector2D(Bl.X, Bl.Y) : (FVector2D(P.X, P.Y) + FVector2D(Bl.X, Bl.Y)) * 0.5f;
	}

	// --- vue : largeur visible au centre, hauteur au ratio de l'écran du proto
	// Zoom à la molette : un cran = × CamZoomStep sur la largeur, borné entre CamZoomMinTiles et CamZoomMaxTiles, lissé.
	const float BaseTiles = FMath::Max(1.f, Feel.CamViewTiles);
	if (const AEONPlayerController* EPC = Cast<AEONPlayerController>(GetOwningPlayerController()))
	{
		if (!EPC->GetDevMenu().IsOpen())
		{
			const float Step = FMath::Max(1.01f, Feel.CamZoomStep);
			if (EPC->WasInputKeyJustPressed(EKeys::MouseScrollUp)) { ZoomTarget /= Step; }     // molette vers l'avant : on s'approche
			if (EPC->WasInputKeyJustPressed(EKeys::MouseScrollDown)) { ZoomTarget *= Step; }
		}
	}
	const float MinT = FMath::Clamp(Feel.CamZoomMinTiles, 1.f, BaseTiles), MaxT = FMath::Max(Feel.CamZoomMaxTiles, BaseTiles);
	ZoomTarget = FMath::Clamp(ZoomTarget, MinT / BaseTiles, MaxT / BaseTiles);
	ZoomCur = bSnap ? ZoomTarget : FMath::FInterpTo(ZoomCur, ZoomTarget, Dt, FMath::Max(0.1f, Feel.CamZoomSpeed));
	const float HalfW = BaseTiles * ZoomCur * EON::CmPerTile * 0.5f;
	// Trait des celluloïds relatif au zoom (Vincent, 24/11) : plus proche = plus épais ; bornes dans M_PP_EON_Trait (SilMin, SilMax).
	if (!RenduMPC) { RenduMPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/EON/Materials/MPC_EON_Rendu.MPC_EON_Rendu")); }
	if (RenduMPC) { UKismetMaterialLibrary::SetScalarParameterValue(this, RenduMPC, TEXT("TraitZoom"), 1.f / FMath::Max(ZoomCur, 0.01f)); }
	const float HalfH = HalfW * (EON::ProtoViewH / EON::ProtoViewW);

	// --- lissage et bornes de salle
	const float Ck = bSnap ? 1.f : FMath::Min(1.f, Dt * 7.f);
	bSnap = false;
	Center += (Focus - Center) * Ck;
	if (const AEONRoomBounds* Room = AEONRoomBounds::FindRoomAt(this, P))
	{
		const FBox2D R = Room->GetRect2D();
		Center.X = CamClampCenter(Center.X, R.Min.X, R.Max.X, HalfW);
		Center.Y = CamClampCenter(Center.Y, R.Min.Y, R.Max.Y, HalfH);
	}

	// --- kick et tremblement (amortis comme dans le proto)
	Trauma = FMath::Max(0.f, Trauma - Dt * 1.8f);
	Kick -= Kick * FMath::Min(1.f, Dt * 14.f);
	const float ShPx = Feel.Shake > 0.f ? 8.f * Trauma * Trauma : 0.f;
	const FVector2D ShakeOff(FMath::FRandRange(-1.f, 1.f) * EON::PxToCm(ShPx), FMath::FRandRange(-1.f, 1.f) * EON::PxToCm(ShPx));
	const FVector2D Look = Center + Kick + ShakeOff;

	// --- placement : plongée fixe, écran vers le haut = -Y (comme le proto)
	const float Fov = FMath::Clamp(Feel.CamFov, 5.f, 120.f);
	const float Pitch = FMath::Clamp(Feel.CamPitch, 20.f, 89.f);
	const float Dist = HalfW / FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f));
	const float PitchRad = FMath::DegreesToRadians(Pitch);
	// Descente du C.O.M : la caméra ne suit pas Jira verticalement.
	const AEONFloorManager* FM = AEONFloorManager::Get(this);
	const float CamDz = FM ? FM->GetCamDz() : 0.f;
	const FVector LookAt(Look.X, Look.Y, P.Z - CamDz - (Target->GetSimpleCollisionHalfHeight()));
	const FVector CamLoc = LookAt + FVector(0.f, Dist * FMath::Cos(PitchRad), Dist * FMath::Sin(PitchRad));

	OutVT.POV.Location = CamLoc;
	OutVT.POV.Rotation = FRotator(-Pitch, -90.f, 0.f);
	OutVT.POV.FOV = Fov;
	if (FM && FM->bDevView) { OutVT.POV.Location = FM->DevViewLoc; OutVT.POV.Rotation = FM->DevViewRot; OutVT.POV.FOV = FM->DevViewFov; }
	OutVT.POV.ProjectionMode = ECameraProjectionMode::Perspective;
	// Irruption : le noir (seule la lampe de Jira éclaire) passe par la compensation d'exposition.
	// (le POV garde ses réglages d'une image à l'autre : on remet l'override à zéro hors du noir)
	const float Bias = FM ? FM->GetExposureBias() : 0.f;
	OutVT.POV.PostProcessBlendWeight = Bias != 0.f ? 1.f : 0.f;
	OutVT.POV.PostProcessSettings.bOverride_AutoExposureBias = Bias != 0.f;
	OutVT.POV.PostProcessSettings.AutoExposureBias = Bias;
	LastViewWidthCm = HalfW * 2.f;
}
