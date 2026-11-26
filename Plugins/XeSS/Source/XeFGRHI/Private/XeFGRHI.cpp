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

#include "XeFGRHI.h"

#include "HAL/IConsoleManager.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Engine/GameViewportClient.h"
#include "Features/IModularFeatures.h"
#include "Framework/Application/SlateApplication.h"
#include "RHIResources.h"
#include "Rendering/SlateRenderer.h"
#include "SystemTextures.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"
#include "XeFGDXGISwapChainProvider.h"
#include "XeFGRHIModule.h"
#include "XeFGRHIUtil.h"
#include "XeFGTypes.h"
#include "XeLLDelegates.h"
#include "XeLLModule.h"
#include "XeSSCommonUtil.h"
#include "XeSSSDKUtil.h"
#include "XeSSUnrealD3D12RHI.h"
#include "XeSSUnrealSlateCore.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess_fg/xefg_swapchain_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if XESS_ENGINE_WITH_XEFG_API
DEFINE_LOG_CATEGORY_STATIC(LogXeFGRHI, Log, All);

// Log category for XeFG SDK
DEFINE_LOG_CATEGORY_STATIC(LogXeFGSDK, Log, All);

DECLARE_STATS_GROUP(TEXT("XeFG"), STATGROUP_XeFG, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("Frames Presented"), STAT_XeFGFramesPresented, STATGROUP_XeFG);
DECLARE_FLOAT_COUNTER_STAT(TEXT("Average FPS"), STAT_XeFGAverageFPS, STATGROUP_XeFG);
DECLARE_MEMORY_STAT(TEXT("XeFG Buffer Memory"), STAT_XeFGTempBufferHeapSize, STATGROUP_XeFG);
DECLARE_MEMORY_STAT(TEXT("XeFG Texture Memory"), STAT_XeFGTempTextureHeapSize, STATGROUP_XeFG);
DECLARE_MEMORY_STAT(TEXT("XeFG Constant Buffer Memory"), STAT_XeFGConstantBufferSize, STATGROUP_XeFG);

extern ENGINE_API float GAverageFPS;
XEFGRHI_API float GXeFGAverageFPS = 0.0f;

static const IID IID_XeFGDSwapChainIID = { 0x157E1000, 0x16F8, 0x16F8, {0x16, 0xF8, 0x15, 0x7E, 0x94, 0x01, 0xA7, 0x10} };

