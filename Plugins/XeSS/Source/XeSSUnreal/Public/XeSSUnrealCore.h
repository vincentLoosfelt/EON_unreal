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
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Math/MathFwd.h"
#include "Misc/CoreDelegates.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Templates/RefCounting.h"
#include "XeSSCommonMacros.h"

#if ENGINE_MAJOR_VERSION < 5
using FVector4f = ::FVector4;
using FVector2f = ::FVector2D;
#endif

namespace XeSSUnreal
{
#if ENGINE_MAJOR_VERSION < 5
	using XTSTickerObjectBase = ::FTickerObjectBase;
#else
	using XTSTickerObjectBase = ::FTSTickerObjectBase;
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 8)
	using XRefCountBase = ::FRefCountedObject;
	using XRefCountAddRefReturn = void;
	using XRefCountReleaseReturn = ::FReturnedRefCountValue;
	using XRefCountGetRefCountReturn = ::FReturnedRefCountValue;
#elif XESS_ENGINE_VERSION_GEQ(5, 6)
	using XRefCountBase = ::FRefCountBase;
	using XRefCountAddRefReturn = ::FReturnedRefCountValue;
	using XRefCountReleaseReturn = uint32;
	using XRefCountGetRefCountReturn = uint32;
#else
	using XRefCountBase = ::FRefCountBase;
	using XRefCountAddRefReturn = uint32;
	using XRefCountReleaseReturn = uint32;
	using XRefCountGetRefCountReturn = uint32;
#endif

	inline FSimpleMulticastDelegate& GetOnPostEngineInit()
	{
#if XESS_ENGINE_VERSION_GEQ(5, 8)
		return FCoreDelegates::GetOnPostEngineInit();
#else
		return FCoreDelegates::OnPostEngineInit;
#endif
	}

	XESSUNREAL_API void ApplyCVarSettingsFromIni(const TCHAR* InSectionBaseName, const TCHAR* InIniFilename, uint32 SetBy, bool bAllowCheating = false);

	/**
	 * Helper function to set console variable value with explicit priority bounds.
	 * This wraps SetWithCurrentPriority with explicit MinPriority and MaxPriority values
	 * to satisfy Unreal Engine's console variable priority requirements.
	 * 
	 * @param ConsoleVariable The console variable to modify
	 * @param Value The value to set
	 * @param MinPriority The minimum priority level (must be >= ECVF_SetByScalability), defaults to ECVF_SetByCode
	 */
	template<typename T>
	inline void SetWithCurrentPriority(IConsoleVariable& ConsoleVariable, T Value, EConsoleVariableFlags MinPriority = ECVF_SetByCode)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 7)
		ConsoleVariable.SetWithCurrentPriority(Value, NAME_None, ECVF_SetByConsole, MinPriority);
#else
		ConsoleVariable.SetWithCurrentPriority(Value);
#endif
	}
}
