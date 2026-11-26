/*******************************************************************************
 * Copyright 2021 Intel Corporation
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

#include "XeSSBlueprintLibrary.h"

#include "XeSSCommonBlueprint.h"
#include "XeSSCommonCoreModuleBase.h"

#if WITH_XESS
#include "XeSSModule.h"
#include "XeSSRHI.h"
#include "XeSSUnrealCore.h"
#include "XeSSUpscaler.h"
#include "XeSSUtil.h"
#include "xess/xess.h"
#endif

// Use this macro only with Blueprint functions that depend on module XeSSCore.
// Note: only check if WITH_XESS to simplify code
#if WITH_XESS
#define XESS_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName) \
	checkf(bInitialized, TEXT("%s can't be called before module XeSSCore loaded."), TEXT(#FunctionName))
#else
#define XESS_CHECK_BLUEPRINT_FUNCTION_CALL(FunctionName)
#endif

static bool bInitialized = false;

#if WITH_XESS
static FXeSSModule* XeSSModule = nullptr;
static FXeSSRHI* XeSSRHI = nullptr;
static FXeSSUpscaler* XeSSUpscaler = nullptr;

// QUALITY EDIT:
static TMap<EXeSSQualityMode, xess_quality_settings_t> EnabledQualityMap = {
	{EXeSSQualityMode::UltraPerformance, XESS_QUALITY_SETTING_ULTRA_PERFORMANCE},
	{EXeSSQualityMode::Performance, XESS_QUALITY_SETTING_PERFORMANCE},
	{EXeSSQualityMode::Balanced, XESS_QUALITY_SETTING_BALANCED},
	{EXeSSQualityMode::Quality, XESS_QUALITY_SETTING_QUALITY},
	{EXeSSQualityMode::UltraQuality, XESS_QUALITY_SETTING_ULTRA_QUALITY},
	{EXeSSQualityMode::UltraQualityPlus, XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS},
	{EXeSSQualityMode::AntiAliasing, XESS_QUALITY_SETTING_AA},
};

static xess_quality_settings_t ToXeSSQualitySetting(EXeSSQualityMode QualityMode)
{
	xess_quality_settings_t QualitySetting = XESS_QUALITY_SETTING_BALANCED;

	if (EnabledQualityMap.Contains(QualityMode))
	{
		QualitySetting = EnabledQualityMap[QualityMode];
	}
	else
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeSSQualitySetting called with invalid enum value: %d"),
			static_cast<int32>(QualityMode)), ELogVerbosity::Warning);
	}
	return QualitySetting;
}

static EXeSSQualityMode ToXeSSQualityMode(int32 CVarInt)
{
	xess_quality_settings_t QualitySetting = XeSS::ToXeSSQualitySetting(CVarInt);

	for (const auto& Pair : EnabledQualityMap)
	{
		if (Pair.Value == QualitySetting)
		{
			return Pair.Key;
		}
	}
	FFrame::KismetExecutionMessage(*FString::Printf(TEXT("ToXeSSQualityMode called with invalid value (%d)"),
		CVarInt), ELogVerbosity::Warning);
	return EXeSSQualityMode::Off;
}

#endif

void UXeSSBlueprintLibrary::Init(FXeSSCommonCoreModuleBase& InXeSSModule)
{
	check(FCString::Strcmp(InXeSSModule.GetModuleName(), TEXT("XeSSCore")) == 0);
#if WITH_XESS
	XeSSModule = static_cast<FXeSSModule*>(&InXeSSModule);
	XeSSRHI = XeSSModule->GetXeSSRHI();
	XeSSUpscaler = XeSSModule->GetXeSSUpscaler();
#endif
	bInitialized = true;
}

void UXeSSBlueprintLibrary::Deinit()
{
#if WITH_XESS
	XeSSModule = nullptr;
	XeSSRHI = nullptr;
	XeSSUpscaler = nullptr;
#endif
	bInitialized = false;
}

bool UXeSSBlueprintLibrary::IsXeSSSupported()
{
	XESS_CHECK_BLUEPRINT_FUNCTION_CALL(IsXeSSSupported);

#if WITH_XESS
	return XeSSModule && XeSSModule->IsFeatureSupported();
#else
	return false;
#endif
}

TArray<EXeSSQualityMode> UXeSSBlueprintLibrary::GetSupportedXeSSQualityModes()
{
	TArray<EXeSSQualityMode> SupportedXeSSQualityModes;
	const UEnum* QualityModeEnum = StaticEnum<EXeSSQualityMode>();

	for (int32 EnumIndex = 0; EnumIndex < QualityModeEnum->NumEnums()-1; ++EnumIndex)
	{
		SupportedXeSSQualityModes.Add(EXeSSQualityMode(QualityModeEnum->GetValueByIndex(EnumIndex)));
	}
	return SupportedXeSSQualityModes;
}

EXeSSQualityMode UXeSSBlueprintLibrary::GetXeSSQualityMode()
{
	XESS_CHECK_BLUEPRINT_FUNCTION_CALL(GetXeSSQualityMode);

#if WITH_XESS
	static IConsoleVariable* CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));

	if (!XeSSUpscaler || !XeSSUpscaler->IsXeSSEnabled())
	{
		return EXeSSQualityMode::Off;
	}

	return ToXeSSQualityMode(CVarXeSSQuality->GetInt());
#else
	return EXeSSQualityMode::Off;
#endif
}

EXeSSQualityMode UXeSSBlueprintLibrary::GetDefaultXeSSQualityMode(FIntPoint ScreenResolution)
{
	// For resolutions equal to and lower than 2560x1440 the default quality mode should be set to Balanced
	// Otherwise Performance should be used
	int PixelCount = ScreenResolution.X * ScreenResolution.Y;
	if (PixelCount <= 2560 * 1440)
	{
		return EXeSSQualityMode::Balanced;
	}

	return EXeSSQualityMode::Performance;
}

void UXeSSBlueprintLibrary::SetXeSSQualityMode(EXeSSQualityMode QualityMode)
{
	XESS_CHECK_BLUEPRINT_FUNCTION_CALL(SetXeSSQualityMode);

#if WITH_XESS
	if (!XeSSCommon::IsValidNonMaxEnumValue(QualityMode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("SetXeSSQualityMode called with invalid enum value: %d"),
			static_cast<int32>(QualityMode)), ELogVerbosity::Error);
		return;
	}

	static IConsoleVariable* CVarXeSSEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Enabled"));
	static IConsoleVariable* CVarXeSSQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.XeSS.Quality"));
	static IConsoleVariable* CVarScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));

	if (QualityMode == EXeSSQualityMode::Off)
	{
		XeSSUnreal::SetWithCurrentPriority(*CVarXeSSEnabled, 0);

	#if XESS_ENGINE_VERSION_GEQ(5, 1)
		// Only set if not in editor(no effect by default)
		if (!GIsEditor)
		{
			XeSSUnreal::SetWithCurrentPriority(*CVarScreenPercentage, 100.f);
		}
	#endif

		return;
	}

	XeSSUnreal::SetWithCurrentPriority(*CVarXeSSQuality, XeSS::ToCVarInt(ToXeSSQualitySetting(QualityMode)));
	XeSSUnreal::SetWithCurrentPriority(*CVarXeSSEnabled, 1);

	#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// Only set if not in editor(no effect by default)
	if (!GIsEditor)
	{
		float ScreenPercentage = 100.f;
		if (GetXeSSQualityModeInformation(QualityMode, ScreenPercentage))
		{
			XeSSUnreal::SetWithCurrentPriority(*CVarScreenPercentage, ScreenPercentage);
		}
	}
	#endif

#endif
}

bool UXeSSBlueprintLibrary::GetXeSSQualityModeInformation(EXeSSQualityMode QualityMode, float& ScreenPercentage)
{
	XESS_CHECK_BLUEPRINT_FUNCTION_CALL(GetXeSSQualityModeInformation);

#if WITH_XESS
	if (!XeSSCommon::IsValidNonMaxEnumValue(QualityMode))
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("GetXeSSQualityModeInformation called with invalid enum value:%d"),
			static_cast<int32>(QualityMode)), ELogVerbosity::Error);
		return false;
	}
	if (!XeSSRHI)
	{
		return false;
	}
	if (QualityMode == EXeSSQualityMode::Off) {
		return false;
	}
	ScreenPercentage = XeSSRHI->GetOptimalResolutionFraction(ToXeSSQualitySetting(QualityMode)) * 100.f;
	return true;
#else
	return false;
#endif
}