static TAutoConsoleVariable<float> CVarXeFGMotionVectorScale(
	TEXT("r.XeFG.MotionVectorScale"),
	1.0f,
	TEXT("[default: 1.0f] Scale applied to motion vector."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarXeFGUICompositionState(
	TEXT("r.XeFG.UICompositionState"),
	0,
	TEXT("[default: 0] UI composition state. If something is wrong with your UI, try to enable UI composition.")
	TEXT(" 0: Disabled\n")
	TEXT(" 1: Enabled"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarXeFGUIMode(
	TEXT("r.XeFG.UIMode"),
	5,
	TEXT("[default: 5] UI mode.")
	TEXT(" 0: Determine UI handling mode automatically, based on provided inputs: hudless color and UI texture\n")
	TEXT(" 1: Interpolate on backbuffer, without any UI handling\n")
	TEXT(" 2: Interpolate on backbuffer, refine UI using UI texture + alpha\n")
	TEXT(" 3: Interpolate on hudless color, blend UI from UI texture + alpha\n")
	TEXT(" 4: Interpolate on hudless color, extract UI from backbuffer\n")
	TEXT(" 5: Interpolate on hudless color, blend UI from UI texture + alpha and refine it by extracting from backbuffer"),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeFGResourceValidity(
	TEXT("r.XeFG.ResourceValidity"),
	1,
	TEXT("[default: 1] Validity of input resources of XeFG, set it via command line -XeFGResourceValidity=\n")
	TEXT(" 0: XEFG_SWAPCHAIN_RV_UNTIL_NEXT_PRESENT\n")
	TEXT(" 1: XEFG_SWAPCHAIN_RV_ONLY_NOW"),
	ECVF_ReadOnly
);

static TAutoConsoleVariable<bool> CVarXeFGOverrideSwapChain(
	TEXT("r.XeFG.OverrideSwapChain"),
	true,
	TEXT("[default: true] If override swap chain, it should help prevent conflicts with other similar plugins."),
	ECVF_ReadOnly);

static TAutoConsoleVariable<int32> CVarXeFGMaxInterpolatedFrames(
	TEXT("r.XeFG.MaxInterpolatedFrames"),
	1,
	TEXT("[default: 1] Max frames XeFG interpolates. 0: auto (uses r.XeFG.MaxInterpolatedFramesSupported). ")
	TEXT("Explicit values must be in range [1, r.XeFG.MaxInterpolatedFramesSupported]."),
	ECVF_Default);

static int32 GXeFGMaxInterpolatedFrameSupported = 0;
static FAutoConsoleVariableRef CVarXeFGMaxInterpolatedFramesSupported(
	TEXT("r.XeFG.MaxInterpolatedFramesSupported"),
	GXeFGMaxInterpolatedFrameSupported,
	TEXT("Max interpolated frames supported by XeFG."),
	ECVF_ReadOnly);

static void XeFGLogCallback(const char* Message, xefg_swapchain_logging_level_t InLogLevel, void*)
{
	switch (InLogLevel)
	{
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_DEBUG:
		UE_LOG(LogXeFGSDK, Verbose, TEXT("%hs"), Message);
		break;
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_INFO:
		UE_LOG(LogXeFGSDK, Log, TEXT("%hs"), Message);
		break;
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_ERROR:
		UE_LOG(LogXeFGSDK, Error, TEXT("%hs"), Message);
		break;
	case XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING:
	default:
		UE_LOG(LogXeFGSDK, Warning, TEXT("%hs"), Message);
		break;
	}
}

FXeFGRHI::FXeFGRHI(FXeLLModule& InXeLLModule)
	: XeFGSwapChainInitFlags(XEFG_SWAPCHAIN_INIT_FLAG_INVERTED_DEPTH)
	, XeLLModule(InXeLLModule)
{
	CVarXeFGEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.Enabled"));
	check(CVarXeFGEnabled);
}

FXeFGRHI::~FXeFGRHI()
{
	CVarXeFGEnabled->OnChangedDelegate().RemoveAll(this);
	if (FXeLLDelegates::OnPresentLatencyMarkerEnd.IsBound())
	{
		FXeLLDelegates::OnPresentLatencyMarkerEnd.Unbind();
	}
#if STATS
	if (OnGameViewportResizedDelegateHandle.IsValid())
	{
		FViewport* Viewport = GetGameViewport();
		if (Viewport)
		{
			Viewport->ViewportResizedEvent.Remove(OnGameViewportResizedDelegateHandle);
		}
	}
#endif
	if (OnGameViewportCreatedDelegateHandle.IsValid())
	{
		UGameViewportClient::OnViewportCreated().Remove(OnGameViewportCreatedDelegateHandle);
	}
	if (CustomSwapChainProvider.IsValid())
	{
		IModularFeatures::Get().UnregisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapChainProvider.Get());
		CustomSwapChainProvider.Reset();
	}
	if (XeFGSwapChainContext)
	{
		xefgSwapChainDestroy(XeFGSwapChainContext);
	}
}

bool FXeFGRHI::CreateXeFGSwapChainContext()
{
	XeSSCommon::PushThirdPartyDllDirectory();
	xefg_swapchain_result_t Result = xefgSwapChainD3D12CreateContext(XeSSUnreal::D3D12RHIGetDevice(), &XeFGSwapChainContext);
	XeSSCommon::PopThirdPartyDllDirectory();

	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to create XeFG swap chain context, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return false;
	}
	UE_LOG(LogXeFGRHI, Display, TEXT("XeFG swap chain context created"));
	return true;
}

bool FXeFGRHI::GetXeFGSwapChainProperties()
{
	xefg_swapchain_properties_t Properties = {};
	xefg_swapchain_result_t Result = xefgSwapChainGetProperties(XeFGSwapChainContext, &Properties);

	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to get XeFG swap chain properties, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return false;
	}
	XeSSUnreal::SetWithCurrentPriority(*CVarXeFGMaxInterpolatedFramesSupported, static_cast<int32>(Properties.maxSupportedInterpolations));
	if (CVarXeFGMaxInterpolatedFrames->GetInt() == 0)
	{
		XeSSUnreal::SetWithCurrentPriority(*CVarXeFGMaxInterpolatedFrames, CVarXeFGMaxInterpolatedFramesSupported->GetInt());
	}
	return true;
}

FViewport* FXeFGRHI::GetGameViewport() const
{
	if (GEngine && GEngine->GameViewport)
	{
		return GEngine->GameViewport->Viewport;
	}
	return nullptr;
}

#if STATS
void FXeFGRHI::UpdateXeFGMemoryStats(uint32_t Width, uint32_t Height)
{
	xefg_swapchain_properties_t MemoryProperties = {};
	xefg_swapchain_result_t MemoryResult = xefgSwapChainD3D12GetProperties(
		XeFGSwapChainContext,
		nullptr,  // Pass null to reuse the parameters set during swap chain initialization
		Width,
		Height,
		DXGI_FORMAT_UNKNOWN,
		&MemoryProperties);

	if (XESS_SUCCEEDED(MemoryResult))
	{
		SET_MEMORY_STAT(STAT_XeFGTempBufferHeapSize, MemoryProperties.tempBufferHeapSize);
		SET_MEMORY_STAT(STAT_XeFGTempTextureHeapSize, MemoryProperties.tempTextureHeapSize);
		SET_MEMORY_STAT(STAT_XeFGConstantBufferSize, MemoryProperties.constantBufferSize);

		UE_LOG(LogXeFGRHI, Verbose, TEXT("XeFG Memory Stats Updated (%dx%d) - TempBuffer: %.2f MB, TempTexture: %.2f MB, ConstantBuffer: %.2f KB"),
			Width, Height,
			MemoryProperties.tempBufferHeapSize / (1024.0 * 1024.0),
			MemoryProperties.tempTextureHeapSize / (1024.0 * 1024.0),
			MemoryProperties.constantBufferSize / 1024.0);
	}
	else
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to get XeFG swap chain memory properties, result: %d, %s"), MemoryResult, XeSSSDK::GetMessage(MemoryResult));
	}
}

void FXeFGRHI::OnGameViewportResized(FViewport* Viewport, uint32 Index)
{
	check(Viewport);

	// Recalculate XeFG swap chain memory stats when the viewport size changes
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	uint32_t NewWidth = static_cast<uint32_t>(ViewportSize.X);
	uint32_t NewHeight = static_cast<uint32_t>(ViewportSize.Y);

	// Only update if resolution has changed
	if (NewWidth != CurrentGameViewportWidth || NewHeight != CurrentGameViewportHeight)
	{
		CurrentGameViewportWidth = NewWidth;
		CurrentGameViewportHeight = NewHeight;
		UpdateXeFGMemoryStats(CurrentGameViewportWidth, CurrentGameViewportHeight);
	}
}
#endif

void FXeFGRHI::OnGameViewportCreated()
{
	check(IsInGameThread());

	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	TSharedPtr<SWindow> SlateWindow = GameEngine ? GameEngine->GameViewportWindow.Pin() : nullptr;
	GameViewportWindow.store(SlateWindow.Get(), std::memory_order_relaxed);
	// Cache whether the game viewport uses the XeFG swap chain when the viewport is created,
	// so IfGameViewportUsesXeFGSwapChain() does not need to query the swap chain interface on every call.
	FRHIViewport* Viewport = GetRHIGameViewport();
	bGameViewportUsesXeFGSwapChain = Viewport
		&& IsXeFGSwapChain(reinterpret_cast<IDXGISwapChain*>(Viewport->GetNativeSwapChain()));

#if STATS
	FViewport* GameViewport = GetGameViewport();
	check(GameViewport);

	if (!OnGameViewportResizedDelegateHandle.IsValid())
	{
		OnGameViewportResizedDelegateHandle = GameViewport->ViewportResizedEvent.AddRaw(this, &FXeFGRHI::OnGameViewportResized);
	}
#endif
}

bool FXeFGRHI::SetXeFGLoggingCallback()
{
	IConsoleVariable* CVarXeFGLogLevel = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.LogLevel"));
	check(CVarXeFGLogLevel);
	int32 LogLevelValue = CVarXeFGLogLevel->GetInt();
	if (XeSSSDK::IsLogLevelValid(LogLevelValue))
	{
		xefg_swapchain_result_t Result = xefgSwapChainSetLoggingCallback(
			XeFGSwapChainContext,
			static_cast<xefg_swapchain_logging_level_t>(LogLevelValue),
			XeFGLogCallback,
			nullptr
		);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogXeFGRHI, Error, TEXT("Failed to set XeFG swap chain logging callback, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
			return false;
		}
	}
	else
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("r.XeFG.LogLevel is %d, skipping XeFG logging callback setup"), LogLevelValue);
	}
	return true;
}

