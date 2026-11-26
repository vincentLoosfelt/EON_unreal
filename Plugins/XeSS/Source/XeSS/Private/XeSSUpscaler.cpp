/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "XeSSUpscaler.h"

#include "Engine/GameViewportClient.h"

#if XESS_ENGINE_VERSION_LSS(5, 1)
#include "LegacyScreenPercentageDriver.h"
#endif

#include "LegacyScreenPercentageDriver.h"
#include "ScenePrivate.h"  // for FSceneViewState, FTemporalAAHistory
#include "XeSSHistory.h"
#include "XeSSPrePass.h"
#include "XeSSRHI.h"
#include "XeSSSDKUtil.h"
#include "XeSSSettings.h"
#include "XeSSUnrealCore.h"
#include "XeSSUnrealRenderer.h"
#include "XeSSUnrealRHI.h"
#include "XeSSCommonUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSUpscaler, Log, All);

// It SHOULD be enough, for 0.001% * 7860 (8K) = 0.0786 (pixel)
constexpr float SCREEN_PERCENTAGE_ERROR_TOLERANCE = 0.001f;
// HACK: Variables to save previous global ones
#if XESS_ENGINE_VERSION_LSS(5, 1)
ICustomStaticScreenPercentage* PreviousGCustomStaticScreenPercentage = nullptr;

	#if XESS_ENGINE_VERSION_LSS(5, 0)
const ITemporalUpscaler* PreviousGTemporalUpscaler = nullptr;
	#endif

