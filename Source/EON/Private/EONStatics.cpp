#include "EONStatics.h"
#include "EONTuningSubsystem.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/ToonProfile.h"
#include "Materials/MaterialInstanceConstant.h"

FEONGameFeelRow UEONStatics::GetGameFeel(const UObject* WorldContext)
{
	if (const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(WorldContext))
	{
		return T->GetGameFeel();
	}
	return FEONGameFeelRow();
}

bool UEONStatics::SetMeshSectionCollision(UStaticMesh* Mesh, int32 LodIndex, int32 SectionIndex, bool bEnable, bool bUseComplexAsSimple)
{
#if WITH_EDITOR
	if (!Mesh) { return false; }
	FMeshSectionInfoMap& Map = Mesh->GetSectionInfoMap();
	if (!Map.IsValidSection(LodIndex, SectionIndex)) { return false; }
	Mesh->Modify();
	FMeshSectionInfo Info = Map.Get(LodIndex, SectionIndex);
	Info.bEnableCollision = bEnable;
	Map.Set(LodIndex, SectionIndex, Info);
	if (UBodySetup* Body = Mesh->GetBodySetup())
	{
		Body->CollisionTraceFlag = bUseComplexAsSimple ? CTF_UseComplexAsSimple : CTF_UseDefault;
	}
	Mesh->Build();
	Mesh->PostEditChange();
	Mesh->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UEONStatics::ConfigureToonProfile(UToonProfile* Profile, float Threshold, float Smooth, FLinearColor ShadowColor, float DiffuseIndirectScale, bool bRampIncludesShadow)
{
#if WITH_EDITOR
	if (!Profile) { return false; }
	FToonProfileStruct& S = Profile->Settings;
	const float A = FMath::Clamp(Threshold, 0.f, 1.f);
	const float B = FMath::Clamp(Threshold + FMath::Max(Smooth, 0.001f), 0.f, 1.f);
	for (int32 c = 0; c < 4; ++c)
	{
		FRichCurve& Diff = S.DiffuseRamp.ColorCurves[c];
		Diff.Reset();
		const float Lo = c < 3 ? ShadowColor.Component(c) : 1.f;
		Diff.AddKey(0.f, Lo); Diff.AddKey(A, Lo);
		// smoothstep : trois points intermédiaires
		for (float u : {0.25f, 0.5f, 0.75f})
		{
			const float sm = u * u * (3.f - 2.f * u);
			Diff.AddKey(FMath::Lerp(A, B, u), FMath::Lerp(Lo, 1.f, sm));
		}
		Diff.AddKey(B, 1.f); Diff.AddKey(1.f, 1.f);
		for (auto It = Diff.GetKeyHandleIterator(); It; ++It) { Diff.SetKeyInterpMode(*It, RCIM_Linear); }

		FRichCurve& Spec = S.SpecularRamp.ColorCurves[c];
		Spec.Reset(); Spec.AddKey(0.f, c < 3 ? 0.f : 1.f); Spec.AddKey(1.f, c < 3 ? 0.f : 1.f);
	}
	S.DiffuseIndirectScale = DiffuseIndirectScale;
	S.SpecularIndirectScale = 0.f;
	S.bDiffuseRampIncludeShadow = bRampIncludesShadow;
	Profile->PostEditChange();
	Profile->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UEONStatics::SetInstanceToonProfile(UMaterialInstanceConstant* Instance, UToonProfile* Profile)
{
#if WITH_EDITOR
	if (!Instance) { return false; }
	Instance->Modify();
	Instance->bOverrideToonProfile = Profile != nullptr;
	Instance->ToonProfileOverride = Profile;
	Instance->PostEditChange();
	// Le profil surchargé donne à l'instance sa propre permutation : ses usages doivent être recalculés.
	Instance->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
	Instance->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