bool FXeFGRHI::SetupXeLLContext()
{
	void* XeLLContext = XeLLModule.GetXeLLContext();
	if (!XeLLContext)
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to get XeLL context used by XeFG, probably not supported."));
		return false;
	}

	xefg_swapchain_result_t Result = xefgSwapChainSetLatencyReduction(XeFGSwapChainContext, XeLLContext);

	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set latency reduction context, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return false;
	}
	return true;
}

void FXeFGRHI::OverrideSwapChain()
{
	bool bXeFGOverrideSwapChain = CVarXeFGOverrideSwapChain->GetBool();

	UE_LOG(LogXeFGRHI, Display, TEXT("Override swap chain: %d"), bXeFGOverrideSwapChain);
	if (bXeFGOverrideSwapChain)
	{
		CustomSwapChainProvider = MakeUnique<FXeFGDXGISwapChainProvider>(this);
		IModularFeatures::Get().RegisterModularFeature(IDXGISwapchainProvider::GetModularFeatureName(), CustomSwapChainProvider.Get());
		FXeLLDelegates::OnPresentLatencyMarkerEnd.BindRaw(this, &FXeFGRHI::OnXeLLPresentLatencyMarkerEnd);
	}
}

bool FXeFGRHI::Initialize()
{
	if (!CreateXeFGSwapChainContext())
	{
		return false;
	}

	if (!GetXeFGSwapChainProperties())
	{
		return false;
	}

	if (!SetXeFGLoggingCallback())
	{
		return false;
	}

	if (!SetupXeLLContext())
	{
		return false;
	}

	OverrideSwapChain();
	return true;
}

