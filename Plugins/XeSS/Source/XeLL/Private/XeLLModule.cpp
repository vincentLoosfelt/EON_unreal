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

#include "XeLLModule.h"

#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "XeSSCommonSDKVersion.h"
#include "XeSSUnrealCore.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "XeSSCommonUtil.h"
#include "XeSSSDKLoader.h"
#include "XeSSSDKUtil.h"
#include "XeSSUnrealD3D12RHI.h"
#include "XeSSUnrealRHI.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xell/xell.h"
#include "xell/xell_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"

static xell_context_handle_t XeLLContext = nullptr;
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXeLLModule, Log, All);

DEFINE_LOG_CATEGORY_STATIC(LogXeLLSDK, Log, All);

static TAutoConsoleVariable<bool> CVarXeLLModuleDisabled(
	TEXT("r.XeLL.ModuleDisabled"),
	false,
	TEXT("If XeLL module(s) are disabled via command line option -XeLLDisabled."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<bool> CVarXeLLSupported(
	TEXT("r.XeLL.Supported"),
	false,
	TEXT("If XeLL is supported."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeLLEnabled(
	TEXT("r.XeLL.Enabled"),
	0,
	TEXT("0: off, 1: low latency enabled"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarXeLLLogLevel(
	TEXT("r.XeLL.LogLevel"), 1,
	TEXT("[default: 1] Minimum log level of XeLL SDK, set it via command line -XeLLLogLevel=\n")
	TEXT(" 0: Debug\n")
	TEXT(" 1: Info\n")
	TEXT(" 2: Warning\n")
	TEXT(" 3: Error\n")
	TEXT(" Other values: Logging disabled"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<FString> CVarXeLLVersion(
	TEXT("r.XeLL.Version"),
	TEXT("Unknown"),
	TEXT("Show XeLL SDK's version."),
	ECVF_ReadOnly);

#if XESS_ENGINE_WITH_XEFG_API
static void XeLLLogCallback(const char* Message, xell_logging_level_t InLogLevel)
{
	switch (InLogLevel)
	{
	case XELL_LOGGING_LEVEL_DEBUG:
		UE_LOG(LogXeLLSDK, Verbose, TEXT("%hs"), Message);
		break;
	case XELL_LOGGING_LEVEL_INFO:
		UE_LOG(LogXeLLSDK, Log, TEXT("%hs"), Message);
		break;
	case XELL_LOGGING_LEVEL_ERROR:
		UE_LOG(LogXeLLSDK, Error, TEXT("%hs"), Message);
		break;
	case XELL_LOGGING_LEVEL_WARNING:
	default:
		UE_LOG(LogXeLLSDK, Warning, TEXT("%hs"), Message);
		break;
	}
}
#endif

bool FXeLLModule::IsEngineVersionSupported() const
{
	// XeLL uses the same Engine version check as XeFG to make it simple.
	return XESS_ENGINE_WITH_XEFG_API != 0;
}

bool FXeLLModule::Init()
{
#if XESS_ENGINE_WITH_XEFG_API
	// Check if libxell.dll can be loaded before proceeding
	if (!FXeSSSDKLoader::TryLoadDll(TEXT("libxell.dll")))
	{
		UE_LOG(LogXeLLModule, Warning, TEXT("Failed to load libxell.dll, XeLL will not be supported"));
		return false;
	}

	XeLLContext = nullptr;
	ObservedMaxTickRateHandlers.Empty();
	ObservedLatencyMarkers.Empty();
	XeSSCommon::PushThirdPartyDllDirectory();
	xell_result_t Result = xellD3D12CreateContext(XeSSUnreal::D3D12RHIGetDevice(), &XeLLContext);
	XeSSCommon::PopThirdPartyDllDirectory();
	if (XESS_FAILED(Result))
	{
		XeLLContext = nullptr;
		UE_LOG(LogXeLLModule, Error, TEXT("Failed to create XeLL, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return false;
	}

	UE_LOG(LogXeLLModule, Log, TEXT("Successfully created XeLL!"));

	int32 XeLLLogLevelValue = CVarXeLLLogLevel->GetInt();
	if (XeSSSDK::IsLogLevelValid(XeLLLogLevelValue))
	{
		Result = xellSetLoggingCallback(XeLLContext, static_cast<xell_logging_level_t>(XeLLLogLevelValue), XeLLLogCallback);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogXeLLModule, Error, TEXT("Failed to set XeLL log callback, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
			return false;
		}
	}
	else
	{
		UE_LOG(LogXeLLModule, Log, TEXT("r.XeLL.LogLevel is %d, skipping XeLL logging callback setup"), XeLLLogLevelValue);
	}
	return true;
#else
	return false;
#endif
}

void FXeLLModule::Deinit()
{
#if XESS_ENGINE_WITH_XEFG_API
	if (XeLLContext)
	{
		xellDestroyContext(XeLLContext);
		XeLLContext = nullptr;
	}

	if (XeLLMaxTickRateHandler.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(XeLLMaxTickRateHandler->GetModularFeatureName(), XeLLMaxTickRateHandler.Get());
	}

	if (XeLLLatencyMarker.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(XeLLLatencyMarker->GetModularFeatureName(), XeLLLatencyMarker.Get());
	}
#endif
}

int FXeLLModule::GetSDKVersion(XeSSCommon::FSDKVersion& SDKVersion)
{
#if XESS_ENGINE_WITH_XEFG_API
	static_assert(sizeof(SDKVersion) == sizeof(xell_version_t), "The sizes of SDKVersion and xell_version_t do not match");

	return xellGetVersion(reinterpret_cast<xell_version_t*>(&SDKVersion));
#else
	check(false);
	return -1;
#endif
}

void FXeLLModule::OnPostEngineInit()
{
#if XESS_ENGINE_WITH_XEFG_API
	XeLLMaxTickRateHandler = MakeUnique<FXeLLMaxTickRateHandler>(XeLLContext);
	XeLLMaxTickRateHandler->Initialize();
	IModularFeatures::Get().RegisterModularFeature(XeLLMaxTickRateHandler->GetModularFeatureName(), XeLLMaxTickRateHandler.Get());

	XeLLLatencyMarker = MakeUnique<FXeLLLatencyMarkers>(XeLLContext);
	XeLLLatencyMarker->Initialize();
	IModularFeatures::Get().RegisterModularFeature(XeLLLatencyMarker->GetModularFeatureName(), XeLLLatencyMarker.Get());
#endif
}

bool FXeLLModule::Tick(float DeltaTime)
{
#if XESS_ENGINE_WITH_XEFG_API
	static IConsoleVariable* CVarXeFGMode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.Enabled"));
	int32 newXeLLMode = CVarXeLLEnabled.GetValueOnGameThread();
	if (newXeLLMode != XeLLMode)
	{
		if (newXeLLMode != 0 && newXeLLMode != 1)
		{
			XeSSUnreal::SetWithCurrentPriority(*CVarXeLLEnabled, XeLLMode);
			UE_LOG(LogXeLLModule, Warning, TEXT("Unsupported r.XeLL.Enabled value: %d"), newXeLLMode);
			return true;
		}

		static IConsoleVariable* CVarSupported = GetSupportedConsoleVariable();
		if (CVarSupported->GetBool())
		{
			if (CVarXeFGMode && CVarXeFGMode->GetInt() && newXeLLMode == 0)
			{
				// If XeFG is ON, can't set XeLL to OFF
				XeSSUnreal::SetWithCurrentPriority(*CVarXeLLEnabled, XeLLMode);
				UE_LOG(LogXeLLModule, Warning, TEXT("Cannot close XeLL while XeFG is on."));
				return true;
			}
			SetXeLLMode(newXeLLMode);
			XeLLMode = newXeLLMode;
		}
	}
#endif
	return true;
}

void* FXeLLModule::GetXeLLContext()
{
#if XESS_ENGINE_WITH_XEFG_API
	return XeLLContext;
#else
	return nullptr;
#endif
}

XELLCORE_API bool FXeLLModule::OnPreSetXeFGEnabled()
{
#if XESS_ENGINE_WITH_XEFG_API
	check(IsInGameThread());  // Console variables can only be set in game thread
	check(IsFeatureSupported());

	{
		xell_sleep_params_t Params{};
		xell_result_t Result = xellGetSleepMode(XeLLContext, &Params);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogXeLLModule, Warning, TEXT("Failed to get XeLL sleep mode, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
			return false;
		}
		if (Params.bLowLatencyMode != 1)
		{
			Params.bLowLatencyMode = 1;
			Result = xellSetSleepMode(XeLLContext, &Params);
			if (XESS_FAILED(Result))
			{
				UE_LOG(LogXeLLModule, Warning, TEXT("Failed to set XeLL sleep mode, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
				return false;
			}
		}
	}
	if (0 == CVarXeLLEnabled->GetInt())
	{
		constexpr int32 XELL_ENABLED_BY_XEFG = 1;
		XeSSUnreal::SetWithCurrentPriority(*CVarXeLLEnabled, XELL_ENABLED_BY_XEFG);
		// TODO(sunzhuoshi): unset it to align with XeSS and XeFG
		UE_LOG(LogXeLLModule, Log, TEXT("r.XeLL.Enabled set by XeFG, value: %d"), XELL_ENABLED_BY_XEFG);
	}
	return true;
#else
	return false;
#endif
}

XELLCORE_API bool FXeLLModule::IsXeLLAvailable()
{
#if XESS_ENGINE_WITH_XEFG_API
	bool bIsMaxTickRateHandlerEnabled = false;

	TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
	for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
	{
		// check if the max tick rate handler available
		if (MaxTickRateHandler && MaxTickRateHandler == GetMaxTickRateHandler())
		{
			bIsMaxTickRateHandlerEnabled = MaxTickRateHandler->GetAvailable();
			break;
		}
	}

	bool bIsLatencyMarkerModuleEnabled = false;

	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
		.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		// check if the latency marker available
		if (LatencyMarkerModule && LatencyMarkerModule == GetLatencyMarker())
		{
			bIsLatencyMarkerModuleEnabled = LatencyMarkerModule->GetAvailable();
			break;
		}
	}

	return bIsMaxTickRateHandlerEnabled && bIsLatencyMarkerModuleEnabled;
#else
	return false;
#endif
}

void FXeLLModule::SetXeLLMode(const uint32 Flags)
{
#if XESS_ENGINE_WITH_XEFG_API
	if (Flags != 0 && Flags != 1)
	{
		UE_LOG(LogXeLLModule, Warning, TEXT("Please set the right value for XeLL mode, 0: off, 1: low latency enabled"));
		return;
	}

	// clear reflex settings (especially for ReflexStatsLatencyMarker) when enable XeLL
	if (Flags != 0)
	{
		// clear reflex settings
		TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
		for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
		{
			if (MaxTickRateHandler != XeLLMaxTickRateHandler.Get() && MaxTickRateHandler->GetEnabled())
			{
				FMaxTickRateHandlerData Data;
				Data.Handler = MaxTickRateHandler;
				Data.Flag = MaxTickRateHandler->GetFlags();
				ObservedMaxTickRateHandlers.Add(Data);
			}
			MaxTickRateHandler->SetEnabled(false);
			MaxTickRateHandler->SetFlags(0);
		}

		TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
		for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
		{
			if (LatencyMarkerModule != XeLLLatencyMarker.Get() && LatencyMarkerModule->GetEnabled())
			{
				ObservedLatencyMarkers.Add(LatencyMarkerModule);
			}
			LatencyMarkerModule->SetEnabled(false);
		}
	}
	else
	{
		// restore other settings
		// compare for safety
		TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
		for (IMaxTickRateHandlerModule* MaxTickRateHandler : MaxTickRateHandlerModules)
		{
			for (const FMaxTickRateHandlerData& Data : ObservedMaxTickRateHandlers)
			{
				if (Data.Handler == MaxTickRateHandler)
				{
					MaxTickRateHandler->SetEnabled(true);
					MaxTickRateHandler->SetFlags(Data.Flag);
					break;
				}
			}
		}

		TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get()
			.GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
		for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
		{
			for (ILatencyMarkerModule* LatencyMarker : ObservedLatencyMarkers)
			{
				if (LatencyMarker == LatencyMarkerModule)
				{
					LatencyMarkerModule->SetEnabled(true);
					break;
				}
			}
		}

		ObservedMaxTickRateHandlers.Empty();
		ObservedLatencyMarkers.Empty();
	}

	// set XeLL mode
	if (GetMaxTickRateHandler())
	{
		GetMaxTickRateHandler()->SetEnabled(Flags != 0);
		GetMaxTickRateHandler()->SetFlags(Flags);
	}
	if (GetLatencyMarker())
	{
		GetLatencyMarker()->SetEnabled(true);
	}
#endif
}

IMPLEMENT_MODULE(FXeLLModule, XeLLCore)
