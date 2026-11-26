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

#include "CoreMinimal.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <libloaderapi.h>  // NOLINT(build/include_order)
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#include <dlfcn.h>  // NOLINT(build/include_order)
#endif

#include "Containers/StringFwd.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Misc/StringBuilder.h"
#include "XeSSCommonUtil.h"

namespace XeSSCommon
{

#if PLATFORM_WINDOWS && defined(_WIN64)  // Only Win64 supported, Win32 not supported by design
const TCHAR* THIRD_PARTY_DLL_RELATIVE_PATH = TEXT("/Binaries/ThirdParty/Win64");
#elif PLATFORM_LINUX  // Linux means Linux64, no Linux32 in Unreal by design
const TCHAR* THIRD_PARTY_DLL_RELATIVE_PATH = TEXT("/Binaries/ThirdParty/Linux");
#else
const TCHAR* THIRD_PARTY_DLL_RELATIVE_PATH = TEXT("");
#endif

void PushThirdPartyDllDirectory()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");
	FPlatformProcess::PushDllDirectory(*FPaths::Combine(Plugin->GetBaseDir(), THIRD_PARTY_DLL_RELATIVE_PATH));
}

void PopThirdPartyDllDirectory()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");
	FPlatformProcess::PopDllDirectory(*FPaths::Combine(Plugin->GetBaseDir(), THIRD_PARTY_DLL_RELATIVE_PATH));
}

bool GetDllFileVersion(const TCHAR* DllFileName, FString& OutVersion)
{
#if PLATFORM_WINDOWS
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("XeSS");
	FString DllPath = FPaths::Combine(Plugin->GetBaseDir(), THIRD_PARTY_DLL_RELATIVE_PATH, DllFileName);

	// Check if file exists
	if (!FPaths::FileExists(DllPath))
	{
		return false;
	}

	// Get the file version using Unreal's platform API
	uint64 FileVersion = FPlatformMisc::GetFileVersion(*DllPath);
	if (FileVersion == 0)
	{
		return false;
	}

	// Extract version components from the 64-bit value
	uint16 MajorVersion = (FileVersion >> 48) & 0xFFFF;
	uint16 MinorVersion = (FileVersion >> 32) & 0xFFFF;
	uint16 PatchVersion = (FileVersion >> 16) & 0xFFFF;
	uint16 BuildNumber = FileVersion & 0xFFFF;

	// Format version string
	OutVersion = FString::Printf(TEXT("%d.%d.%d.%d"), MajorVersion, MinorVersion, PatchVersion, BuildNumber);
	return true;
#else
	checkf(false, TEXT("GetDllFileVersion is Windows-only and cannot be called on this platform. Check calling code for proper platform guards."));
	OutVersion = TEXT("");
	return false;
#endif
}

}
