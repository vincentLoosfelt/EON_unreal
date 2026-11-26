/*******************************************************************************
 * Copyright 2025 Intel Corporation
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

#include "XeSSCommonModuleBase.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "XeSSLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSCommonModuleBase, Log, All);

static void LogPluginInfo()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");

	XESS_LOG(LogXeSSCommonModuleBase, Display, TEXT("XeSS plugin version: %u, version name: %s"), Plugin->GetDescriptor().Version, *Plugin->GetDescriptor().VersionName);
}

void FXeSSCommonModuleBase::StartupModule()
{
	static bool bLogged = false;
	if (!bLogged)
	{
		// Initialize the XeSS logger for Shipping configuration if enabled via command line
		if (FParse::Param(FCommandLine::Get(), TEXT("XeSSLogEnabled")))
		{
			XeSSCommon::FXeSSLogger::Initialize();
		}

		LogPluginInfo();
		bLogged = true;
	}

	if (FParse::Param(FCommandLine::Get(), *GetDisabledCommandlineOption()))
	{
		UE_LOG(LogXeSSCommonModuleBase, Log, TEXT("%s disabled by command line option, skip initialization of module %s"), GetFeatureName(), GetModuleName());
		OnDisabledCommandlineOption();
		return;
	}

	if (::IsRunningCommandlet() && !IsEnabledInRunningCommandlet())
	{
		UE_LOG(LogXeSSCommonModuleBase, Display, TEXT("Module %s cannot run in running commandlet, skip initialization"), GetModuleName());
		return;
	}

	bModuleEnabled = true;
	UE_LOG(LogXeSSCommonModuleBase, Display, TEXT("Starting up module %s..."), GetModuleName());
	OnStartupModule();
}

void FXeSSCommonModuleBase::ShutdownModule()
{
	if (bModuleEnabled)
	{
		UE_LOG(LogXeSSCommonModuleBase, Display, TEXT("Shutting down module %s..."), GetModuleName());
		OnShutdownModule();
	}
}

FString FXeSSCommonModuleBase::GetDisabledCommandlineOption()
{
	return FString::Format(TEXT("{0}Disabled"), { GetFeatureName() });
}

