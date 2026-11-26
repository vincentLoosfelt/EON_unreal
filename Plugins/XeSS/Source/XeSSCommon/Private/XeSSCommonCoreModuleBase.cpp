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

#include "XeSSCommonCoreModuleBase.h"

#include "XeSSCommonSDKVersion.h"

 // NOTE: keep following order for Unreal 4.27 and below
#include "RHI.h"
#include "RHIResources.h"
#include "DynamicRHI.h"

#include "HAL/ConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/EngineVersion.h"
#include "Misc/Parse.h"
#include "Misc/StringBuilder.h"
#include "XeSSCommonUtil.h"
#include "XeSSCommonModuleBase.h"
#include "XeSSLogger.h"
#include "XeSSUnrealCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSCommonCoreModuleBase, Log, All);

#define XESSCOMMON_UNSUPPORTED_ENGINE_VERSION_MESSAGE	TEXT("%s not supported with Engine version: %s, please check any Engine source patch available and applied")
#define XESSCOMMON_UNSUPPORTED_RHI_MESSAGE				TEXT("%s not supported on RHI: %s")
#define XESSCOMMON_UNSUPPORTED_IN_EDITOR_MESSAGE		TEXT("%s not supported in Editor, please try Standalone Game")
#define XESSCOMMON_UNSUPPORTED_DEFAULT_MESSAGE			TEXT("%s not supported, please check log for details")

FXeSSCommonCoreModuleBase::FXeSSCommonCoreModuleBase()
{
	static uint32 NextCoreModuleID = XeSSCommon::ON_SCREEN_MESSAGE_KEY_GROUP_DEFAULT + 1;

	CoreModuleID = NextCoreModuleID++;
}

FXeSSCommonCoreModuleBase::~FXeSSCommonCoreModuleBase()
{
}

bool FXeSSCommonCoreModuleBase::IsFeatureSupported() const
{
	return GetSupportedConsoleVariable()->GetBool();
}

void FXeSSCommonCoreModuleBase::OnDisabledCommandlineOption()
{
	GetModuleDisabledConsoleVariable()->Set(true, ECVF_SetByCommandline);
}

void FXeSSCommonCoreModuleBase::OnStartupModule()
{
	// Core module console variables MUST be defined and available (they SHOULD be defined in the same module)
	check(GetEnabledConsoleVariable());
	check(GetLogLevelConsoleVariable());
	check(GetModuleDisabledConsoleVariable());
	check(GetSupportedConsoleVariable());
	check(GetVersionConsoleVariable());

	bool bInitSucceeded = false;
	FString RHIName;

	HandleLogLevelCommandLineOption();

	// Log DLL file versions (only once for all modules)
	{
		static bool bDllVersionsLogged = false;
		if (!bDllVersionsLogged)
		{
			bDllVersionsLogged = true;
			const TCHAR* DllFiles[] = { TEXT("libxess.dll"), TEXT("libxess_dx11.dll"), TEXT("libxess_fg.dll"), TEXT("libxell.dll") };
			for (const TCHAR* DllFile : DllFiles)
			{
				FString DllVersion;
				if (XeSSCommon::GetDllFileVersion(DllFile, DllVersion))
				{
					UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("Local XeSS SDK DLL file version: %s - %s"), DllFile, *DllVersion);
				}
				else
				{
					UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("Local XeSS SDK DLL file version: %s - Not available or file not found"), DllFile);
				}
			}
		}
	}

	// Check if RHI is supported
	if (!bSkipInit && IsUnsupportedRHI(RHIName))
	{
		bSkipInit = true;
		UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("%s not supported on RHI: %s, skip initialization of module %s"), GetFeatureName(), *RHIName, GetModuleName());
	}

	// Check if Editor supported
	if (!bSkipInit && IsInEditorAndUnsupported())
	{
		bSkipInit = true;
		UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("%s not supported in Editor, skip initialization of module %s"), GetFeatureName(), GetModuleName());
	}

	// Initialize module and record whether Init() succeeded
	if (!bSkipInit)
	{
		UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("Initializing module %s..."), GetModuleName());
		bInitSucceeded = Init();
	}

	GetSupportedConsoleVariable()->Set(bInitSucceeded);
	UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("Set %s to %s"), *GetSupportedConsoleVariableName(), XeSSCommon::ToString(bInitSucceeded));

	if (!bInitSucceeded)
	{
		IConsoleVariable* CVarEnabled = GetEnabledConsoleVariable();
		CVarEnabled->OnChangedDelegate().Add(FConsoleVariableDelegate::CreateRaw(this, &FXeSSCommonCoreModuleBase::HandleEnabledConsoleVariableSetIfUnsupported));
		// Handle value set by ini file to log and show screen tips of unsupported info
		HandleEnabledConsoleVariableSetIfUnsupported(CVarEnabled);
		return;
	}

	// Get SDK version here, as XeSS-SR SDK requires different library file (loaded in Init()) depending on the RHI.
	{
		XeSSCommon::FSDKVersion SDKVersion{};
		XeSSCommon::PushThirdPartyDllDirectory();
		int Result = GetSDKVersion(SDKVersion);
		XeSSCommon::PopThirdPartyDllDirectory();
		if (Result != 0)
		{
			UE_LOG(LogXeSSCommonCoreModuleBase, Error, TEXT("Failed to get version of %s SDK, result: %d"), GetFeatureName(), Result);
			return;
		}
		GetVersionConsoleVariable()->Set(*SDKVersion.ToString());
		XESS_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("%s SDK version: %s, vendor: %s, RHI: %s"), GetFeatureName(), *SDKVersion.ToString(), RHIVendorIdToString(), GDynamicRHI->GetName());
	}
	XeSSUnreal::GetOnPostEngineInit().AddRaw(this, &FXeSSCommonCoreModuleBase::OnPostEngineInit);
	UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("Initialized module %s successfully"), GetModuleName());
}