bool FXeFGRHI::IsXeFGAvailable() const
{
	return GEngine && GEngine->GameViewport
		&& GEngine->GameViewport->GetCurrentSplitscreenConfiguration() == ESplitScreenType::None
		&& !GEngine->GameViewport->bDisableWorldRendering
		&& IfGameViewportUsesXeFGSwapChain();
}

bool FXeFGRHI::IsXeFGEnabled() const
{
	int32 XeFGEnabled = CVarXeFGEnabled->GetInt();

	return XeFGEnabled >= EXeFGEnabled_Min && XeFGEnabled <= EXeFGEnabled_Max && XeFGEnabled != EXeFGEnabled_Off;
}

HRESULT FXeFGRHI::CreateInterpolationSwapChain(
	HWND WindowsHandle,
	const DXGI_SWAP_CHAIN_DESC1* SwapChainDesc,
	const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* FullscreenDesc,
	IDXGIFactory2* Factory,
	const IID& SwapChainInterfaceID,
	IDXGISwapChain** OutSwapChain)
{
	check(XeFGSwapChainContext);

	xefg_swapchain_d3d12_init_params_t InitParams{};

	InitParams.uiMode = static_cast<xefg_swapchain_ui_mode_t>(CVarXeFGUIMode->GetInt());
	InitParams.pApplicationSwapChain = nullptr;
	InitParams.maxInterpolatedFrames = CVarXeFGMaxInterpolatedFramesSupported->GetInt();
	InitParams.initFlags = XeFGSwapChainInitFlags;

	xefg_swapchain_result_t Result = xefgSwapChainD3D12InitFromSwapChainDesc(
		XeFGSwapChainContext,
		WindowsHandle,
		SwapChainDesc,
		FullscreenDesc,
		XeSSUnreal::D3D12RHIGetCommandQueue(),
		Factory,
		&InitParams
	);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to create XeFG swap chain, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return E_FAIL;
	}
	IDXGISwapChain* TempSwapChain = nullptr;
	Result = xefgSwapChainD3D12GetSwapChainPtr(XeFGSwapChainContext, SwapChainInterfaceID, reinterpret_cast<void**>(&TempSwapChain));
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to get XeFG swap chain pointer, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		return E_FAIL;
	}
	*OutSwapChain = TempSwapChain;

	// xefgSwapChainSetNumInterpolatedFrames can't be called before xefgSwapChainD3D12InitFromSwapChainDesc
	CVarXeFGEnabled->OnChangedDelegate().Add(FConsoleVariableDelegate::CreateRaw(this, &FXeFGRHI::HandleXeFGEnabledSet));
	CVarXeFGMaxInterpolatedFrames->OnChangedDelegate().Add(FConsoleVariableDelegate::CreateRaw(this, &FXeFGRHI::HandleXeFGMaxInterpolatedFrames));
	CVarXeFGUICompositionState->OnChangedDelegate().Add(FConsoleVariableDelegate::CreateRaw(this, &FXeFGRHI::HandleXeFGUICompositionState));
	// Handle values set in .ini
	HandleXeFGMaxInterpolatedFrames(CVarXeFGMaxInterpolatedFrames->AsVariable());
	HandleXeFGEnabledSet(CVarXeFGEnabled);
	HandleXeFGUICompositionState(CVarXeFGUICompositionState.AsVariable());

	// Register delegate for game viewport creation to cache XeFG swap chain usage (and, with STATS, set up resize monitoring)
	OnGameViewportCreatedDelegateHandle = UGameViewportClient::OnViewportCreated().AddRaw(this, &FXeFGRHI::OnGameViewportCreated);

	return S_OK;
}

