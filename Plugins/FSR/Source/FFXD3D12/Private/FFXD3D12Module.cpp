// This file is part of the FSR Upscaling Unreal Engine Plugin.
//
// Copyright (c) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "FFXD3D12Module.h"
#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 5, 0)
#define UE_STRINGIZE(x) PREPROCESSOR_TO_STRING(x)
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogFFXD3D12Module, Verbose, All);
DEFINE_LOG_CATEGORY(LogFFXD3D12Module);

IMPLEMENT_MODULE(FFXD3D12, FFXD3D12)

//-------------------------------------------------------------------------------------
// Directory containing the FFX SDK's runtime DLLs. FFXD3D12.Build.cs stages them here via
// RuntimeDependencies and defines FSR_FFXSDK_BIN_PATH to match.
//-------------------------------------------------------------------------------------
static const TCHAR* GetSdkDllDirectory()
{
	static FString SDKDllDirectory = FPaths::Combine(IPluginManager::Get().FindPlugin("FSR")->GetBaseDir(), TEXT(UE_STRINGIZE(FSR_FFXSDK_BIN_PATH)));
	return *SDKDllDirectory;
}

void FFXD3D12::StartupModule()
{
	FPlatformProcess::PushDllDirectory(GetSdkDllDirectory());
	// These handles can't be free'd until the game fully exits, so the FreeDllHandle calls are omitted on purpose.
	auto LoadSdkDll = [](const TCHAR* DllName)
	{
		void* Handle = FPlatformProcess::GetDllHandle(DllName);
		if (!Handle)
		{
			uint32 ErrorCode = FPlatformMisc::GetLastError();
			TCHAR ErrorMessage[1024];
			FPlatformMisc::GetSystemErrorMessage(ErrorMessage, UE_ARRAY_COUNT(ErrorMessage), ErrorCode);
			UE_LOG(LogFFXD3D12Module, Error, TEXT("Failed to load required FSR SDK DLL '%s' from '%s' (error %u: %s)"), DllName, GetSdkDllDirectory(), ErrorCode, ErrorMessage);
		}
		return Handle;
	};
	LoadSdkDll(TEXT("amd_fidelityfx_loader_dx12.dll"));
	LoadSdkDll(TEXT("amd_fidelityfx_upscaler_dx12.dll"));
	LoadSdkDll(TEXT("amd_fidelityfx_framegeneration_dx12.dll"));
	FPlatformProcess::PopDllDirectory(GetSdkDllDirectory());
}

void FFXD3D12::ShutdownModule()
{
}