void FXeSSCommonCoreModuleBase::OnShutdownModule()
{
	GetEnabledConsoleVariable()->OnChangedDelegate().RemoveAll(this);
	XeSSUnreal::GetOnPostEngineInit().RemoveAll(this);
	SafeDeinit();
}

void FXeSSCommonCoreModuleBase::SafeDeinit()
{
	if (!bSkipInit)
	{
		if (!bDeinited)
		{
			UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("Deinitializing module %s..."), GetModuleName());
			Deinit();
			bDeinited = true;
		}
	}
}

void FXeSSCommonCoreModuleBase::HandleEnabledConsoleVariableSetIfUnsupported(IConsoleVariable* ConsoleVariable)
{
	check(GetEnabledConsoleVariable() == ConsoleVariable);
	check(!IsFeatureSupported());

	bool bEnabled = ConsoleVariable->GetBool();
	FString RHIName;

	if (!IsEngineVersionSupported())
	{
		FString EngineVersion = FEngineVersion::Current().ToString(EVersionComponent::Patch);
		if (bEnabled)
		{
			UE_LOG(LogXeSSCommonCoreModuleBase, Warning, XESSCOMMON_UNSUPPORTED_ENGINE_VERSION_MESSAGE, GetFeatureName(), *EngineVersion);
		}
		XeSSCommon::UpdateMessageOnScreen(FString::Printf(XESSCOMMON_UNSUPPORTED_ENGINE_VERSION_MESSAGE, GetFeatureName(), *EngineVersion),
			XeSSCommon::ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_ENGINE_VERSION, bEnabled, CoreModuleID);
		return;
	}
	if (IsUnsupportedRHI(RHIName))
	{
		if (bEnabled)
		{
			UE_LOG(LogXeSSCommonCoreModuleBase, Warning, XESSCOMMON_UNSUPPORTED_RHI_MESSAGE, GetFeatureName(), *RHIName);
		}
		XeSSCommon::UpdateMessageOnScreen(FString::Printf(XESSCOMMON_UNSUPPORTED_RHI_MESSAGE, GetFeatureName(), *RHIName),
			XeSSCommon::ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_RHI, bEnabled, CoreModuleID);
		return;
	}
	if (IsInEditorAndUnsupported())
	{
		if (bEnabled)
		{
			UE_LOG(LogXeSSCommonCoreModuleBase, Warning, XESSCOMMON_UNSUPPORTED_IN_EDITOR_MESSAGE, GetFeatureName());
		}
		XeSSCommon::UpdateMessageOnScreen(FString::Printf(XESSCOMMON_UNSUPPORTED_IN_EDITOR_MESSAGE, GetFeatureName()),
			XeSSCommon::ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_IN_EDITOR, bEnabled, CoreModuleID);
		return;
	}
	if (bEnabled)
	{
		UE_LOG(LogXeSSCommonCoreModuleBase, Warning, XESSCOMMON_UNSUPPORTED_DEFAULT_MESSAGE, GetFeatureName());
	}
	XeSSCommon::UpdateMessageOnScreen(FString::Printf(XESSCOMMON_UNSUPPORTED_DEFAULT_MESSAGE, GetFeatureName()),
		XeSSCommon::ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_DEFAULT, bEnabled, CoreModuleID);
}

void FXeSSCommonCoreModuleBase::HandleLogLevelCommandLineOption()
{
	int32 SDKLogLevel = 0;
	if (FParse::Value(FCommandLine::Get(), *FString::Format(TEXT("{0}LogLevel="), { GetFeatureName() }), SDKLogLevel))
	{
		UE_LOG(LogXeSSCommonCoreModuleBase, Display, TEXT("%s SDK log level set by command line option, value: %d"), GetFeatureName(), SDKLogLevel);
		GetLogLevelConsoleVariable()->Set(SDKLogLevel, ECVF_SetByCommandline);
	}
}

bool FXeSSCommonCoreModuleBase::IsUnsupportedRHI(FString& OutCurrentRHIName) const
{
	check(GDynamicRHI);

	OutCurrentRHIName = GDynamicRHI->GetName();
	return !IsRHISupported(OutCurrentRHIName);
}

bool FXeSSCommonCoreModuleBase::IsInEditorAndUnsupported() const
{
#if WITH_EDITOR
	return GIsEditor && !IsEditorSupported();
#else
	return false;
#endif
}

IConsoleVariable* FXeSSCommonCoreModuleBase::GetEnabledConsoleVariable() const
{
	return IConsoleManager::Get().FindConsoleVariable(*GetEnabledConsoleVariableName());
}

IConsoleVariable* FXeSSCommonCoreModuleBase::GetLogLevelConsoleVariable() const
{
	return IConsoleManager::Get().FindConsoleVariable(*GetLogLevelConsoleVariableName());
}

IConsoleVariable* FXeSSCommonCoreModuleBase::GetModuleDisabledConsoleVariable() const
{
	return IConsoleManager::Get().FindConsoleVariable(*GetModuleDisabledConsoleVariableName());
}

IConsoleVariable* FXeSSCommonCoreModuleBase::GetSupportedConsoleVariable() const
{
	return IConsoleManager::Get().FindConsoleVariable(*GetSupportedConsoleVariableName());
}

IConsoleVariable* FXeSSCommonCoreModuleBase::GetVersionConsoleVariable() const
{
	return IConsoleManager::Get().FindConsoleVariable(*GetVersionConsoleVariableName());
}