#else
static bool HasTemporalUpscaleView(const FSceneViewFamily& ViewFamily)
{
	for (const FSceneView* View : ViewFamily.Views)
	{
		if (View && View->PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
		{
			return true;
		}
	}
	return false;
}
#endif

static TAutoConsoleVariable<int32> CVarXeSSEnabled(
	TEXT("r.XeSS.Enabled"),
	0,
	TEXT("[default: 0] Set to 1 to use XeSS instead of TAAU or any other upscaling method."),
	ECVF_RenderThreadSafe);

// QUALITY EDIT:
static TAutoConsoleVariable<int32> CVarXeSSQuality(
	TEXT("r.XeSS.Quality"),
	2,
	TEXT("[default: 2] Set XeSS quality setting\n")
	TEXT(" 0: Ultra Performance\n")
	TEXT(" 1: Performance\n")
	TEXT(" 2: Balanced\n")
	TEXT(" 3: Quality\n")
	TEXT(" 4: Ultra Quality\n")
	TEXT(" 5: Ultra Quality Plus\n")
	TEXT(" 6: Anti-Aliasing"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarXeSSPreExposure(
	TEXT("r.XeSS.Experimental.PreExposure"),
	1,
	TEXT("[default: 1] Whether to enable pre-exposure. It just unifies commands across different Unreal versions."),
	ECVF_Default);

DECLARE_GPU_STAT_NAMED(XeSS, TEXT("XeSS"));

const TCHAR* FXeSSUpscaler::DEBUG_NAME = TEXT("FXeSSUpscaler");

static void TriggerResourceTransitions(FRHICommandListImmediate& RHICmdList, TRDGBufferAccess<ERHIAccess::UAVCompute> DummyBufferAccess)
{
	XeSSUnreal::XRHIBuffer* DummyBuffer = XeSSUnreal::GetRHIBuffer(DummyBufferAccess);
	// Using the dummy structured buffer to trigger a resource transition
	XeSSUnreal::LockRHIBuffer(RHICmdList, DummyBuffer, 0, sizeof(float), EResourceLockMode::RLM_WriteOnly);
	XeSSUnreal::UnlockRHIBuffer(RHICmdList, DummyBuffer);
}

FXeSSPassParameters::FXeSSPassParameters(const FViewInfo& ViewInfo, const XeSSUnreal::XTemporalUpscalerInputs& Inputs)
	: InputViewRect(ViewInfo.ViewRect)
	, OutputViewRect(FIntPoint::ZeroValue, ViewInfo.GetSecondaryViewRectSize())
	, SceneColorTexture(XESS_UNREAL_GET_PASS_INPUTS_TEXTURE(Inputs, SceneColor))
	, SceneDepthTexture(XESS_UNREAL_GET_PASS_INPUTS_TEXTURE(Inputs, SceneDepth))
{ }

FIntPoint FXeSSPassParameters::GetOutputExtent() const
{
	check(Validate());
	check(SceneColorTexture);

	FIntPoint InputExtent = SceneColorTexture->Desc.Extent;

	check(OutputViewRect.Min == FIntPoint::ZeroValue);
	FIntPoint QuantizedPrimaryUpscaleViewSize;
	QuantizeSceneBufferSize(OutputViewRect.Size(), QuantizedPrimaryUpscaleViewSize);

	return FIntPoint(
		FMath::Max(InputExtent.X, QuantizedPrimaryUpscaleViewSize.X),
		FMath::Max(InputExtent.Y, QuantizedPrimaryUpscaleViewSize.Y));
}

bool FXeSSPassParameters::Validate() const
{
	check(OutputViewRect.Min == FIntPoint::ZeroValue);
	return true;
}

const TCHAR* FXeSSUpscaler::GetDebugName() const
{
	return FXeSSUpscaler::DEBUG_NAME;
}

bool FXeSSUpscaler::IsXeSSEnabled() const
{
	static IConsoleVariable* CVarTAAUpscaler = IConsoleManager::Get().FindConsoleVariable(TEXT("r.TemporalAA.Upscaler"));
	check(CVarTAAUpscaler);

#if ENGINE_MAJOR_VERSION < 5
	return (CVarTAAUpscaler->GetInt() != 0) && (CVarXeSSEnabled.GetValueOnAnyThread() != 0) && (GTemporalUpscaler == this);
#else
	return (CVarTAAUpscaler->GetInt() != 0) && (CVarXeSSEnabled.GetValueOnAnyThread() != 0);
#endif
}

#if XESS_ENGINE_VERSION_LSS(5, 1)
// HACK: assignment of GTemporalUpscaler and GCustomStaticScreenPercentage moved from StartupModule()
void FXeSSUpscaler::HandleXeSSEnabledSet(IConsoleVariable* Variable)
{
	// Return if no change as bool
	if (bCurrentXeSSEnabled == Variable->GetBool())
	{
		return;
	}
	bCurrentXeSSEnabled = Variable->GetBool();
	if (!bCurrentXeSSEnabled)
	{
		GCustomStaticScreenPercentage = PreviousGCustomStaticScreenPercentage;

	#if XESS_ENGINE_VERSION_LSS(5, 0)
		GTemporalUpscaler = PreviousGTemporalUpscaler;
	#endif
	}
	else
	{
		PreviousGCustomStaticScreenPercentage = GCustomStaticScreenPercentage;
		GCustomStaticScreenPercentage = this;

	#if XESS_ENGINE_VERSION_LSS(5, 0)
		PreviousGTemporalUpscaler = GTemporalUpscaler;
		GTemporalUpscaler = this;
	#endif
	}
}
#endif

BEGIN_SHADER_PARAMETER_STRUCT(FXeSSShaderParameters, )
	// Exec parameters
	RDG_TEXTURE_ACCESS(InputColor, ERHIAccess::SRVCompute)
	RDG_TEXTURE_ACCESS(InputVelocity, ERHIAccess::SRVCompute)

	// Only used as WA to force Resource Transition Barrier
	RDG_BUFFER_ACCESS(DummyBuffer, ERHIAccess::UAVCompute)

	// Output
	RDG_TEXTURE_ACCESS_DYNAMIC(SceneColorOutput)
END_SHADER_PARAMETER_STRUCT()

#if XESS_ENGINE_VERSION_GEQ(5, 3)
XeSSUnreal::XTemporalUpscaler::FOutputs FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FInputs& Inputs) const
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
XeSSUnreal::XTemporalUpscaler::FOutputs FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& ViewInfo,
	const FPassInputs& Inputs) const
#else
void FXeSSUpscaler::AddPasses(
	FRDGBuilder& GraphBuilder,
	const FViewInfo& ViewInfo,
	const FPassInputs& Inputs,
	FRDGTextureRef* OutSceneColorTexture,
	FIntRect* OutSceneColorViewRect,
	FRDGTextureRef* OutSceneColorHalfResTexture,
	FIntRect* OutSceneColorHalfResViewRect) const
#endif
{
	RDG_EVENT_SCOPE(GraphBuilder, "XeSS Pass");
	FRDGTexture* SceneVelocityTexture = XESS_UNREAL_GET_PASS_INPUTS_TEXTURE(Inputs, SceneVelocity);

	XeSSUnreal::XTemporalUpscalerOutputs Outputs{};

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	check(View.bIsViewInfo);
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
#endif

	// NOTE: XeSS does not check whether it is enabled here for engine versions 5.0 and above. There
	// the engine only calls the upscaler set by `FSceneViewFamily::SetTemporalUpscalerInterface()`,
	// which XeSS only does when it is enabled, and bailing out would return empty outputs and break
	// the rendering of the frame.
#if XESS_ENGINE_VERSION_LSS(5, 0)
	// HACK: exit if XeSS upscaler is not active, allows to have multiple upscalers loaded by project
	if (!IsXeSSEnabled())
	{
		return;
	}
#endif
	const TRefCountPtr<XeSSUnreal::XTemporalUpscalerHistory> InputCustomHistory = XeSSUnreal::GetInputCustomHistory(Inputs, ViewInfo);
	TRefCountPtr<XeSSUnreal::XTemporalUpscalerHistory>* OutputCustomHistory = XeSSUnreal::GetOutputCustomHistory(Outputs, ViewInfo);
	FXeSSPassParameters PassParameters(ViewInfo, Inputs);
	FXeSSInitArguments InitArgsXeSS;

	InitArgsXeSS.OutputWidth = PassParameters.OutputViewRect.Width();
	InitArgsXeSS.OutputHeight = PassParameters.OutputViewRect.Height();
	InitArgsXeSS.QualitySetting = CVarXeSSQuality.GetValueOnAnyThread();
	InitArgsXeSS.InitFlags = UpscalerXeSSRHI->GetXeSSInitFlags();

	FXeSSHistory* InputXeSSHistory = nullptr;
	if (InputCustomHistory.IsValid())
	{
		InputXeSSHistory = FXeSSHistory::GetXeSSHistory(InputCustomHistory.GetReference());
	}

	// Explicit per-view ownership tracking: map ViewState -> XeSSState to safely reuse the
	// correct context across camera cuts without risking state theft in multi-view scenarios.
	const FSceneViewStateInterface* ViewKey = ViewInfo.State;
	FXeSSStatePtr XeSSState;
	if (InputXeSSHistory)
	{
		// Normal frame: retrieve the state that was saved with the history last frame.
		XeSSState = InputXeSSHistory->GetXeSSState();
	}
	else if (ViewKey != nullptr)
	{
		// Camera cut or first frame for this view: reuse the state that was previously
		// assigned to this ViewState (if any) so the XeSS context is reset in-place instead
		// of being re-assigned from the pool, which could corrupt another view's context.
		XeSSState = UpscalerXeSSRHI->FindStateForView(ViewKey);
	}
	if (!XeSSState)
	{
		XeSSState = UpscalerXeSSRHI->FindFreeXeSSState(InitArgsXeSS);
	}
	if (!XeSSState)
	{
		XeSSState = UpscalerXeSSRHI->CreateXeSSState();
	}
	check(XeSSState);
	// Record ownership so subsequent camera cuts for this view reuse the same context.
	if (ViewKey != nullptr)
	{
		UpscalerXeSSRHI->AssignStateToView(ViewKey, XeSSState);
	}
	XeSSState->Use(GFrameNumberRenderThread);

	PassParameters.SceneVelocityTexture = AddXeSSFlattenVelocityPass(GraphBuilder,
		PassParameters.SceneDepthTexture,
		SceneVelocityTexture,
		ViewInfo);
	FRDGTexture* XeSSOutput = nullptr;
	{
		XESS_RDG_EVENT_SCOPE_STAT(GraphBuilder, XeSS);

		FXeSSRHI* LocalXeSSRHI = UpscalerXeSSRHI;
		const FIntPoint OutputExtent = PassParameters.GetOutputExtent();
		// Whether to use camera cut shader permutation or not.
		// Also force a camera cut when there is no XeSS history for this view (e.g. first frame,
		// camera cut, or a switch from another temporal upscaler), so the reused/pooled XeSS
		// context resets its internal state rather than continuing from stale history.
		const bool bCameraCut = !InputXeSSHistory || !ViewInfo.PrevViewInfo.TemporalAAHistory.IsValid() || ViewInfo.bCameraCut;
		const FVector2f JitterOffset = FVector2f(ViewInfo.TemporalJitterPixels);

		// Create outputs
		FRDGTextureDesc OutputColorDesc = PassParameters.SceneColorTexture->Desc;
		OutputColorDesc.Extent = OutputExtent;
		OutputColorDesc.Flags = TexCreate_ShaderResource | TexCreate_UAV;

		XeSSOutput = GraphBuilder.CreateTexture(
			OutputColorDesc,
			TEXT("XeSS::OutputSceneColor"),
			ERDGTextureFlags::MultiFrame);

		FXeSSShaderParameters* ShaderParameters = GraphBuilder.AllocParameters<FXeSSShaderParameters>();
		ShaderParameters->InputColor = PassParameters.SceneColorTexture;
		ShaderParameters->InputVelocity = PassParameters.SceneVelocityTexture;
		ShaderParameters->SceneColorOutput = FRDGTextureAccess(XeSSOutput, ERHIAccess::UAVCompute);
		ShaderParameters->DummyBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(float), 1u), TEXT("ForceTransitionDummyBuffer"));

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("XeSS::Main Pass"),
			ShaderParameters,
			ERDGPassFlags::Compute,
			[bCameraCut, InitArgsXeSS, JitterOffset, LocalXeSSRHI, PassParameters, ShaderParameters, XeSSState](FRHICommandListImmediate& RHICmdList)
			{
				FXeSSExecuteArguments ExecArgsXeSS;
				check(ShaderParameters->InputColor);
				ShaderParameters->InputColor->MarkResourceAsUsed();
				ExecArgsXeSS.ColorTexture = ShaderParameters->InputColor->GetRHI();

				check(ShaderParameters->InputVelocity);
				ShaderParameters->InputVelocity->MarkResourceAsUsed();
				ExecArgsXeSS.VelocityTexture = ShaderParameters->InputVelocity->GetRHI();

				check(ShaderParameters->SceneColorOutput);
				ShaderParameters->SceneColorOutput->MarkResourceAsUsed();
				ExecArgsXeSS.OutputTexture = ShaderParameters->SceneColorOutput->GetRHI();

				ExecArgsXeSS.JitterOffsetX = JitterOffset.X;
				ExecArgsXeSS.JitterOffsetY = JitterOffset.Y;
				ExecArgsXeSS.bCameraCut = bCameraCut;
				ExecArgsXeSS.SrcViewRect = PassParameters.InputViewRect;
				ExecArgsXeSS.DstViewRect = PassParameters.OutputViewRect;

				check(ShaderParameters->DummyBuffer);
				ShaderParameters->DummyBuffer->MarkResourceAsUsed();

				if (XeSSState->IsInitializationRequired(InitArgsXeSS))
				{
					XeSSState->UpdateInitArguments(InitArgsXeSS);
					// Invalidate history if XeSS is re-initialized
					ExecArgsXeSS.bCameraCut = 1;
					// Make sure all cmd lists in flight have complete before XeSS re-initialized
					XeSSUnreal::BlockUntilGPUIdle(RHICmdList);
					RHICmdList.EnqueueLambda([LocalXeSSRHI, InitArgsXeSS, XeSSState](FRHICommandListImmediate& Cmd)
					{
						xess_result_t Result = LocalXeSSRHI->RHIInitializeXeSS(InitArgsXeSS, XeSSState->GetXeSSContext());
						if (XESS_SUCCEEDED(Result))
						{
							LocalXeSSRHI->UpdateXeSSMemoryStats(XeSSState->GetXeSSContext(), XeSSState, InitArgsXeSS);
						}
						else
						{
							UE_LOG(LogXeSSUpscaler, Warning, TEXT("Failed to initialize XeSS, memory stats not updated, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
						}
					});
				}

				// Make sure all resource transitions barriers are executed before RHIExecuteXeSS is called
				TriggerResourceTransitions(RHICmdList, ShaderParameters->DummyBuffer);
				RHICmdList.EnqueueLambda([LocalXeSSRHI, ExecArgsXeSS, XeSSState](FRHICommandListImmediate& Cmd)
				{
					LocalXeSSRHI->RHIExecuteXeSS(ExecArgsXeSS, Cmd, XeSSState->GetXeSSContext());
				});
			}
		);
#if XESS_ENGINE_VERSION_LSS(5, 3)
		// Following code is done in Engine version 5.3 and above
		if (!ViewInfo.bStatePrevViewInfoIsReadOnly)
		{
			FTemporalAAHistory* OutputHistory = &ViewInfo.ViewState->PrevFrameViewInfo.TemporalAAHistory;
			OutputHistory->SafeRelease();
			GraphBuilder.QueueTextureExtraction(XeSSOutput, &OutputHistory->RT[0]);
			OutputHistory->ViewportRect = PassParameters.OutputViewRect;
			OutputHistory->ReferenceBufferSize = OutputExtent;
		}
#endif
	}
	check(XeSSOutput);

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	Outputs.NewHistory = new FXeSSHistory(XeSSState);
#else
	if (!ViewInfo.bStatePrevViewInfoIsReadOnly && OutputCustomHistory)
	{
		if (!OutputCustomHistory->GetReference())
		{
			*OutputCustomHistory = new FXeSSHistory(XeSSState);
		}
	}
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 0)
	Outputs.FullRes.Texture = XeSSOutput;
	Outputs.FullRes.ViewRect = PassParameters.OutputViewRect;
	return Outputs;
