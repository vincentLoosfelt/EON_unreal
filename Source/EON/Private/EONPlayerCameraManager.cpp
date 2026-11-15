#include "EONPlayerCameraManager.h"
#include "EONCharacter.h"
#include "EONRoomBounds.h"
#include "EONBoss.h"
#include "EONTuningSubsystem.h"
#include "EONTypes.h"

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
	const float HalfW = Feel.CamViewTiles * EON::CmPerTile * 0.5f;
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
	const FVector LookAt(Look.X, Look.Y, P.Z - (Target->GetSimpleCollisionHalfHeight()));
	const FVector CamLoc = LookAt + FVector(0.f, Dist * FMath::Cos(PitchRad), Dist * FMath::Sin(PitchRad));

	OutVT.POV.Location = CamLoc;
	OutVT.POV.Rotation = FRotator(-Pitch, -90.f, 0.f);
	OutVT.POV.FOV = Fov;
	OutVT.POV.ProjectionMode = ECameraProjectionMode::Perspective;
	LastViewWidthCm = HalfW * 2.f;
}
