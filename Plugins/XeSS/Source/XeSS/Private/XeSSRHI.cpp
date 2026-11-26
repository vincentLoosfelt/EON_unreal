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

#include "XeSSRHI.h"

#include <atomic>
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTLS.h"
#include "Rendering/SlateRenderer.h"
#include "XeSSCommonMacros.h"
#include "XeSSCommonRHI.h"
#include "XeSSSDKLoader.h"
#include "XeSSSDKUtil.h"
#include "XeSSSDKWrapperBase.h"
#include "XeSSUnrealRHI.h"
#include "XeSSUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSRHI, Log, All);

// Log category for XeSS SDK
DEFINE_LOG_CATEGORY_STATIC(LogXeSSSDK, Log, All);

#if STATS
DECLARE_STATS_GROUP(TEXT("XeSS"), STATGROUP_XeSS, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("XeSS Contexts"), STAT_XeSSContexts, STATGROUP_XeSS);
DECLARE_MEMORY_STAT(TEXT("XeSS Buffer Memory"), STAT_XeSSTempBufferHeapSize, STATGROUP_XeSS);
DECLARE_MEMORY_STAT(TEXT("XeSS Texture Memory"), STAT_XeSSTempTextureHeapSize, STATGROUP_XeSS);
static std::atomic<uint32_t> NumXeSSContexts = 0;
// Thread-safe global totals - multiple XeSS contexts can update these concurrently
// Each context updates its contribution atomically via fetch_add/fetch_sub
static std::atomic<uint64_t> TotalTempBufferHeapSize = 0;
static std::atomic<uint64_t> TotalTempTextureHeapSize = 0;
#endif

struct FResolutionFractionSetting
{
	float Min = 0.f;
	float Max = 0.f;
	float Optimal = 0.f;
};
static FResolutionFractionSetting ResolutionFractionSettings[XeSS::QUALITY_SETTING_COUNT] = {};
static float MinResolutionFraction = 100.f;
static float MaxResolutionFraction = 0.f;

static FString GetXeSSCurrentThreadInfo()
{
	const TCHAR* ThreadType = TEXT("Other");
	if (IsInGameThread())
	{
		ThreadType = TEXT("Game");
	}
	else if (IsInRenderingThread())
	{
		ThreadType = TEXT("Rendering");
	}
	else if (IsInRHIThread())
	{
		ThreadType = TEXT("RHI");
	}
	return FString::Printf(TEXT("thread (type: %s, id: %u)"), ThreadType, FPlatformTLS::GetCurrentThreadId());
}