void FXeFGRHI::TagBackBuffer(uint32 FrameID, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList)
{
	if (!CVarXeFGEnabled->GetBool())
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging XeFG back buffer because XeFG is disabled, frame ID: %d"), FrameID);
		return;
	}
	ID3D12GraphicsCommandList* CommandList = XeSSUnreal::D3D12RHIGetGraphicsCommandList(ExecutingCmdList);
	xefg_swapchain_d3d12_resource_data_t ResData{};
	ResData.type = XEFG_SWAPCHAIN_RES_BACKBUFFER;
	ResData.resourceBase.x = TextureRect.Min.X;
	ResData.resourceBase.y = TextureRect.Min.Y;
	ResData.resourceSize.x = TextureRect.Width();
	ResData.resourceSize.y = TextureRect.Height();
	xefg_swapchain_result_t Result = xefgSwapChainD3D12TagFrameResource(XeFGSwapChainContext, CommandList, FrameID, &ResData);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to tag XeFG swap chain back buffer, frame ID: %d, result: %d, %s"), FrameID, Result, XeSSSDK::GetMessage(Result));
	}
}

void FXeFGRHI::TagUI(uint32 FrameID, FRHITexture* Texture, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList)
{
	if (!IsXeFGEnabled())
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging XeFG UI because XeFG is disabled, frame ID: %d"), FrameID);
		return;
	}
	// TODO(sunzhuoshi): move checking to FXeFGUIHandler::OnBackBufferReadyToPresent() to avoid unnecessary UI extraction pass
	// NOTE: render thread should be used to check, not current RHI thread
	if (FrameID != TargetFrameID || bTargetFrameHandled)
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging UI with frame ID: %d, target frame ID: %d, target frame handled: %d"),
			FrameID, TargetFrameID, bTargetFrameHandled);
		// UI frame ID mismatch with target frame ID, disable XeFG temporarily,
		// until interface method of FXeFGViewExtension used is called.
		if (bIsXeFGEnabledWithAPI)
		{
			UE_LOG(LogXeFGRHI, Log, TEXT("Frame data mismatch detected, disable XeFG temporarily"));
			SetXeFGEnabledWithTemporary(false, true);
		}
		return;
	}
	TagTexture(XEFG_SWAPCHAIN_RES_UI, FrameID, Texture, TextureRect, ExecutingCmdList);
	bTargetFrameHandled = true;
}

void FXeFGRHI::TagConstants(const FXeFGRHIArguments& Arguments)
{
	xefg_swapchain_frame_constant_data_t Constants = {};
	float MotionVectorScale = CVarXeFGMotionVectorScale->GetFloat();
	Constants.motionVectorScaleX = MotionVectorScale;
	Constants.motionVectorScaleY = MotionVectorScale;
	Constants.jitterOffsetX = Arguments.JitterOffset.X;
	Constants.jitterOffsetY = Arguments.JitterOffset.Y;
	XeFGRHI::Assign(Arguments.ViewMatrix, Constants.viewMatrix);
	XeFGRHI::Assign(Arguments.ProjectionMatrix, Constants.projectionMatrix);
	Constants.resetHistory = Arguments.bCameraCut;
	xefg_swapchain_result_t Result = xefgSwapChainTagFrameConstants(XeFGSwapChainContext, Arguments.FrameID, &Constants);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to tag XeFG swap chain frame constants, frame ID: %d, result: %d, %s"), Arguments.FrameID, Result, XeSSSDK::GetMessage(Result));
	}
}

