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

#include "XeSSModule.h"

#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Windows/WindowsHWrapper.h"
#include "XeSSCommonMacros.h"
#include "XeSSCommonSDKVersion.h"
#include "XeSSCommonUtil.h"
#include "XeSSRHI.h"
#include "XeSSSDKLoader.h"
#include "XeSSUpscaler.h"
#include "XeSSUtil.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSModule, Log, All);

#define XESS_UNSUPPORTED_RHI_MESSAGE TEXT("RHI not supported by XeSS, RHI: %s")

static TAutoConsoleVariable<bool> CVarXeSSModuleDisabled(
	TEXT("r.XeSS.ModuleDisabled"),
	false,
	TEXT("If XeSS module(s) are disabled via command line option -XeSSDisabled."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<bool> CVarXeSSSupported(
	TEXT("r.XeSS.Supported"),
	false,
	TEXT("If XeSS is supported."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<FString> CVarXeSSVersion(
	TEXT("r.XeSS.Version"),
	TEXT("Unknown"),
	TEXT("Version of XeSS SDK."),
	ECVF_ReadOnly);

static TUniquePtr<FXeSSUpscaler> XeSSUpscaler;
static TUniquePtr<FXeSSRHI> XeSSRHI;
#if XESS_ENGINE_VERSION_GEQ(5, 1)
static TSharedPtr<FXeSSUpscalerViewExtension, ESPMode::ThreadSafe> XeSSUpscalerViewExtension;
#endif

bool FXeSSModule::IsEngineVersionSupported() const
{
	return XESS_ENGINE_VERSION_GEQ(4, 26);
}

bool FXeSSModule::Init()
{
	IXeSSRHIModule& XeSSRHIModule = FModuleManager::LoadModuleChecked<IXeSSRHIModule>(*FString::Format(TEXT("XeSS{0}RHI"), { GDynamicRHI->GetName() }));
	XeSSRHI.Reset(XeSSRHIModule.CreateXeSSRHI());
	check(XeSSRHI);

	if (!XeSSRHI->Initialize())
	{
		XeSSRHI.Reset();
		return false;
	}
	XeSSUpscaler.Reset(new FXeSSUpscaler(XeSSRHI.Get()));
	return true;
}

void FXeSSModule::Deinit()
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	XeSSUpscalerViewExtension = nullptr;
#else
	// Restore default screen percentage driver and upscaler
	GCustomStaticScreenPercentage = nullptr;

	#if XESS_ENGINE_VERSION_LSS(5, 0)
	GTemporalUpscaler = ITemporalUpscaler::GetDefaultTemporalUpscaler();
	#endif

#endif

	XeSSRHI.Reset();
	XeSSUpscaler.Reset();
}

int FXeSSModule::GetSDKVersion(XeSSCommon::FSDKVersion& SDKVersion)
{
	static_assert(sizeof(SDKVersion) == sizeof(xess_version_t), "The sizes of SDKVersion and xess_version_t do not match");
	check(XeSSRHI);

	return XeSSRHI->GetXeSSVersion(reinterpret_cast<xess_version_t&>(SDKVersion));
}

void FXeSSModule::OnPostEngineInit()
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	check(XeSSUpscaler);
	XeSSUpscalerViewExtension = FSceneViewExtensions::NewExtension<FXeSSUpscalerViewExtension>(XeSSUpscaler.Get());
#endif
}

FXeSSRHI* FXeSSModule::GetXeSSRHI() const
{
	return XeSSRHI.Get();
}

FXeSSUpscaler* FXeSSModule::GetXeSSUpscaler() const
{
	return XeSSUpscaler.Get();
}

IMPLEMENT_MODULE(FXeSSModule, XeSSCore)
