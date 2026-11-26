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

#pragma once

#include "XeSSCommonModuleBase.h"

class IConsoleVariable;

namespace XeSSCommon
{
	struct FSDKVersion;
}

// NOTE: Loading phase of child class MUST NOT be `PostEngineInit` or later, or `OnPostEngineInit()` will never be called
class FXeSSCommonCoreModuleBase : public FXeSSCommonModuleBase
{
public:
	XESSCOMMON_API FXeSSCommonCoreModuleBase();
	XESSCOMMON_API ~FXeSSCommonCoreModuleBase();
	// NOTE: Do not call this function frequently (e.g. in Tick()), as it finds the console variable by name internally.
	// Use GetSupportedConsoleVariable() to cache the IConsoleVariable* and query it directly in performance-critical paths.
	XESSCOMMON_API bool IsFeatureSupported() const;
	XESSCOMMON_API IConsoleVariable* GetSupportedConsoleVariable() const;

private:
	IConsoleVariable* GetEnabledConsoleVariable() const;
	IConsoleVariable* GetLogLevelConsoleVariable() const;
	IConsoleVariable* GetModuleDisabledConsoleVariable() const;
	IConsoleVariable* GetVersionConsoleVariable() const;

	virtual bool IsEditorSupported() const = 0;
	virtual bool IsEngineVersionSupported() const = 0;
	virtual bool IsRHISupported(const FString& RHIName) const = 0;
	// The feature is marked supported only when startup attempts initialization and Init() succeeds;
	// if startup skips initialization (e.g. unsupported RHI or editor), the supported state is false.
	virtual bool Init() = 0;
	virtual void Deinit() = 0;
	virtual int GetSDKVersion(XeSSCommon::FSDKVersion& SDKVersion) = 0;
	virtual void OnPostEngineInit() {}
	void SafeDeinit();

	FString GetEnabledConsoleVariableName() const
	{
		return FString::Format(TEXT("r.{0}.Enabled"), { GetFeatureName() });
	}
	FString GetLogLevelConsoleVariableName() const
	{
		return FString::Format(TEXT("r.{0}.LogLevel"), { GetFeatureName() });
	}
	FString GetSupportedConsoleVariableName() const
	{
		return FString::Format(TEXT("r.{0}.Supported"), { GetFeatureName() });
	}
	FString GetModuleDisabledConsoleVariableName() const
	{
		return FString::Format(TEXT("r.{0}.ModuleDisabled"), { GetFeatureName() });
	}
	FString GetVersionConsoleVariableName() const
	{
		return FString::Format(TEXT("r.{0}.Version"), { GetFeatureName() });
	}
	XESSCOMMON_API void OnDisabledCommandlineOption() final;
	XESSCOMMON_API void OnStartupModule() final;
	XESSCOMMON_API void OnShutdownModule() final;

	void HandleEnabledConsoleVariableSetIfUnsupported(IConsoleVariable* ConsoleVariable);
	void HandleLogLevelCommandLineOption();
	bool IsUnsupportedRHI(FString& OutCurrentRHIName) const;
	bool IsInEditorAndUnsupported() const;

	bool bSkipInit = false;
	bool bDeinited = false;
	uint32 CoreModuleID = 0;
};