void FXeFGRHI::TagTexture(int32 Type, uint32 FrameID, FRHITexture* Texture, const FIntRect& TextureRect, FRHICommandListBase& ExecutingCmdList)
{
	ID3D12Resource* Resource = static_cast<ID3D12Resource*>(Texture->GetNativeResource());
	check(Resource);
	ID3D12GraphicsCommandList* CommandList = XeSSUnreal::D3D12RHIGetGraphicsCommandList(ExecutingCmdList);
	xefg_swapchain_d3d12_resource_data_t ResData{};
	D3D12_RESOURCE_DESC Desc = Resource->GetDesc();
	D3D12_RESOURCE_STATES ResourceState = D3D12_RESOURCE_STATE_COMMON;
#if XESS_ENGINE_VERSION_GEQ(5, 6)
	D3D12_RESOURCE_STATES StencilResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
#else
	D3D12_RESOURCE_STATES StencilResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
#endif

	switch (Type)
	{
	case XEFG_SWAPCHAIN_RES_HUDLESS_COLOR:
		ResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
		break;
	case XEFG_SWAPCHAIN_RES_MOTION_VECTOR:
	case XEFG_SWAPCHAIN_RES_UI:
		ResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		break;
	case XEFG_SWAPCHAIN_RES_DEPTH:
#if XESS_ENGINE_VERSION_GEQ(5, 6)
		ResourceState = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
#elif XESS_ENGINE_VERSION_GEQ(5, 2)
		ResourceState = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
#else
		ResourceState = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
#endif
		break;
	case XEFG_SWAPCHAIN_RES_BACKBUFFER:
		break;
	default:
		checkf(false, TEXT("Resource type not supported yet."));
		break;
	}
	ResData.type = static_cast<xefg_swapchain_resource_type_t>(Type);
	ResData.validity = static_cast<xefg_swapchain_resource_validity_t>(CVarXeFGResourceValidity->GetInt());
	ResData.resourceBase.x = TextureRect.Min.X;
	ResData.resourceBase.y = TextureRect.Min.Y;
	ResData.resourceSize.x = TextureRect.Width();
	ResData.resourceSize.y = TextureRect.Height();
	ResData.pResource = Resource;
	ResData.incomingState = ResourceState;
	/*
	* DepthStencil texture uses planar resources with subresource index and subresources are in different states here:
	*   0: depth plane, state: ResourceState
	*   1: stencil plane, state: StencilResourceState
	* Transition stencil plane to the same state as depth plane before calling and revert after calling.
	* Reference: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/PlanarDepthStencilDDISpec.md
	*/
	if (XEFG_SWAPCHAIN_RES_DEPTH == Type)
	{
		XeSSUnreal::TransitionResource(Texture, StencilResourceState, ResourceState, 1, ExecutingCmdList);
	}
	xefg_swapchain_result_t Result = xefgSwapChainD3D12TagFrameResource(XeFGSwapChainContext, CommandList, FrameID, &ResData);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to tag XeFG swap chain frame resource, frame ID: %d, result: %d, %s"), FrameID, Result, XeSSSDK::GetMessage(Result));
	}
	if (XEFG_SWAPCHAIN_RES_DEPTH == Type)
	{
		XeSSUnreal::TransitionResource(Texture, ResourceState, StencilResourceState, 1, ExecutingCmdList);
	}
}

bool FXeFGRHI::IsXeFGSwapChain(IDXGISwapChain* SwapChain) const
{
	check(SwapChain);

	TRefCountPtr<IUnknown> XeFGSwapChain;
	return SUCCEEDED(SwapChain->QueryInterface(IID_XeFGDSwapChainIID, reinterpret_cast<void**>(XeFGSwapChain.GetInitReference())));
}

bool FXeFGRHI::IfGameViewportUsesXeFGSwapChain() const
{
	return bGameViewportUsesXeFGSwapChain;
}