static TAutoConsoleVariable<int32> CVarXeSSLogLevel(
	TEXT("r.XeSS.LogLevel"), 1,
	TEXT("[default: 1] Minimum log level of XeSS SDK, set it via command line -XeSSLogLevel=\n")
	TEXT(" 0: Debug\n")
	TEXT(" 1: Info\n")
	TEXT(" 2: Warning\n")
	TEXT(" 3: Error\n")
	TEXT(" Other values: Logging disabled"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<float> CVarXeSSOptimalScreenPercentage(
	TEXT("r.XeSS.OptimalScreenPercentage"),
	100.f,
	TEXT("Optimal screen percentage for current XeSS quality."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeSSAutoExposure(
	TEXT("r.XeSS.AutoExposure"),
	1,
	TEXT("[default: 1] Use XeSS internal auto exposure."),
	ECVF_RenderThreadSafe);

static void XeSSLogCallback(const char* Message, xess_logging_level_t InLogLevel)
{
	switch (InLogLevel)
	{
	case XESS_LOGGING_LEVEL_DEBUG:
		UE_LOG(LogXeSSSDK, Verbose, TEXT("%hs"), Message);
		break;
	case XESS_LOGGING_LEVEL_INFO:
		UE_LOG(LogXeSSSDK, Log, TEXT("%hs"), Message);
		break;
	case XESS_LOGGING_LEVEL_ERROR:
		UE_LOG(LogXeSSSDK, Error, TEXT("%hs"), Message);
		break;
	case XESS_LOGGING_LEVEL_WARNING:
	default:
		UE_LOG(LogXeSSSDK, Warning, TEXT("%hs"), Message);
		break;
	}
}

FXeSSStatePtr FXeSSRHI::CreateXeSSState(bool bPersistent)
{
	check(XeSSSDKWrapper);

	xess_context_handle_t XeSSContext = nullptr;
	xess_result_t Result = CreateXeSSContext(&XeSSContext);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to create XeSS context, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return nullptr;
	}
	UE_LOG(LogXeSSRHI, Log, TEXT("Created XeSS context (%p) on %s"), XeSSContext, *GetXeSSCurrentThreadInfo());
	FXeSSStatePtr XeSSStatePtr = MakeShared<FXeSSState, ESPMode::ThreadSafe>(XeSSContext, XeSSSDKWrapper);
	Result = BuildXeSSPipelines(XeSSContext, GetXeSSInitFlags());
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to build XeSS pipelines, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return nullptr;
	}
	int32 XeSSLogLevelValue = CVarXeSSLogLevel->GetInt();
	if (XeSSSDK::IsLogLevelValid(XeSSLogLevelValue))
	{
		Result = XeSSSDKWrapper->xessSetLoggingCallback(XeSSContext, static_cast<xess_logging_level_t>(XeSSLogLevelValue), XeSSLogCallback);
		if (XESS_FAILED(Result))
		{
			if (Result == XESS_RESULT_ERROR_NOT_IMPLEMENTED && XeSSCommon::IsNonValidationD3D11RHI())
			{
				UE_LOG(LogXeSSRHI, Display, TEXT("XeSS logging callback not implemented on RHI DX11"));
			}
			else
			{
				UE_LOG(LogXeSSRHI, Error, TEXT("Failed to set XeSS log callback, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
				return nullptr;
			}
		}
	}
	else
	{
		UE_LOG(LogXeSSRHI, Log, TEXT("r.XeSS.LogLevel is %d, skipping XeSS logging callback setup"), XeSSLogLevelValue);
	}
	if (bPersistent)
	{
		XeSSStates.Add(XeSSStatePtr);
	}
	return XeSSStatePtr;
}

FXeSSStatePtr FXeSSRHI::FindFreeXeSSState(const FXeSSInitArguments& InitArguments) const
{
	FXeSSStatePtr Result;

	// Known issue: a state that previously failed initialization can still appear compatible
	// here because FXeSSState::InitArguments is updated before RHIInitializeXeSS() runs and is
	// not reverted on failure.
	for (const FXeSSStatePtr& XeSSState : XeSSStates)
	{
		// Only reuse states referenced exclusively by the XeSSStates container. States held in
		// the per-view ownership map will have a higher reference count and are never considered
		// free, preventing context corruption in multi-view (e.g. split-screen) scenarios.
		if (XeSSState.GetSharedReferenceCount() == 1 && !XeSSState->IsInitializationRequired(InitArguments))
		{
			Result = XeSSState;
			break;
		}
	}
	return Result;
}

FXeSSStatePtr FXeSSRHI::FindStateForView(const FSceneViewStateInterface* ViewState) const
{
	const FXeSSStatePtr* Found = ViewStateToXeSSState.Find(ViewState);
	return Found ? *Found : nullptr;
}

void FXeSSRHI::AssignStateToView(const FSceneViewStateInterface* ViewState, const FXeSSStatePtr& State)
{
	ViewStateToXeSSState.Add(ViewState, State);
}

float FXeSSRHI::GetMinSupportedResolutionFraction()
{
	return MinResolutionFraction;
}

float FXeSSRHI::GetMaxSupportedResolutionFraction()
{
	return MaxResolutionFraction;
}

float FXeSSRHI::GetOptimalResolutionFraction(xess_quality_settings_t InQualitySetting)
{
	check(XeSS::IsValid(InQualitySetting));

	return ResolutionFractionSettings[XeSS::ToIndex(InQualitySetting)].Optimal;
}

uint32 FXeSSRHI::GetXeSSInitFlags()
{
	uint32 InitFlags = XESS_INIT_FLAG_HIGH_RES_MV;
	if (CVarXeSSAutoExposure->GetBool())
	{
		InitFlags |= XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE;
	}
	return InitFlags;
}

FXeSSRHI::FXeSSRHI()
{
	check(FSlateApplication::IsInitialized());
	CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	check(CVarXeSSEnabled);
	CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));
	check(CVarXeSSQuality);

	OnBackBufferReadyToPresentHandle = FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().AddLambda([this](SWindow&, XeSSUnreal::XOnBackBufferReadyParam)
	{
		static uint32 LastTickFrameNumber = 0;
		if (GFrameNumberRenderThread - LastTickFrameNumber > 0)
		{
			Tick_RenderThread(GFrameNumberRenderThread);
			LastTickFrameNumber = GFrameNumberRenderThread;
		}
	});
}

FXeSSRHI::~FXeSSRHI()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->OnBackBufferReadyToPresent().Remove(OnBackBufferReadyToPresentHandle);
	}
	ViewStateToXeSSState.Empty();
	XeSSStates.Empty();
	CVarXeSSEnabled->OnChangedDelegate().RemoveAll(this);
	CVarXeSSQuality->OnChangedDelegate().RemoveAll(this);
	FXeSSSDKLoader::Unload();
}

