/*******************************************************************************
 * Copyright 2024 Intel Corporation
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

#include "XeSSCommonMacros.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "Misc/ConfigUtilities.h"
#else
#include "Misc/ConfigCacheIni.h"
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 0)
#include "SceneTextures.h"
#else
#include "Misc/CommandLine.h"
#include "PostProcess/SceneRenderTargets.h"
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "PostProcess/PostProcessMaterialInputs.h"
#else
#include "PostProcess/PostProcessMaterial.h"
#include "ScenePrivate.h"  // For FSceneViewState
#endif
#if XESS_ENGINE_VERSION_GEQ(5, 8)
#include "Slate/SlateViewportProvider.h"
#endif
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "Rendering/SlateRenderer.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "SceneRendering.h"
#include "ScreenPass.h"
#include "SystemTextures.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"
#include "XeSSUnrealCore.h"
#include "XeSSUnrealEngine.h"
#include "XeSSUnrealRenderer.h"
#include "XeSSUnrealRHI.h"
#include "XeSSUnrealSlateCore.h"

namespace XeSSUnreal
{

void ApplyCVarSettingsFromIni(const TCHAR* InSectionBaseName, const TCHAR* InIniFilename, uint32 SetBy, bool bAllowCheating)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	UE::ConfigUtilities::ApplyCVarSettingsFromIni(InSectionBaseName, InIniFilename, SetBy, bAllowCheating);
#else
	::ApplyCVarSettingsFromIni(InSectionBaseName, InIniFilename, SetBy, bAllowCheating);
#endif
}

XRHIBuffer* GetRHIBuffer(TRDGBufferAccess<ERHIAccess::UAVCompute> BufferAccess)
{
#if ENGINE_MAJOR_VERSION >= 5
	return BufferAccess->GetRHI();
#else
	return BufferAccess->GetRHIStructuredBuffer();
#endif
}

void* LockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
#if ENGINE_MAJOR_VERSION >= 5
	return CommandList.LockBuffer(Buffer, Offset, SizeRHI, LockMode);
#else
	return CommandList.LockStructuredBuffer(Buffer, Offset, SizeRHI, LockMode);
#endif
}

void UnlockRHIBuffer(FRHICommandListImmediate& CommandList, XRHIBuffer* Buffer)
{
#if ENGINE_MAJOR_VERSION >= 5
	CommandList.UnlockBuffer(Buffer);
#else
	CommandList.UnlockStructuredBuffer(Buffer);
#endif
}

XTextureRHIRef GetBackBufferResource(XOnBackBufferReadyParam Param)
{
#if XESS_ENGINE_VERSION_GEQ(5, 8)
	return Param.GetBackBufferResource();
#else
	return Param;
#endif
}

const XSceneTextures& GetSceneTextures(const FViewInfo& ViewInfo, FRDGBuilder& GraphBuilder)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return ViewInfo.GetSceneTextures();
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
	return FSceneTextures::Get(GraphBuilder);
#else
	return FSceneRenderTargets::Get(GraphBuilder.RHICmdList);
#endif
}

FRDGTexture* GetSceneDepthTexture(const XSceneTextures& SceneTextures, FRDGBuilder& GraphBuilder)
{
#if ENGINE_MAJOR_VERSION >= 5
	return SceneTextures.Depth.Resolve;
#else
	return GraphBuilder.RegisterExternalTexture(SceneTextures.SceneDepthZ);
#endif
}

FRDGTexture* GetSceneVelocityTexture(const XSceneTextures& SceneTextures, FRDGBuilder& GraphBuilder)
{
#if ENGINE_MAJOR_VERSION >= 5
	if (!SceneTextures.Velocity->HasBeenProduced())
	{
		return GSystemTextures.GetBlackDummy(GraphBuilder);
	}
	return SceneTextures.Velocity;
#else
	TRefCountPtr<IPooledRenderTarget> SceneVelocity = SceneTextures.SceneVelocity;
	if (!SceneVelocity.IsValid())
	{
		SceneVelocity = GSystemTextures.BlackDummy;
	}
	return GraphBuilder.RegisterExternalTexture(SceneVelocity, TEXT("SceneVelocity"));
#endif
}

FScreenPassTexture ReturnUntouchedSceneColorForPostProcessing(const FPostProcessMaterialInputs& Inputs, FRDGBuilder& GraphBuilder)
{
#if XESS_ENGINE_VERSION_GEQ(5, 4)
	return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
#else
	if (Inputs.OverrideOutput.IsValid())
	{
		return Inputs.OverrideOutput;
	}
	else
	{
		return FScreenPassTexture(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
	}
#endif
}

FScreenPassTexture GetInputScreenPassTexture(const FPostProcessMaterialInputs& Inputs, EPostProcessMaterialInput Input, FRDGBuilder& GraphBuilder)
{
#if XESS_ENGINE_VERSION_GEQ(5, 4)
	return FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(Input));
#else
	return Inputs.GetInput(Input);
#endif
}

TRefCountPtr<XTemporalUpscalerHistory> GetInputCustomHistory(const XTemporalUpscalerInputs& Inputs, const FViewInfo& ViewInfo)
{
#if XESS_ENGINE_VERSION_GEQ(5, 3)
	return Inputs.PrevHistory;
#else
	return ViewInfo.PrevViewInfo.CustomTemporalAAHistory;
#endif
}

TRefCountPtr<XTemporalUpscalerHistory>* GetOutputCustomHistory(XTemporalUpscalerOutputs& Outputs, const FViewInfo& ViewInfo)
{
#if XESS_ENGINE_VERSION_GEQ(5, 3)
	return &Outputs.NewHistory;
#else
	return ViewInfo.ViewState ? &(ViewInfo.ViewState->PrevFrameViewInfo.CustomTemporalAAHistory) : nullptr;
#endif
}

FRDGTexture* GetRenderTargetTexture(const FRenderTarget& RenderTarget, FRDGBuilder& GraphBuilder)
{
#if ENGINE_MAJOR_VERSION >= 5
	return RenderTarget.GetRenderTargetTexture(GraphBuilder);
#else
	FRHITexture* Texture = RenderTarget.GetRenderTargetTexture();
	if (FRDGTexture* FoundTexture = GraphBuilder.FindExternalTexture(Texture))
	{
		return FoundTexture;
	}
	return GraphBuilder.RegisterExternalTexture(CreateRenderTarget(Texture, TEXT("RenderTarget")));
#endif
}

FRHIViewport* GetViewportResource(FSlateRenderer& SlateRenderer, SWindow& SlateWindow)
{
	void* ViewportResource = SlateRenderer.GetViewportResource(SlateWindow);
	if (!ViewportResource)
	{
		return nullptr;
	}
#if XESS_ENGINE_VERSION_GEQ(5, 8)
	return static_cast<FRHIViewport*>(ViewportResource);
#else
	return static_cast<FViewportRHIRef*>(ViewportResource)->GetReference();
#endif
}

}