bool FXeFGRHI::IsGameViewportWindow(const SWindow& Window) const
{
	return &Window == GameViewportWindow.load(std::memory_order_relaxed);
}

void FXeFGRHI::TagForPresent(const FXeFGRHIArguments& Arguments, FRHICommandListBase& ExecutingCmdList)
{
	if (!CVarXeFGEnabled->GetBool())
	{
		UE_LOG(LogXeFGRHI, Log, TEXT("Skip tagging XeFG resources for present because XeFG is disabled, frame ID: %d"), Arguments.FrameID);
		return;
	}
	TagConstants(Arguments);
	TagTexture(XEFG_SWAPCHAIN_RES_HUDLESS_COLOR, Arguments.FrameID, Arguments.HUDLessColorTexture, Arguments.HUDLessColorRect, ExecutingCmdList);
	TagTexture(XEFG_SWAPCHAIN_RES_MOTION_VECTOR, Arguments.FrameID, Arguments.VelocityTexture, Arguments.VelocityRect, ExecutingCmdList);
	TagTexture(XEFG_SWAPCHAIN_RES_DEPTH, Arguments.FrameID, Arguments.DepthTexture, Arguments.DepthRect, ExecutingCmdList);
	xefg_swapchain_result_t Result = xefgSwapChainSetPresentId(XeFGSwapChainContext, Arguments.FrameID);
	if (XESS_SUCCEEDED(Result))
	{
		// Only re-enable XeFG if it is still enabled (it may be disabled by API since internal temporary disabling)
		if (!bIsXeFGEnabledWithAPI && IsXeFGEnabled())
		{
			UE_LOG(LogXeFGRHI, Log, TEXT("Re-enabling temporarily disabled XeFG"));
			SetXeFGEnabledWithTemporary(true, true);
		}
	}
	else
	{
		UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeFG swap chain present ID, result: %d, frame ID: %d"), Result, Arguments.FrameID);
	}
	TargetFrameID = Arguments.FrameID;
	bTargetFrameHandled = false;
}

void FXeFGRHI::SetXeFGEnabledWithTemporary(bool bEnabled, bool bTemporary)
{
	check(IsInGameThread() || bTemporary)  // Either called in game thread or in other thread temporarily

	UE_LOG(LogXeFGRHI, Log, TEXT("Set XeFG enabled: %s, Is XeFG enabled with API: %s"), XeSSCommon::ToString(bEnabled), XeSSCommon::ToString(bIsXeFGEnabledWithAPI));
	if (bIsXeFGEnabledWithAPI != bEnabled)
	{
		// Don't notify XeLL if XeFG enabled set temporarily
		if (bEnabled && !bTemporary)
		{
			if (!XeLLModule.OnPreSetXeFGEnabled())
			{
				UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeLL before enabling XeFG"));
				return;
			}
		}
		xefg_swapchain_result_t Result = xefgSwapChainSetEnabled(XeFGSwapChainContext, bEnabled);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogXeFGRHI, Warning, TEXT("Failed to set XeFG swap chain enabled, result: %d, %s, enabled: %s"), Result, XeSSSDK::GetMessage(Result), XeSSCommon::ToString(bEnabled));
			return;
		}
		bIsXeFGEnabledWithAPI = bEnabled;
	}
}

FRHIViewport* FXeFGRHI::GetRHIGameViewport() const
{
	check(IsInGameThread());

	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	TSharedPtr<SWindow> SlateWindow = GameEngine ? GameEngine->GameViewportWindow.Pin() : nullptr;
	if (SlateWindow.IsValid() && FSlateApplication::IsInitialized())
	{
		if (FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer())
		{
			return XeSSUnreal::GetViewportResource(*SlateRenderer, *SlateWindow);
		}
	}
	return nullptr;
}
void FXeFGRHI::HandleXeFGEnabledSet(IConsoleVariable* Variable)
{
	int32 Enabled = Variable->GetInt();

	if (Enabled != EXeFGEnabled_Off)
	{
		switch (Enabled)
		{
		case EXeFGEnabled_On:
		{
			FViewport* GameViewport = GetGameViewport();
			UE_LOG(LogXeFGRHI, Log, TEXT("XeFG enabled: game viewport: %p, is XeFG available: %s"),
				GameViewport, XeSSCommon::ToString(IsXeFGAvailable()));
			break;
		}
		default:
			UE_LOG(LogXeFGRHI, Warning, TEXT("Invalid r.XeFG.Enabled value: %d, valid range: [%d, %d], disable it"), Enabled, EXeFGEnabled_Min, EXeFGEnabled_Max);
			Enabled = EXeFGEnabled_Off;
			break;
		}
	}
	SetXeFGEnabledWithTemporary(Enabled != EXeFGEnabled_Off, false);
}