#else
	*OutSceneColorTexture = XeSSOutput;
	*OutSceneColorViewRect = PassParameters.OutputViewRect;
#endif
}

#if XESS_ENGINE_VERSION_LSS(5, 1)
// Inherited via ICustomStaticScreenPercentage
void FXeSSUpscaler::SetupMainGameViewFamily(FSceneViewFamily& ViewFamily)
{
	if (!IsXeSSEnabled())
	{
		return;
	}
	#if XESS_ENGINE_VERSION_LSS(5, 0)
	checkf(GTemporalUpscaler == this, TEXT("GTemporalUpscaler is not set to XeSS, please make sure no other upscaling plugin is enabled."));
	#endif
	checkf(GCustomStaticScreenPercentage == this, TEXT("GCustomStaticScreenPercentage is not set to XeSS, please make sure no other upscaling plugin is enabled."));

	if (!GIsEditor || GetDefault<UXeSSSettings>()->bEnableXeSSInEditorViewports)
	{
		// NOTE: Do not check for temporal-upscale views here.
		// On engine versions < 5.1, UGameViewportClient::Draw() calls
		// ICustomStaticScreenPercentage::SetupMainGameViewFamily() before
		// LocalPlayer->CalcSceneView() populates ViewFamily.Views, so the view list is
		// always empty at this point.

		ViewFamily.SetTemporalUpscalerInterface(this);

		if (ViewFamily.EngineShowFlags.ScreenPercentage && !ViewFamily.GetScreenPercentageInterface())
		{
			const float ResolutionFraction = UpscalerXeSSRHI->GetOptimalResolutionFraction();
			ViewFamily.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(
				ViewFamily, ResolutionFraction
	#if XESS_ENGINE_VERSION_LSS(5, 0)
				, /*InAllowPostProcessSettingsScreenPercentage*/ false
	#endif
			));
		}
	}
}