bool FXeSSRHI::Initialize()
{
	check(GDynamicRHI);

	const TCHAR* RHIName = GDynamicRHI->GetName();
	XeSSSDKWrapper = FXeSSSDKLoader::Load(RHIName);
	check(XeSSSDKWrapper);

	if (!XeSSSDKWrapper->IsSupported())
	{
		UE_LOG(LogXeSSRHI, Display, TEXT("XeSS not supported on RHI: %s, vendor: %s, platform: %hs"), RHIName, RHIVendorIdToString(), FPlatformProperties::IniPlatformName());
		return false;
	}
	if (!XeSSSDKWrapper->IsLoaded())
	{
		UE_LOG(LogXeSSRHI, Error, TEXT("Failed to load XeSS SDK, library file name: %s"), XeSSSDKWrapper->GetLibraryFileName());
		return false;
	}
	OnXeSSSDKLoaded();

	// Create a temporary XeSS state (context) to query XeSS information
	FXeSSStatePtr XeSSState = CreateXeSSState(false);
	if (!XeSSState)
	{
		return false;
	}

	InitResolutionFractions(XeSSState->GetXeSSContext());

	CVarXeSSQuality->OnChangedDelegate().Add(FConsoleVariableDelegate::CreateRaw(this, &FXeSSRHI::HandleXeSSQualitySet));
	// Handle value set by ini file
	HandleXeSSQualitySet(CVarXeSSQuality);

	return true;
}

void FXeSSRHI::InitResolutionFractions(xess_context_handle_t XeSSContext)
{
	for (int32 QualitySettingInt = XeSS::QUALITY_SETTING_MIN; QualitySettingInt <= XeSS::QUALITY_SETTING_MAX; ++QualitySettingInt)
	{
		xess_2d_t OutputResolution{ XeSS::OUTPUT_RESOLUTION_MAX_SIDE_LENGTH, XeSS::OUTPUT_RESOLUTION_MAX_SIDE_LENGTH };
		xess_2d_t MinInputResolution{};
		xess_2d_t MaxInputResolution{};
		xess_2d_t OptimalInputResolution{};
		xess_quality_settings_t TempQualitySetting = static_cast<xess_quality_settings_t>(QualitySettingInt);
		xess_result_t Result = XeSSSDKWrapper->xessGetOptimalInputResolution(XeSSContext, &OutputResolution, TempQualitySetting, &OptimalInputResolution, &MinInputResolution, &MaxInputResolution);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogXeSSRHI, Warning, TEXT("Failed to get XeSS optimal input resolution, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
			continue;
		}
		FResolutionFractionSetting Setting;
		Setting.Optimal = static_cast<float>(OptimalInputResolution.x) / static_cast<float>(OutputResolution.x);
		Setting.Min = static_cast<float>(MinInputResolution.x) / static_cast<float>(OutputResolution.x);
		Setting.Max = static_cast<float>(MaxInputResolution.x) / static_cast<float>(OutputResolution.x);
		if (Setting.Min < MinResolutionFraction)
		{
			MinResolutionFraction = Setting.Min;
		}
		if (Setting.Max > MaxResolutionFraction)
		{
			MaxResolutionFraction = Setting.Max;
		}
		ResolutionFractionSettings[XeSS::ToIndex(TempQualitySetting)] = Setting;
	}
}

