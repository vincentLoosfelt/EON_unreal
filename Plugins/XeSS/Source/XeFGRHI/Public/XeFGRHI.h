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

#pragma once

#include <atomic>
#include "CoreMinimal.h"
#include "XeSSCommonMacros.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "XeSSUnrealCore.h"

class FXeFGDXGISwapChainProvider;
class FXeLLModule;
class FRHICommandListBase;
class FRHITexture;
class FRHIViewport;
class FViewport;
class IConsoleVariable;
class SWindow;
struct IDXGISwapChain;
struct DXGI_SWAP_CHAIN_DESC1;
struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC;
struct IDXGIFactory2;
typedef struct _xefg_swapchain_handle_t* xefg_swapchain_handle_t;
typedef struct _GUID GUID;
typedef struct HWND__ *HWND;
typedef GUID IID;

struct XEFGRHI_API FXeFGRHIArguments
{
	FRHITexture* HUDLessColorTexture;
	FIntRect HUDLessColorRect;
	FRHITexture* VelocityTexture;
	FIntRect VelocityRect;
	FRHITexture* DepthTexture;
	FIntRect DepthRect;
	uint32 FrameID;
	FVector2f JitterOffset;
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	bool bCameraCut;  // TODO(sunzhuoshi): check when to enable it
};

class XEFGRHI_API FXeFGRHI
{
public:
	explicit FXeFGRHI(FXeLLModule& InXeLLModule);
	virtual ~FXeFGRHI();

	bool Initialize();
	// In some cases, XeFG is not available even if it is supported, e.g., in split screen mode.
	bool IsXeFGAvailable() const;
	bool IsXeFGEnabled() const;
	HRESULT CreateInterpolationSwapChain(HWND WindowsHandle, const DXGI_SWAP_CHAIN_DESC1* SwapChainDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* FullscreenDesc, IDXGIFactory2* Factory, const IID& SwapChainInterfaceID, IDXGISwapChain** OutSwapChain);
	void TagBackBuffer(uint32 FrameID, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList);
	void TagUI(uint32 FrameID, FRHITexture* Texture, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList);
	void TagForPresent(const FXeFGRHIArguments& Arguments, FRHICommandListBase& ExecutingCmdList);
	void HandleXeFGEnabledSet(IConsoleVariable* Variable);
	void HandleXeFGMaxInterpolatedFrames(IConsoleVariable* Variable);
	void HandleXeFGUICompositionState(IConsoleVariable* Variable);
	void OnXeLLPresentLatencyMarkerEnd(uint64);
	bool IfGameViewportUsesXeFGSwapChain() const;
	bool IsGameViewportWindow(const SWindow& Window) const;
private:
	FViewport* GetGameViewport() const;
	FRHIViewport* GetRHIGameViewport() const;
	bool CreateXeFGSwapChainContext();
	bool GetXeFGSwapChainProperties();
	bool SetXeFGLoggingCallback();
	bool SetupXeLLContext();
	void OverrideSwapChain();
	void SetXeFGEnabledWithTemporary(bool bEnabled, bool bTemporary);
	void TagConstants(const FXeFGRHIArguments& Arguments);
	void TagTexture(int32 Type, uint32 FrameID, FRHITexture* Texture, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList);
	bool IsXeFGSwapChain(IDXGISwapChain* SwapChain) const;
	void OnGameViewportCreated();
#if STATS
	void UpdateXeFGMemoryStats(uint32_t Width, uint32_t Height);
	void OnGameViewportResized(FViewport* Viewport, uint32 Index);
#endif
	TUniquePtr<FXeFGDXGISwapChainProvider> CustomSwapChainProvider;
	uint32_t XeFGSwapChainInitFlags = 0;
	xefg_swapchain_handle_t XeFGSwapChainContext = nullptr;
	FXeLLModule& XeLLModule;
	uint32 TargetFrameID = 0;
	bool bTargetFrameHandled = false;
	bool bIsXeFGEnabledWithAPI = false;
	bool bGameViewportUsesXeFGSwapChain = false;
	std::atomic<const SWindow*> GameViewportWindow{ nullptr };
	IConsoleVariable* CVarXeFGEnabled = nullptr;
	FDelegateHandle OnGameViewportCreatedDelegateHandle;
#if STATS
	uint32_t CurrentGameViewportWidth = 0;
	uint32_t CurrentGameViewportHeight = 0;
	FDelegateHandle OnGameViewportResizedDelegateHandle;
#endif
};

// Similar to GAverageFPS, used to display in-game FPS with or without XeFG
extern XEFGRHI_API float GXeFGAverageFPS;

#endif
