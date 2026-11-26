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

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "XeSSUnrealRHI.h"
#include "xess/xess.h"

class FRHICommandListBase;
class FRHITexture;
class FSceneViewStateInterface;
class FXeSSSDKWrapperBase;

struct FXeSSInitArguments
{
	uint32 OutputWidth = 0;
	uint32 OutputHeight = 0;
	int32 QualitySetting = 0;
	uint32 InitFlags = 0;
	bool operator != (const FXeSSInitArguments& Other) const
	{
		return
			OutputWidth != Other.OutputWidth ||
			OutputHeight != Other.OutputHeight ||
			QualitySetting != Other.QualitySetting ||
			InitFlags != Other.InitFlags;
	}
	FString ToString() const
	{
		return FString::Format(TEXT("OutputWidth: {0}, OutputHeight: {1}, QualitySetting: {2}, InitFlags: {3}"),
			{ OutputWidth, OutputHeight, QualitySetting, InitFlags });
	}
};

struct FXeSSExecuteArguments
{
	FRHITexture* ColorTexture = nullptr;
	FRHITexture* VelocityTexture = nullptr;
	FRHITexture* OutputTexture = nullptr;

	float JitterOffsetX = 0.0f;
	float JitterOffsetY = 0.0f;

	FIntRect SrcViewRect = FIntRect(FIntPoint::ZeroValue, FIntPoint::ZeroValue);
	FIntRect DstViewRect = FIntRect(FIntPoint::ZeroValue, FIntPoint::ZeroValue);

	uint32 bCameraCut = 0;
};

class XESSCORE_API FXeSSState
{
public:
	explicit FXeSSState(xess_context_handle_t InXeSSContext, TSharedPtr<FXeSSSDKWrapperBase, ESPMode::ThreadSafe> InXeSSSDKWrapper);
	virtual ~FXeSSState();
	xess_context_handle_t GetXeSSContext() const { return XeSSContext; }
	bool IsInitializationRequired(const FXeSSInitArguments& InInitArguments) const;
	void UpdateInitArguments(const FXeSSInitArguments& InInitArguments);
	void Use(uint32 FrameNumber);
	uint32 GetLastUsedFrameNumber() const { return LastUsedFrameNumber; }
	uint64 GetTempBufferHeapSize() const { return TempBufferHeapSize; }
	uint64 GetTempTextureHeapSize() const { return TempTextureHeapSize; }
	void UpdateMemoryStats(uint64 InTempBufferHeapSize, uint64 InTempTextureHeapSize);
private:
	xess_context_handle_t XeSSContext = nullptr;
	TSharedPtr<FXeSSSDKWrapperBase, ESPMode::ThreadSafe> XeSSSDKWrapper;
	FXeSSInitArguments InitArguments;
	uint32 LastUsedFrameNumber = 0;
	uint64 TempBufferHeapSize = 0;
	uint64 TempTextureHeapSize = 0;
};

using FXeSSStatePtr = TSharedPtr<FXeSSState, ESPMode::ThreadSafe>;

class XESSCORE_API FXeSSRHI
{
public:
	FXeSSRHI();
	virtual ~FXeSSRHI();
	bool Initialize();

	virtual const TCHAR* GetName() const = 0;
	virtual void OnXeSSSDKLoaded() = 0;
	virtual xess_result_t RHIInitializeXeSS(const FXeSSInitArguments& InArguments, xess_context_handle_t XeSSContext) = 0;
	virtual xess_result_t RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments, FRHICommandListBase& ExecutingCmdList, xess_context_handle_t XeSSContext) = 0;
	virtual xess_result_t CreateXeSSContext(xess_context_handle_t* OutXeSSContext) = 0;
	virtual xess_result_t BuildXeSSPipelines(xess_context_handle_t InXeSSContext, uint32_t InInitFlags) = 0;

	FXeSSStatePtr CreateXeSSState(bool bPersistent = true);
	FXeSSStatePtr FindFreeXeSSState(const FXeSSInitArguments& InitArguments) const;
	FXeSSStatePtr FindStateForView(const FSceneViewStateInterface* ViewState) const;
	void AssignStateToView(const FSceneViewStateInterface* ViewState, const FXeSSStatePtr& State);
	float GetMinSupportedResolutionFraction();
	float GetMaxSupportedResolutionFraction();
	float GetOptimalResolutionFraction() const { return OptimalResolutionFraction; }
	float GetOptimalResolutionFraction(xess_quality_settings_t InQualitySetting);
	uint32 GetXeSSInitFlags();
	void HandleXeSSQualitySet(IConsoleVariable* Variable);
	// It is needed due to different XeSS SDK file is loaded based on RHI
	xess_result_t GetXeSSVersion(xess_version_t& XeSSVersion) const;
	void UpdateXeSSMemoryStats(xess_context_handle_t XeSSContext, FXeSSStatePtr XeSSState, const FXeSSInitArguments& InitArguments);
protected:
	TSharedPtr<FXeSSSDKWrapperBase, ESPMode::ThreadSafe> XeSSSDKWrapper;
private:
	void InitResolutionFractions(xess_context_handle_t XeSSContext);
	void Tick_RenderThread(uint32 FrameNumber);
	IConsoleVariable* CVarXeSSEnabled = nullptr;
	IConsoleVariable* CVarXeSSQuality = nullptr;
	// Used by `r.XeSS.Enabled` console variable `OnChanged` callback
	bool bCurrentXeSSEnabled = false;
	float OptimalResolutionFraction = 1.0f;
	TArray<FXeSSStatePtr> XeSSStates;
	TMap<const FSceneViewStateInterface*, FXeSSStatePtr> ViewStateToXeSSState;
	FDelegateHandle OnBackBufferReadyToPresentHandle;
};

class IXeSSRHIModule : public IModuleInterface
{
public:
	virtual FXeSSRHI* CreateXeSSRHI() = 0;
};