void FXeFGRHI::HandleXeFGMaxInterpolatedFrames(IConsoleVariable* Variable)
{
	if (!ensureMsgf(GXeFGMaxInterpolatedFrameSupported > 0,
		TEXT("HandleXeFGMaxInterpolatedFrames called before GXeFGMaxInterpolatedFrameSupported is initialized")))
	{
		return;
	}
	int32 MaxInterpolatedFrames = Variable->GetInt();
	if (MaxInterpolatedFrames >= 1 && MaxInterpolatedFrames <= GXeFGMaxInterpolatedFrameSupported)
	{
		// Value is in valid range, apply it
		xefg_swapchain_result_t Result = xefgSwapChainSetNumInterpolatedFrames(XeFGSwapChainContext, MaxInterpolatedFrames);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogXeFGRHI, Error, TEXT("Failed to set XeFG swap chain max interpolated frames, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		}
		return;
	}
	if (MaxInterpolatedFrames != 0)
	{
		// Out-of-range value (0 means "use max supported" and is silently resolved)
		UE_LOG(LogXeFGRHI, Warning, TEXT("Invalid r.XeFG.MaxInterpolatedFrames value: %d, valid range: [1, %d], reset to %d"),
			MaxInterpolatedFrames, GXeFGMaxInterpolatedFrameSupported, GXeFGMaxInterpolatedFrameSupported);
	}
	// Resolve to max supported: SetWithCurrentPriority re-triggers this handler with the corrected value,
	// which will then call xefgSwapChainSetNumInterpolatedFrames in the valid-range branch above.
	XeSSUnreal::SetWithCurrentPriority(*Variable, GXeFGMaxInterpolatedFrameSupported);
}

void FXeFGRHI::HandleXeFGUICompositionState(IConsoleVariable* Variable)
{
	xefg_swapchain_ui_composition_state_t State = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_DISABLED;
	int32 VarValue = Variable->GetInt();
	switch (VarValue)
	{
	case 0:
		State = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_DISABLED;
		break;
	case 1:
		State = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_ENABLED;
		break;
	default:
		UE_LOG(LogXeFGRHI, Warning, TEXT("Invalid r.XeFG.UICompositionState value: %d, treating it as 1"), VarValue);
		State = XEFG_SWAPCHAIN_UI_COMPOSITION_STATE_ENABLED;
		break;
	}
	xefg_swapchain_result_t Result = xefgSwapChainSetUiCompositionState(XeFGSwapChainContext, State);
	if (XESS_FAILED(Result))
	{
		UE_LOG(LogXeFGRHI, Error, TEXT("Failed to set XeFG swap chain UI composition state, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
	}
}

void FXeFGRHI::OnXeLLPresentLatencyMarkerEnd(uint64 FrameNumber)
{
	check(IsInRHIThread() || !IsRunningRHIInSeparateThread());
	xefg_swapchain_present_status_t PresentStatus;
	xefg_swapchain_result_t Result = xefgSwapChainGetLastPresentStatus(XeFGSwapChainContext, &PresentStatus);

	switch (Result)
	{
	case XEFG_SWAPCHAIN_RESULT_SUCCESS:
		{
			uint32_t FramesPresented = PresentStatus.framesPresented;
#if STATS
			SET_DWORD_STAT(STAT_XeFGFramesPresented, FramesPresented);
			SET_FLOAT_STAT(STAT_XeFGAverageFPS, GAverageFPS * FramesPresented);
#endif
			GXeFGAverageFPS = GAverageFPS * FramesPresented;  // Data may not be accurate without synchronization, but it should be acceptable to make code simple
		}
		break;
	default:
		check(false);
		GXeFGAverageFPS = GAverageFPS;  // Data may not be accurate without synchronization, but it should be acceptable to make code simple
		break;
	}
}
#endif
