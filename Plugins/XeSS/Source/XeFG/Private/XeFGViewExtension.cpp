/*******************************************************************************
 * Copyright 2023 Intel Corporation
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

#include "XeFGViewExtension.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "ClearQuad.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "PostProcess/PostProcessMaterial.h"
#include "RenderGraphUtils.h"
#include "SceneRendering.h"
#include "XeFGBackBufferHandler.h"
#include "XeFGDelegates.h"
#include "XeFGModule.h"
#include "XeFGRHI.h"
#include "XeSSPrePass.h"
#include "XeSSUnrealEngine.h"
#include "XeSSUnrealRenderer.h"
#include "XeSSUnrealRHI.h"

BEGIN_SHADER_PARAMETER_STRUCT(FXeFGShaderParameters, )
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Depth)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HUDLessColor)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, Velocity)
END_SHADER_PARAMETER_STRUCT()

DECLARE_GPU_STAT_NAMED(XeFG, TEXT("XeFG"));

static TAutoConsoleVariable<int32> CVarXeFGClearSceneColorAlpha(
	TEXT("r.XeFG.ClearSceneColorAlpha"),
	1,
	TEXT("[default: 1] if clear scene color alpha value."),
	ECVF_Default);

bool FXeFGViewExtension::ShouldSkipViewForXeFG(const FSceneView& View) const
{
	// Check if the delegate is bound and if it returns true, skip XeFG operations for this view
	return FXeFGDelegates::ShouldSkipView.IsBound() && FXeFGDelegates::ShouldSkipView.Execute(View);
}

bool FXeFGViewExtension::IsActiveThisFrame_Unified(const FSceneViewExtensionContext& Context) const
{
	// Only game viewport supported
	if (Context.Viewport && Context.Viewport->GetClient() == GEngine->GameViewport)
	{
		return XeFGRHI->IsXeFGEnabled() && XeFGRHI->IsXeFGAvailable();
	}
	return false;
}

#if !(XESS_ENGINE_WITH_XEFG_API & XESS_ENGINE_WITH_XEFG_API_SVE_POST_RENDER_VIEW_RDG)
void FXeFGViewExtension::SubscribeToPostProcessingPass_Unified(EPostProcessingPass Pass, const FSceneView* InOptionalView, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::VisualizeDepthOfField)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FXeFGViewExtension::PostProcessPassAfterVisualizeDepthOfField_RenderThread));
	}
}
#endif

#if XESS_ENGINE_VERSION_EQU(5, 4)
void FXeFGViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
	// Fix for Unreal 5.4, validation error will occur without this fix.
	if (InViewFamily.RenderTarget)
	{
		if (const FTextureRHIRef& Texture = InViewFamily.RenderTarget->GetRenderTargetTexture())
		{
			FRHITextureDesc& Desc = const_cast<FRHITextureDesc&>(Texture->GetDesc());
			EnumAddFlags(Desc.Flags, ETextureCreateFlags::ShaderResource);
		}
	}
}
#endif

void FXeFGViewExtension::ProcessXeFG_RenderThread(FRDGBuilder& GraphBuilder, const FViewInfo& ViewInfo, const FXeFGProcessInputs& Inputs)
{
	if (ShouldSkipViewForXeFG(ViewInfo))
	{
		return;
	}

	XESS_RDG_EVENT_SCOPE_STAT(GraphBuilder, XeFG);

	FRDGTexture* SceneColorTexture = Inputs.SceneColorTexture;
	FRDGTexture* DepthTexture = Inputs.DepthTexture;
	FRDGTexture* VelocityTexture = Inputs.VelocityTexture;
	check(VelocityTexture);

	const FIntRect ViewRect = ViewInfo.ViewRect;
	const FIntRect SecondaryViewRect = FIntRect(FIntPoint::ZeroValue, ViewInfo.GetSecondaryViewRectSize());
	FXeFGRHI* LocalXeFGRHI = XeFGRHI;
	FXeFGRHIArguments XeFGRHIArguments = {};
	FXeFGShaderParameters* PassParameters = GraphBuilder.AllocParameters<FXeFGShaderParameters>();
	FRDGTextureDesc Desc = SceneColorTexture->Desc;
	Desc.Flags = ETEXTURE_CREATE_FLAGS(None);
	EnumAddFlags(Desc.Flags, TexCreate_ShaderResource | TexCreate_UAV);

	FRDGTexture* HUDLessSceneColor = GraphBuilder.CreateTexture(Desc, TEXT("XeFG::HUDlessColor"));
	AddCopyTexturePass(GraphBuilder, SceneColorTexture, HUDLessSceneColor);

	XeFGRHIArguments.FrameID = GFrameNumberRenderThread;
	XeFGRHIArguments.HUDLessColorRect = Inputs.HUDLessColorRect;
	XeFGRHIArguments.DepthRect = ViewRect;
	XeFGRHIArguments.JitterOffset = FVector2f(ViewInfo.TemporalJitterPixels);
	XeFGRHIArguments.ProjectionMatrix = XeSSUnreal::GetProjectionNoAAMatrix(ViewInfo.ViewMatrices);
	XeFGRHIArguments.ViewMatrix = ViewInfo.ViewMatrices.GetTranslatedViewMatrix();

	PassParameters->HUDLessColor = HUDLessSceneColor;
	// TODO(sunzhuoshi): refactor velocity pass code
	XeFGRHIArguments.VelocityRect = ViewRect;
	PassParameters->Velocity = AddXeFGFlattenVelocityPass(GraphBuilder, DepthTexture, VelocityTexture, ViewInfo);
	PassParameters->Depth = DepthTexture;
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("XeFG::FrameID=%u", XeFGRHIArguments.FrameID),
		PassParameters,
		ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass | ERDGPassFlags::NeverCull,
		// Note: mutable is required, due to XeFGRHIArguments is a lvalue inside lambda
		[LocalXeFGRHI, PassParameters, XeFGRHIArguments](FRHICommandListImmediate& RHICmdList) mutable
		{
			PassParameters->HUDLessColor->MarkResourceAsUsed();
			XeFGRHIArguments.HUDLessColorTexture = PassParameters->HUDLessColor->GetRHI();
			PassParameters->Depth->MarkResourceAsUsed();
			XeFGRHIArguments.DepthTexture = PassParameters->Depth->GetRHI();
			PassParameters->Velocity->MarkResourceAsUsed();
			XeFGRHIArguments.VelocityTexture = PassParameters->Velocity->GetRHI();

			RHICmdList.EnqueueLambda(
				[LocalXeFGRHI, XeFGRHIArguments](FRHICommandListImmediate& Cmd)
				{
					LocalXeFGRHI->TagForPresent(XeFGRHIArguments, Cmd);
				});
		}
	);

	if (XeFGRHI->IsXeFGEnabled() && CVarXeFGClearSceneColorAlpha.GetValueOnRenderThread())
	{
		auto* PassParametersClear = GraphBuilder.AllocParameters<FRenderTargetParameters>();
		PassParametersClear->RenderTargets[0] = FRenderTargetBinding(SceneColorTexture, ERenderTargetLoadAction::ENoAction);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("XeFG::Clear Scene Color Alpha"),
			PassParametersClear,
			ERDGPassFlags::Raster,
			[SecondaryViewRect](FRHICommandList& RHICmdList)
			{
				RHICmdList.SetViewport(SecondaryViewRect.Min.X, SecondaryViewRect.Min.Y, 0.0f, SecondaryViewRect.Max.X, SecondaryViewRect.Max.Y, 1.0f);
				DrawClearQuadAlpha(RHICmdList, 0.0f);
			});
	}
	XeFGBackBufferHandler->AddViewInfo(ViewInfo);
}

#if XESS_ENGINE_WITH_XEFG_API & XESS_ENGINE_WITH_XEFG_API_SVE_POST_RENDER_VIEW_RDG
void FXeFGViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	FRDGTexture* SceneColorTexture = XeSSUnreal::GetRenderTargetTexture(*InView.Family->RenderTarget, GraphBuilder);
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(InView);
	const XeSSUnreal::XSceneTextures& SceneTextures = XeSSUnreal::GetSceneTextures(ViewInfo, GraphBuilder);
	FRDGTexture* DepthTexture = XeSSUnreal::GetSceneDepthTexture(SceneTextures, GraphBuilder);
	FRDGTexture* VelocityTexture = XeSSUnreal::GetSceneVelocityTexture(SceneTextures, GraphBuilder);

	FXeFGProcessInputs Inputs;
	Inputs.SceneColorTexture = SceneColorTexture;
	Inputs.DepthTexture = DepthTexture;
	Inputs.VelocityTexture = VelocityTexture;
	Inputs.HUDLessColorRect = ViewInfo.UnscaledViewRect;  // Unscaled view rect used to match back buffer

	ProcessXeFG_RenderThread(GraphBuilder, ViewInfo, Inputs);
}
#else
FScreenPassTexture FXeFGViewExtension::PostProcessPassAfterVisualizeDepthOfField_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
{
	check(IsInRenderingThread());
	check(View.bIsViewInfo);

	FScreenPassTexture SceneColorScenePassTexture = XeSSUnreal::GetInputScreenPassTexture(InOutInputs, EPostProcessMaterialInput::SceneColor, GraphBuilder);
	FRDGTexture* SceneColorTexture = SceneColorScenePassTexture.Texture;
	const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
	const XeSSUnreal::XSceneTextures& SceneTextures = XeSSUnreal::GetSceneTextures(ViewInfo, GraphBuilder);
	FRDGTexture* DepthTexture = XeSSUnreal::GetSceneDepthTexture(SceneTextures, GraphBuilder);
	FRDGTexture* VelocityTexture = XeSSUnreal::GetInputScreenPassTexture(InOutInputs, EPostProcessMaterialInput::Velocity, GraphBuilder).Texture;
	// VelocityTexture should be always valid, FSystemTextures::GetBlackDummy(GraphBuilder) will be assigned if velocity texture of SceneTextures is null

	FXeFGProcessInputs Inputs;
	Inputs.SceneColorTexture = SceneColorTexture;
	Inputs.DepthTexture = DepthTexture;
	Inputs.VelocityTexture = VelocityTexture;
	Inputs.HUDLessColorRect = ViewInfo.UnscaledViewRect;

	ProcessXeFG_RenderThread(GraphBuilder, ViewInfo, Inputs);
	return XeSSUnreal::ReturnUntouchedSceneColorForPostProcessing(InOutInputs, GraphBuilder);
}
#endif

#endif