#endif

float FXeSSUpscaler::GetMinUpsampleResolutionFraction() const
{
	return UpscalerXeSSRHI->GetMinSupportedResolutionFraction();
}

float FXeSSUpscaler::GetMaxUpsampleResolutionFraction() const
{
	return UpscalerXeSSRHI->GetMaxSupportedResolutionFraction();
}

#if XESS_ENGINE_VERSION_GEQ(5, 1)
XeSSUnreal::XTemporalUpscaler* FXeSSUpscaler::Fork_GameThread(const FSceneViewFamily& ViewFamily) const
{
	return new FXeSSUpscaler(FXeSSUpscaler::UpscalerXeSSRHI);
}
void FXeSSUpscaler::SetupViewFamily(FSceneViewFamily& ViewFamily)
{
	ViewFamily.SetTemporalUpscalerInterface(new FXeSSUpscaler(UpscalerXeSSRHI));
}
#endif

FXeSSRHI* FXeSSUpscaler::UpscalerXeSSRHI;

FXeSSUpscaler::FXeSSUpscaler(FXeSSRHI* InXeSSRHI)
{
	UpscalerXeSSRHI = InXeSSRHI;

#if XESS_ENGINE_VERSION_LSS(5, 1)
	// NOTE: OnChangedCallback will always be called when set even if the value is not changed
	CVarXeSSEnabled->OnChangedDelegate().Add(FConsoleVariableDelegate::CreateRaw(this, &FXeSSUpscaler::HandleXeSSEnabledSet));
	// Handle value set by ini file
	HandleXeSSEnabledSet(CVarXeSSEnabled->AsVariable());
#endif

	CVarXeSSPreExposure->OnChangedDelegate().AddLambda([this](IConsoleVariable* InVariable)
	{
		IConsoleVariable* CVarEyeAdaptationPreExposureOverride = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.PreExposureOverride"));

		if (InVariable->GetBool())
		{
			CVarEyeAdaptationPreExposureOverride->Set(0.f);
		}
		else
		{
			CVarEyeAdaptationPreExposureOverride->Set(1.f);
		}
#if XESS_ENGINE_VERSION_LSS(5, 0)
		IConsoleVariable* CVarUsePreExposure = IConsoleManager::Get().FindConsoleVariable(TEXT("r.UsePreExposure"));
		if (InVariable->GetBool())
		{
			CVarUsePreExposure->Set(1);
		}
		else
		{
			CVarUsePreExposure->Set(0);
		}
#endif
	});
}