void FXeSSRHI::Tick_RenderThread(uint32 FrameNumber)
{
	constexpr uint32 XESS_STATE_EXPIRY_FRAMES = 10;  // 10 should be long enough for something unexpected

	// Remove per-view ownership entries for states that have not been used recently.
	// This must run before the pool cleanup so that dropping the map reference reduces the
	// refcount back to 1, allowing the subsequent pool sweep to destroy the state.
	for (auto It = ViewStateToXeSSState.CreateIterator(); It; ++It)
	{
		if (FrameNumber - It.Value()->GetLastUsedFrameNumber() > XESS_STATE_EXPIRY_FRAMES)
		{
			It.RemoveCurrent();
		}
	}

	TArray<FXeSSStatePtr> XeSSStatesToBeRemoved;
	XeSSStates.RemoveAllSwap([FrameNumber, &XeSSStatesToBeRemoved, XESS_STATE_EXPIRY_FRAMES](const FXeSSStatePtr& Item) {
		if (Item.GetSharedReferenceCount() == 1 && FrameNumber - Item->GetLastUsedFrameNumber() > XESS_STATE_EXPIRY_FRAMES)
		{
			XeSSStatesToBeRemoved.Add(Item);
			return true;
		}
		return false;
	});
	if (XeSSStatesToBeRemoved.Num() > 0)
	{
		FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();
		// Flush required by D3D11, not sure if required by other RHIs
		XeSSUnreal::BlockUntilGPUIdle(CommandList);
		// It should be OK to remove in render thread
		XeSSStatesToBeRemoved.Empty();
	}

#if STATS
	SET_DWORD_STAT(STAT_XeSSContexts, NumXeSSContexts.load(std::memory_order_relaxed));
	SET_MEMORY_STAT(STAT_XeSSTempBufferHeapSize, TotalTempBufferHeapSize);
	SET_MEMORY_STAT(STAT_XeSSTempTextureHeapSize, TotalTempTextureHeapSize);
#endif
}

void FXeSSRHI::HandleXeSSQualitySet(IConsoleVariable* Variable)
{
	OptimalResolutionFraction = GetOptimalResolutionFraction(
		XeSS::ToXeSSQualitySetting(Variable->GetInt())
	);
	CVarXeSSOptimalScreenPercentage->Set(100.f * OptimalResolutionFraction);
}

FXeSSState::FXeSSState(xess_context_handle_t InXeSSContext, TSharedPtr<FXeSSSDKWrapperBase, ESPMode::ThreadSafe> InXeSSSDKWrapper)
{
	check(InXeSSContext);
	check(InXeSSSDKWrapper);
	check(IsInRenderingThread() || IsInGameThread());

	XeSSContext = InXeSSContext;
	XeSSSDKWrapper = InXeSSSDKWrapper;

	UE_LOG(LogXeSSRHI, Log, TEXT("Created XeSS state (%p) with XeSS context (%p)"), this, XeSSContext);

#if STATS
	NumXeSSContexts.fetch_add(1, std::memory_order_relaxed);
#endif
}

FXeSSState::~FXeSSState()
{
	check(XeSSContext);

	UE_LOG(LogXeSSRHI, Log, TEXT("Destroyed XeSS state (%p)"), this);

	UE_LOG(LogXeSSRHI, Log, TEXT("Destroying XeSS context (%p, output width: %d, output height: %d) on %s"), XeSSContext, InitArguments.OutputWidth, InitArguments.OutputHeight, *GetXeSSCurrentThreadInfo());
	xess_result_t Result = XeSSSDKWrapper->xessDestroyContext(XeSSContext);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeSSRHI, Warning, TEXT("Failed to destroy XeSS context (%p), result: %d, %s"), XeSSContext, Result, XeSSSDK::GetMessage(Result));
	}

