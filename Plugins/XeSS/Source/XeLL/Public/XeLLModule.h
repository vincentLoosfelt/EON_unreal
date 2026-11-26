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

#include "CoreMinimal.h"
#include "XeLLLatencyMarkers.h"
#include "XeLLMaxTickRateHandler.h"
#include "XeSSCommonCoreModuleBase.h"
#include "XeSSCommonMacros.h"
#include "XeSSUnrealCore.h"

class FXeLLModule : public FXeSSCommonCoreModuleBase, public XeSSUnreal::XTSTickerObjectBase
{
private:
	const TCHAR* GetFeatureName() const override { return TEXT("XeLL"); }
	const TCHAR* GetModuleName() const override { return TEXT("XeLLCore"); }
	bool IsEditorSupported() const override { return false; }
	bool IsEngineVersionSupported() const override;
	bool IsRHISupported(const FString& RHIName) const override { return RHIName == TEXT("D3D12"); }
	bool Init() override;
	void Deinit() override;
	int GetSDKVersion(XeSSCommon::FSDKVersion& SDKVersion) override;
	void OnPostEngineInit() override;
#if XESS_ENGINE_WITH_XEFG_API
public:
	FXeLLLatencyMarkers* GetLatencyMarker() { return XeLLLatencyMarker.Get(); }
	FXeLLMaxTickRateHandler* GetMaxTickRateHandler() { return XeLLMaxTickRateHandler.Get(); }
private:
	TUniquePtr<FXeLLLatencyMarkers> XeLLLatencyMarker;
	TUniquePtr<FXeLLMaxTickRateHandler> XeLLMaxTickRateHandler;
	int32 XeLLMode = 0;

	struct FMaxTickRateHandlerData
	{
		IMaxTickRateHandlerModule* Handler;
		uint32 Flag;
	};
	TArray<FMaxTickRateHandlerData> ObservedMaxTickRateHandlers;
	TArray<ILatencyMarkerModule*> ObservedLatencyMarkers;
#endif
public:
	// XeSSUnreal::XTSTickerObjectBase
	bool Tick(float DeltaTime) override;

	// Get the ptr of the XeLL context
	XELLCORE_API void* GetXeLLContext();

	// XeFG needs XeLL to be enabled
	XELLCORE_API bool OnPreSetXeFGEnabled();

	// Check XeLL available
	// XeLL can be supported on this platform, but may be disabled by the client. This function is used to determine whether XeLL is currently available.
	XELLCORE_API bool IsXeLLAvailable();

private:
	// Set XeLL mode, 0: off, 1: low latency enabled
	void SetXeLLMode(const uint32 Flags);
};
