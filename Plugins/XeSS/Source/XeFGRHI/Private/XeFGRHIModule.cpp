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

#include "XeFGRHIModule.h"

#include "XeSSCommonMacros.h"

#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "XeFGRHI.h"
#include "XeSSCommonRHI.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXeFGRHIModule, Log, All);

void FXeFGRHIModule::OnStartupModule()
{
	int32 XeFGUIMode = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeFGUIMode="), XeFGUIMode))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.UIMode"))->Set(XeFGUIMode, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG UI mode set by command line option, value: %d"), XeFGUIMode);
	}

	int32 XeFGResourceValidity = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("XeFGResourceValidity="), XeFGResourceValidity))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.ResourceValidity"))->Set(XeFGResourceValidity, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG resource validity set by command line option, value: %d"), XeFGResourceValidity);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("XeFGOverrideSwapChainDisabled")))
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.OverrideSwapChain"))->Set(false, ECVF_SetByCommandline);
		UE_LOG(LogXeFGRHIModule, Log, TEXT("XeFG override swap chain disabled by command line option"));
	}
}

FXeFGRHI* FXeFGRHIModule::CreateXeFGRHI(FXeLLModule& XeLLModule)
{
#if XESS_ENGINE_WITH_XEFG_API
	check(XeSSCommon::IsNonValidationD3D12RHI());

	return new FXeFGRHI(XeLLModule);
#else
	return nullptr;
#endif
}

IMPLEMENT_MODULE(FXeFGRHIModule, XeFGRHI)