FXeSSUpscaler::~FXeSSUpscaler()
{
#if XESS_ENGINE_VERSION_LSS(5, 1)
	CVarXeSSEnabled->OnChangedDelegate().RemoveAll(this);
#endif
}

#if XESS_ENGINE_VERSION_GEQ(5, 1)
bool FXeSSUpscalerViewExtension::IsActiveThisFrame_Unified(const FSceneViewExtensionContext& Context) const
{
	bool IsXeSSEnabled = XeSSUpscaler->IsXeSSEnabled();

	if (Context.Viewport == nullptr || !GEngine)
	{
		return false;
	}
	#if WITH_EDITOR
	if (GIsEditor)
	{
		if (!IsXeSSEnabled || !GetDefault<UXeSSSettings>()->bEnableXeSSInEditorViewports)
		{
			return false;
		}
		if (Context.Viewport->IsPlayInEditorViewport())
		{
			if (!GIsPlayInEditorWorld)
			{
				return false;
			}
			return true;
		}
		else
		{
			// Editor viewports not supported right now
			return false;
		}
	}
	else
	#endif
	{
		// Game viewport
		if (Context.Viewport->GetClient() == GEngine->GameViewport)
		{
			if (IsXeSSEnabled)
			{
				float MinUpsampleScreenPercentage = XeSSUpscaler->GetMinUpsampleResolutionFraction() * 100.f;
				float MaxUpsampleScreenPercentage = XeSSUpscaler->GetMaxUpsampleResolutionFraction() * 100.f;
				// It also handles cases where r.ScreenPercentage <= 0
				float CurrentScreenPercentage = FLegacyScreenPercentageDriver::GetCVarResolutionFraction() * 100.f;
				bool bValidScreenPercentage = CurrentScreenPercentage >= MinUpsampleScreenPercentage && CurrentScreenPercentage <= MaxUpsampleScreenPercentage
					|| FMath::IsNearlyEqual(CurrentScreenPercentage, MinUpsampleScreenPercentage, SCREEN_PERCENTAGE_ERROR_TOLERANCE)
					|| FMath::IsNearlyEqual(CurrentScreenPercentage, MaxUpsampleScreenPercentage, SCREEN_PERCENTAGE_ERROR_TOLERANCE);

				XeSSCommon::UpdateMessageOnScreen(
					FString::Printf(TEXT("XeSS is off due to invalid screen percentage: %.3f, valid range: %.3f - %.3f"),
						CurrentScreenPercentage, MinUpsampleScreenPercentage, MaxUpsampleScreenPercentage),
					XeSSCommon::ON_SCREEN_MESSAGE_KEY_INCORRECT_SCREEN_PERCENTAGE, !bValidScreenPercentage
				);
				return bValidScreenPercentage;
			}
		}
		return false;
	}
}

void FXeSSUpscalerViewExtension::BeginRenderViewFamily(FSceneViewFamily& ViewFamily)
{
	if (!ViewFamily.bRealtimeUpdate ||
		!ViewFamily.EngineShowFlags.AntiAliasing ||
		!ViewFamily.EngineShowFlags.ScreenPercentage ||
		ViewFamily.ViewMode != EViewModeIndex::VMI_Lit)
	{
		return;
	}

	// Skip XeSS view-family setup if no view requires temporal upscale
	if (!HasTemporalUpscaleView(ViewFamily))
	{
		return;
	}

	if (!ViewFamily.GetTemporalUpscalerInterface())
	{
		check(XeSSUpscaler);
		XeSSUpscaler->SetupViewFamily(ViewFamily);
	}
}
#endif
