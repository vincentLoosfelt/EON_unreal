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

#include "XeFGBlueprintLibrary.h"

#include "HAL/IConsoleManager.h"
#include "XeSSCommonCoreModuleBase.h"
#include "XeSSCommonMacros.h"

#if WITH_XEFG
#include "XeFGModule.h"
#include "XeFGRHI.h"
#include "XeFGTypes.h"
#include "XeSSCommonBlueprint.h"
#include "XeSSUnrealCore.h"
#endif

#define XEFG_ENABLE_IN_BP (XESS_ENGINE_WITH_XEFG_API && WITH_XEFG)

// Use this macro only with Blueprint functions that depend on module XeFGCore.
// Note: only check if WITH_XESS to simplify code
#if WITH_XEFG
#define XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName) \
	checkf(bInitialized, TEXT("%s can't be called before module XeFGCore loaded."), TEXT(#FunctionName))
#else
#define XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName)
#endif

static bool bInitialized = false;

#if XEFG_ENABLE_IN_BP
static FXeFGModule* XeFGModule = nullptr;
static FXeFGRHI* XeFGRHI = nullptr;
#endif

static IConsoleVariable* CVarXeFGEnabled = nullptr;
static IConsoleVariable* CVarXeFGMaxInterpolatedFrames = nullptr;
static IConsoleVariable* CVarXeFGMaxInterpolatedFramesSupported = nullptr;
static IConsoleVariable* CVarXeFGUICompositionState = nullptr;

void UXeFGBlueprintLibrary::Init(FXeSSCommonCoreModuleBase& InXeFGModule)
{
	check(FCString::Strcmp(InXeFGModule.GetModuleName(), TEXT("XeFGCore")) == 0);
#if XEFG_ENABLE_IN_BP
	XeFGModule = static_cast<FXeFGModule*>(&InXeFGModule);
	XeFGRHI = XeFGModule->GetXeFGRHI();

	CVarXeFGEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.Enabled"));
	check(CVarXeFGEnabled);
	CVarXeFGMaxInterpolatedFrames = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.MaxInterpolatedFrames"));
	check(CVarXeFGMaxInterpolatedFrames);
	CVarXeFGMaxInterpolatedFramesSupported = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.MaxInterpolatedFramesSupported"));
	check(CVarXeFGMaxInterpolatedFramesSupported);
	CVarXeFGUICompositionState = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeFG.UICompositionState"));
	check(CVarXeFGUICompositionState);
#endif
	bInitialized = true;
}

void UXeFGBlueprintLibrary::Deinit()
{
#if XEFG_ENABLE_IN_BP
	XeFGModule = nullptr;
	XeFGRHI = nullptr;

	CVarXeFGEnabled = nullptr;
	CVarXeFGMaxInterpolatedFrames = nullptr;
	CVarXeFGMaxInterpolatedFramesSupported = nullptr;
	CVarXeFGUICompositionState = nullptr;
#endif
	bInitialized = false;
}

bool UXeFGBlueprintLibrary::IsXeFGSupported()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(IsXeFGSupported);

#if XEFG_ENABLE_IN_BP
	return XeFGModule && XeFGModule->IsFeatureSupported();
#else
	return false;
#endif
}

TArray<EXeFGMode> UXeFGBlueprintLibrary::GetSupportedXeFGModes()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(GetSupportedXeFGModes);

	TArray<EXeFGMode> SupportedModes;
	const UEnum* ModeEnum = StaticEnum<EXeFGMode>();

	for (int32 EnumIndex = 0; EnumIndex < ModeEnum->NumEnums() - 1; ++EnumIndex)
	{
		EXeFGMode XeFGMode = EXeFGMode(ModeEnum->GetValueByIndex(EnumIndex));
		bool bSupported = false;
		switch (XeFGMode)
		{
		case EXeFGMode::Off:
			bSupported = true;  // "Off" is always a supported mode
			break;
		case EXeFGMode::On:
			bSupported = UXeFGBlueprintLibrary::IsXeFGSupported();
			break;
		case EXeFGMode::On2x:
		case EXeFGMode::On3x:
		case EXeFGMode::On4x:
			{
				int32 InterpolatedFrames = (int32)XeFGMode - (int32)EXeFGMode::On;
				bSupported = InterpolatedFrames > 0 && InterpolatedFrames <= CVarXeFGMaxInterpolatedFramesSupported->GetInt();
			}
			break;
		default:
			checkf(false, TEXT("Unexpected XeFG mode: %d"), XeFGMode);
			break;
		}
		if (bSupported)
		{
			SupportedModes.Add(XeFGMode);
		}
	}
	return SupportedModes;
}

EXeFGMode UXeFGBlueprintLibrary::GetXeFGMode()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(GetXeFGMode);

