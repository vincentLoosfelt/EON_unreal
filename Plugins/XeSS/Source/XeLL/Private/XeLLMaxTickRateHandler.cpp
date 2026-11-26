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

#include "XeLLMaxTickRateHandler.h"

#if XESS_ENGINE_WITH_XEFG_API
#include "XeLLModule.h"
#include "XeSSSDKUtil.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xell/xell.h"
#include "xell/xell_d3d12.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"

#include "RHI.h"
#include "Engine/Console.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "RenderingThread.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "XeSSUnrealRHI.h"

int32 DisableXeLLTickRateHandler = 0;
static FAutoConsoleVariableRef CVarDisableXeLLTickRateHandler(
	TEXT("r.XeLL.DisableTickRateHandler"),
	DisableXeLLTickRateHandler,
	TEXT("Disable XeLL Tick Rate Handler")
);

static TAutoConsoleVariable<int> CVarXeLLMinIntervalUS(
	TEXT("r.XeLL.MinIntervalUS"),
	-1,
	TEXT("The XeLL minimum interval time in us. -1: using engine default > 0: override engine settings"),
	ECVF_RenderThreadSafe);

DEFINE_LOG_CATEGORY_STATIC(LogMaxTickRateHandler, Log, All);

FXeLLMaxTickRateHandler::FXeLLMaxTickRateHandler(xell_context_handle_t InXeLLContext) : XeLLContext(InXeLLContext)
{
}

void FXeLLMaxTickRateHandler::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
}

bool FXeLLMaxTickRateHandler::GetEnabled()
{
	if (DisableXeLLTickRateHandler != 0)
		return false;

	return bEnabled;
}

bool FXeLLMaxTickRateHandler::GetAvailable()
{
	if (DisableXeLLTickRateHandler != 0)
		return false;

	return true;
}

#if !defined(UE_ALLOW_EXEC_COMMANDS) || UE_ALLOW_EXEC_COMMANDS
bool FXeLLMaxTickRateHandler::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;
	FString XeLLMode;
	APlayerController* PlayerController = (InWorld ? UGameplayStatics::GetPlayerController(InWorld, 0) : nullptr);
	ULocalPlayer* LocalPlayer = (PlayerController ? Cast<ULocalPlayer>(PlayerController->Player) : nullptr);

	if (FParse::Value(Cmd, TEXT("XeLLMode="), XeLLMode))
	{
		if (XeLLMode == "0")
		{
			SetEnabled(false);

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->ViewportConsole)
			{
				LocalPlayer->ViewportClient->ViewportConsole->OutputText("XeLL Low Latency mode: Off");
			}

			UE_LOG(LogMaxTickRateHandler, Log, TEXT("XeLL Low Latency mode: Off"));
		}
		else if (XeLLMode == "1")
		{
			SetEnabled(true);
			SetFlags(1);

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->ViewportConsole)
			{
				LocalPlayer->ViewportClient->ViewportConsole->OutputText("XeLL Low Latency mode: On");
			}

			UE_LOG(LogMaxTickRateHandler, Log, TEXT("XeLL Low Latency mode: On"));
		}
		else
		{
			UE_LOG(LogMaxTickRateHandler, Warning, TEXT("Please set the right value for XeLL mode, 0: off, 1: low latency enabled"));
		}

		bHandled = true;
	}
	else if (FParse::Command(&Cmd, TEXT("XeLLModeToggle")))
	{
		bool bLowLatencyModeEnabled = GetEnabled();

		if (bLowLatencyModeEnabled)
		{
			SetEnabled(false);

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->ViewportConsole)
			{
				LocalPlayer->ViewportClient->ViewportConsole->OutputText("XeLL Low Latency mode: Off");
			}

			UE_LOG(LogMaxTickRateHandler, Log, TEXT("XeLL Low Latency mode: Off"));
		}
		else
		{
			SetEnabled(true);
			SetFlags(1);

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->ViewportConsole)
			{
				LocalPlayer->ViewportClient->ViewportConsole->OutputText("XeLL Low Latency mode: On");
			}

			UE_LOG(LogMaxTickRateHandler, Log, TEXT("XeLL Low Latency mode: On"));
		}

		bHandled = true;
	}

	return bHandled;
}
#endif