#if STATS
	NumXeSSContexts.fetch_sub(1, std::memory_order_relaxed);
	TotalTempBufferHeapSize.fetch_sub(TempBufferHeapSize, std::memory_order_relaxed);
	TotalTempTextureHeapSize.fetch_sub(TempTextureHeapSize, std::memory_order_relaxed);
#endif
}

bool FXeSSState::IsInitializationRequired(const FXeSSInitArguments& InInitArguments) const
{
	return InitArguments != InInitArguments;
}

void FXeSSState::UpdateInitArguments(const FXeSSInitArguments& InInitArguments)
{
	check(InitArguments != InInitArguments);
	UE_LOG(LogXeSSRHI, Log, TEXT("Update XeSS init arguments from %s to %s"), *InitArguments.ToString(), *InInitArguments.ToString());
	InitArguments = InInitArguments;
}

void FXeSSState::Use(uint32 FrameNumber)
{
	check(IsInRenderingThread());

	LastUsedFrameNumber = FrameNumber;
}

void FXeSSState::UpdateMemoryStats(uint64 InTempBufferHeapSize, uint64 InTempTextureHeapSize)
{
#if STATS
	// Each XeSS context atomically updates its contribution to global memory totals
	// Note: The sequence of fetch_sub/fetch_add is not atomic as a whole, so global totals
	// may show transient inconsistencies if sampled between operations. This is acceptable
	// for statistics collection as Tick_RenderThread samples at frame boundaries.
	TotalTempBufferHeapSize.fetch_sub(TempBufferHeapSize, std::memory_order_relaxed);
	TotalTempTextureHeapSize.fetch_sub(TempTextureHeapSize, std::memory_order_relaxed);

	// Update stored values (safe to update non-atomically since this is per-instance and protected by render thread/RHI thread)
	TempBufferHeapSize = InTempBufferHeapSize;
	TempTextureHeapSize = InTempTextureHeapSize;

	// Add the new contribution using input parameters to avoid any dependency on non-atomic updates
	TotalTempBufferHeapSize.fetch_add(InTempBufferHeapSize, std::memory_order_relaxed);
	TotalTempTextureHeapSize.fetch_add(InTempTextureHeapSize, std::memory_order_relaxed);
#endif
}


xess_result_t FXeSSRHI::GetXeSSVersion(xess_version_t& XeSSVersion) const
{
	check(XeSSSDKWrapper);
	return XeSSSDKWrapper->xessGetVersion(&XeSSVersion);
}

void FXeSSRHI::UpdateXeSSMemoryStats(xess_context_handle_t XeSSContext, FXeSSStatePtr XeSSState, const FXeSSInitArguments& InitArguments)
{
	check(XeSSSDKWrapper);
	check(XeSSContext);
	check(XeSSState);

	xess_2d_t OutputResolution = { InitArguments.OutputWidth, InitArguments.OutputHeight };
	xess_properties_t Properties = {};

	xess_result_t Result = XeSSSDKWrapper->xessGetProperties(XeSSContext, &OutputResolution, &Properties);
	if (XESS_SUCCEEDED(Result))
	{
		XeSSState->UpdateMemoryStats(Properties.tempBufferHeapSize, Properties.tempTextureHeapSize);
		UE_LOG(LogXeSSRHI, Verbose, TEXT("XeSS Memory - TempBuffer: %.2f MB, TempTexture: %.2f MB"),
			Properties.tempBufferHeapSize / (1024.0 * 1024.0),
			Properties.tempTextureHeapSize / (1024.0 * 1024.0));
	}
	else
	{
		if (Result == XESS_RESULT_ERROR_UNSUPPORTED && XeSSCommon::IsNonValidationD3D11RHI())
		{
			UE_LOG(LogXeSSRHI, Display, TEXT("XeSS memory stats update not supported on RHI DX11"));
		}
		else
		{
			UE_LOG(LogXeSSRHI, Warning, TEXT("Failed to get XeSS properties, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		}
	}
}