#if XEFG_ENABLE_IN_BP
	EXeFGMode XeFGMode = EXeFGMode::Off;

	switch (CVarXeFGEnabled->GetInt())
	{
	case EXeFGEnabled_On:
		{
			int XeFGMaxInterpolatedFrames = CVarXeFGMaxInterpolatedFrames->GetInt();
			if (XeFGMaxInterpolatedFrames == 0)
			{
				// 0 means "use max supported"
				XeFGMaxInterpolatedFrames = CVarXeFGMaxInterpolatedFramesSupported->GetInt();
			}
			switch (XeFGMaxInterpolatedFrames)
			{
			case 3:
				XeFGMode = EXeFGMode::On4x;
				break;
			case 2:
				XeFGMode = EXeFGMode::On3x;
				break;
			case 1:
				XeFGMode = EXeFGMode::On2x;
				break;
			default:
				checkf(false, TEXT("Unexpected value of r.XeFG.MaxInterpolatedFrames: %d"), XeFGMaxInterpolatedFrames);
				break;
			}
		}
		break;
	case EXeFGEnabled_Off:
	default:
		break;
	}
	return XeFGMode;
#else
	return EXeFGMode::Off;
#endif
}

void UXeFGBlueprintLibrary::SetXeFGMode(EXeFGMode Mode)
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(SetXeFGMode);

#if XEFG_ENABLE_IN_BP
	if (!XeSSCommon::IsValidNonMaxEnumValue(Mode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("SetXeFGMode called with invalid enum value: %d"), Mode), ELogVerbosity::Error);
		return;
	}
	int Enabled = EXeFGEnabled_Off;
	int MaxInterpolatedFrames = 0;

	switch (Mode)
	{
	case EXeFGMode::Off:
		break;
	case EXeFGMode::On:
		Enabled = EXeFGEnabled_On;
		MaxInterpolatedFrames = 1;
		break;
	case EXeFGMode::On2x:
		Enabled = EXeFGEnabled_On;
		MaxInterpolatedFrames = 1;
		break;
	case EXeFGMode::On3x:
		Enabled = EXeFGEnabled_On;
		MaxInterpolatedFrames = 2;
		break;
	case EXeFGMode::On4x:
		Enabled = EXeFGEnabled_On;
		MaxInterpolatedFrames = 3;
		break;
	default:
		checkf(false, TEXT("Unexpected value of mode: %d"), Mode);
		break;
	}
	// Only check MaxInterpolatedFrames support for modes where MaxInterpolatedFrames > 0
	int32 MaxInterpolatedFramesSupported = CVarXeFGMaxInterpolatedFramesSupported->GetInt();
	if (MaxInterpolatedFrames > 0 && MaxInterpolatedFrames > MaxInterpolatedFramesSupported)
	{
		const UEnum* ModeEnum = StaticEnum<EXeFGMode>();
		FString ModeName = TEXT("Unknown");
		if (ModeEnum)
		{
			// Use display name for user-friendly error message (e.g., "2X" instead of "EXeFGMode::On2x")
			FText DisplayNameText = ModeEnum->GetDisplayNameTextByValue(static_cast<int64>(Mode));
			if (!DisplayNameText.IsEmpty())
			{
				ModeName = DisplayNameText.ToString();
			}
		}
		FFrame::KismetExecutionMessage(
			*FString::Printf(TEXT("Unsupported XeFG mode '%s' (value: %d): Requires %d interpolated frames, but hardware supports a maximum of %d interpolated frames"),
				*ModeName, (int32)Mode, MaxInterpolatedFrames, MaxInterpolatedFramesSupported),
			ELogVerbosity::Error);
		return;
	}
	if (CVarXeFGEnabled->GetInt() != Enabled)
	{
		XeSSUnreal::SetWithCurrentPriority(*CVarXeFGEnabled, Enabled);
	}
	if (MaxInterpolatedFrames > 0 && MaxInterpolatedFrames != CVarXeFGMaxInterpolatedFrames->GetInt())
	{
		XeSSUnreal::SetWithCurrentPriority(*CVarXeFGMaxInterpolatedFrames, MaxInterpolatedFrames);
	}
#endif
}

XEFGBLUEPRINT_API bool UXeFGBlueprintLibrary::IfRelaunchRequiredByXeFG()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(IfRelaunchRequiredByXeFG);

#if XEFG_ENABLE_IN_BP
	return XeFGRHI && GetXeFGMode() != EXeFGMode::Off && !XeFGRHI->IfGameViewportUsesXeFGSwapChain();
#else
	return false;
#endif
}

EXeFGUICompositionState UXeFGBlueprintLibrary::GetXeFGUICompositionState()
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(GetXeFGUICompositionState);

#if XEFG_ENABLE_IN_BP
	return (EXeFGUICompositionState)CVarXeFGUICompositionState->GetInt();
#else
	return EXeFGUICompositionState::Disabled;
#endif
}

void UXeFGBlueprintLibrary::SetXeFGUICompositionState(EXeFGUICompositionState State)
{
	XEFG_CHECK_BLUEPRINT_FUNCTION_CALL(SetXeFGUICompositionState);

#if XEFG_ENABLE_IN_BP
	XeSSUnreal::SetWithCurrentPriority(*CVarXeFGUICompositionState, (int32)State);
#endif
}