// Used to provide a generic customization interface for custom tick rate handlers
void FXeLLMaxTickRateHandler::SetFlags(uint32 Flags)
{
	CustomFlags = Flags;
	if ((Flags & 1) > 0)
	{
		bLowLatencyMode = true;
	}
	else
	{
		bLowLatencyMode = false;
	}
	if ((Flags & 2) > 0)
	{
		bBoost = true;
	}
	else
	{
		bBoost = false;
	}
}

uint32 FXeLLMaxTickRateHandler::GetFlags()
{
	return CustomFlags;
}

bool FXeLLMaxTickRateHandler::HandleMaxTickRate(float DesiredMaxTickRate)
{
	if (GetAvailable())
	{
		int XeLLMinIntervalUS = CVarXeLLMinIntervalUS.GetValueOnAnyThread();
		const float DesiredMinimumInterval = XeLLMinIntervalUS < 0 ? (DesiredMaxTickRate > 0 ? ((1000.0f / DesiredMaxTickRate) * 1000.0f) : 0.0f) : XeLLMinIntervalUS;
		if (bEnabled)
		{
			xell_result_t Result;
			if (MinimumInterval != DesiredMinimumInterval || LastCustomFlags != CustomFlags)
			{
				if (LastCustomFlags != CustomFlags)
				{
					FlushRenderingCommands();
				}

				xell_sleep_params_t params = { 0 };
				params.bLowLatencyMode = bLowLatencyMode;
				params.bLowLatencyBoost = bBoost;
				MinimumInterval = DesiredMinimumInterval;
				params.minimumIntervalUs = MinimumInterval;
				Result = xellSetSleepMode(XeLLContext, &params);

				// Need to verify that Low Latency mode change actually applied successfully
				if (bLowLatencyMode && XESS_FAILED(Result))
				{
					UE_LOG(LogMaxTickRateHandler, Warning, TEXT("Unable to turn on XeLL low latency, mode: %d, boost: %d, interval: %d, result: %d, %s"), params.bLowLatencyMode, params.bLowLatencyBoost, params.minimumIntervalUs, Result, XeSSSDK::GetMessage(Result));
					// Clear the ULL flag
					CustomFlags = CustomFlags & ~1;
					bLowLatencyMode = false;
				}

				LastCustomFlags = CustomFlags;
				bWasEnabled = true;
			}
		}
		else
		{
			// When disabled, if we ever called SetSleepMode, we need to clean up after ourselves
			if (bWasEnabled || MinimumInterval != DesiredMinimumInterval)
			{
				if (bWasEnabled)
				{
					FlushRenderingCommands();
				}

				xell_result_t Result;
				xell_sleep_params_t params = { 0 };
				params.bLowLatencyMode = false;
				params.bLowLatencyBoost = false;
				MinimumInterval = DesiredMinimumInterval;
				params.minimumIntervalUs = MinimumInterval;
				Result = xellSetSleepMode(XeLLContext, &params);
				if (XESS_FAILED(Result))
				{
					UE_LOG(LogMaxTickRateHandler, Warning, TEXT("XeLL sleep mode cleanup failed, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
				}

				// Reset module back to default values in case re-enabled in the same session
				LastCustomFlags = 0;
				bWasEnabled = false;
				bLowLatencyMode = true;
				bBoost = false;
			}
		}
		// Always call xellSleep per XeLL developer guide
		xell_result_t Result = xellSleep(XeLLContext, GFrameCounter);
		if (XESS_FAILED(Result))
		{
			UE_LOG(LogMaxTickRateHandler, Warning, TEXT("Unable to run XeLL low latency sleep, result: %d, %s"), Result, XeSSSDK::GetMessage(Result));
		}
		return true;
	}

	return false;
}
#endif
