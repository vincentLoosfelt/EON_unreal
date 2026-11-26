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

#include "XeSSSDKLoader.h"

#include "XeSSCommonUtil.h"
#include "XeSSSDKWrapperD3D11.h"
#include "XeSSSDKWrapperD3D12.h"
#include "XeSSSDKWrapperVulkan.h"
#include "HAL/PlatformProcess.h"

DEFINE_LOG_CATEGORY_STATIC(LogXeSSSDKLoader, Log, All);

static TSharedPtr<FXeSSSDKWrapperBase, ESPMode::ThreadSafe> XeSSSDKWrapper;

TSharedPtr<FXeSSSDKWrapperBase, ESPMode::ThreadSafe> FXeSSSDKLoader::Load(const TCHAR* InRHIName)
{
	FString RHIName(InRHIName);
	if (XeSSSDKWrapper)
	{
		// Only one RHI loading supported to simplify code
		check(RHIName == XeSSSDKWrapper->GetRHIName());
		return XeSSSDKWrapper;
	}

	if (RHIName == TEXT("D3D12"))
	{
		XeSSSDKWrapper = MakeShareable(new FXeSSSDKWrapperD3D12(InRHIName));
	}
	else if (RHIName == TEXT("D3D11"))
	{
		XeSSSDKWrapper = MakeShareable(new FXeSSSDKWrapperD3D11(InRHIName));
	}
	else if (RHIName == TEXT("Vulkan"))
	{
		XeSSSDKWrapper = MakeShareable(new FXeSSSDKWrapperVulkan(InRHIName));
	}
	if (XeSSSDKWrapper)
	{
		XeSSSDKWrapper->Load();
	}
	return XeSSSDKWrapper;
}

void FXeSSSDKLoader::Unload()
{
	XeSSSDKWrapper.Reset();
}

bool FXeSSSDKLoader::TryLoadDll(const TCHAR* DllFileName)
{
	if (!DllFileName || FCString::Strlen(DllFileName) == 0)
	{
		UE_LOG(LogXeSSSDKLoader, Warning, TEXT("TryLoadDll called with empty DLL filename"));
		return false;
	}

	XeSSCommon::PushThirdPartyDllDirectory();
	void* DllHandle = FPlatformProcess::GetDllHandle(DllFileName);
	XeSSCommon::PopThirdPartyDllDirectory();

	if (DllHandle)
	{
		FPlatformProcess::FreeDllHandle(DllHandle);
		UE_LOG(LogXeSSSDKLoader, Verbose, TEXT("Successfully loaded DLL: %s"), DllFileName);
		return true;
	}
	else
	{
		UE_LOG(LogXeSSSDKLoader, Warning, TEXT("Failed to load DLL: %s"), DllFileName);
		return false;
	}
}
