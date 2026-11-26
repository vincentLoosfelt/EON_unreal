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

#include "XeFGModule.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "XeSSCommonMacros.h"
#include "XeSSCommonSDKVersion.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "XeFGBackBufferHandler.h"
#include "XeFGRHI.h"
#include "XeFGRHIModule.h"
#include "XeFGViewExtension.h"
#include "XeLLModule.h"
#include "XeSSCommonRHI.h"
#include "XeSSSDKLoader.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess_fg/xefg_swapchain.h"
#include "Windows/HideWindowsPlatformTypes.h"

static TUniquePtr<FXeFGBackBufferHandler> XeFGBackBufferHandler;
static TUniquePtr<FXeFGRHI> XeFGRHI;
static FXeLLModule* XeLLModule = nullptr;
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXeFGModule, Log, All);

static TAutoConsoleVariable<bool> CVarXeFGModuleDisabled(
	TEXT("r.XeFG.ModuleDisabled"),
	false,
	TEXT("If XeFG module(s) are disabled via command line option -XeFGDisabled."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<bool> CVarXeFGSupported(
	TEXT("r.XeFG.Supported"),
	false,
	TEXT("If XeFG is supported."),
	ECVF_ReadOnly);

// NOTE: MUST be synced with EXeFGEnabled
static TAutoConsoleVariable<int32> CVarXeFGEnabled(
	TEXT("r.XeFG.Enabled"),
	0,
	TEXT("[default: 0] Set to 1 to enable XeFG."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarXeFGLogLevel(
	TEXT("r.XeFG.LogLevel"),
	1,
	TEXT("[default: 1] Minimum log level of XeFG SDK, set it via command line -XeFGLogLevel=\n")
	TEXT(" 0: Debug\n")
	TEXT(" 1: Info\n")
	TEXT(" 2: Warning\n")
	TEXT(" 3: Error\n")
	TEXT(" Other values: Logging disabled"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<FString> CVarXeFGVersion(
	TEXT("r.XeFG.Version"),
	TEXT("Unknown"),
	TEXT("Show XeFG SDK's version."),
	ECVF_ReadOnly);

bool FXeFGModule::IsEngineVersionSupported() const
{
	return XESS_ENGINE_WITH_XEFG_API != 0;
}

bool FXeFGModule::Init()
{
#if XESS_ENGINE_WITH_XEFG_API
	check(XeSSCommon::IsNonValidationD3D12RHI());

	// Check if libxess_fg.dll can be loaded before proceeding
	if (!FXeSSSDKLoader::TryLoadDll(TEXT("libxess_fg.dll")))
	{
		UE_LOG(LogXeFGModule, Warning, TEXT("Failed to load libxess_fg.dll, XeFG will not be supported"));
		return false;
	}

	XeLLModule = &FModuleManager::LoadModuleChecked<FXeLLModule>(TEXT("XeLLCore"));
	XeFGRHI.Reset(FModuleManager::LoadModuleChecked<FXeFGRHIModule>(TEXT("XeFGRHI")).CreateXeFGRHI(*XeLLModule));
	if (!XeFGRHI)
	{
		return false;
	}
	if (!XeFGRHI->Initialize())
	{
		XeFGRHI.Reset();
		return false;
	}
	XeFGBackBufferHandler.Reset(new FXeFGBackBufferHandler(*XeFGRHI.Get()));
	return true;
#else
	return false;
#endif
}

void FXeFGModule::Deinit()
{
#if XESS_ENGINE_WITH_XEFG_API
	XeFGBackBufferHandler.Reset();
	XeFGViewExtension = nullptr;
	XeFGRHI.Reset();

	XeLLModule = nullptr;
	// Workaround for some editor exit crash, it should be OK to unload module with shutdown
	FModuleManager::Get().UnloadModule(TEXT("XeLLCore"), true);
#endif
}

int FXeFGModule::GetSDKVersion(XeSSCommon::FSDKVersion& SDKVersion)
{
#if XESS_ENGINE_WITH_XEFG_API
	static_assert(sizeof(SDKVersion) == sizeof(xefg_swapchain_version_t), "The sizes of SDKVersion and xefg_swapchain_version_t do not match");

	return xefgSwapChainGetVersion(reinterpret_cast<xefg_swapchain_version_t*>(&SDKVersion));
#else
	check(false);
	return -1;
#endif
}

void FXeFGModule::OnPostEngineInit()
{
#if XESS_ENGINE_WITH_XEFG_API
	XeFGViewExtension = FSceneViewExtensions::NewExtension<FXeFGViewExtension>(XeFGRHI.Get(), XeFGBackBufferHandler.Get());
#endif
}

FXeFGRHI* FXeFGModule::GetXeFGRHI() const
{
#if XESS_ENGINE_WITH_XEFG_API
	return XeFGRHI.Get();
#else
	return nullptr;
#endif
}

IMPLEMENT_MODULE(FXeFGModule, XeFGCore)
