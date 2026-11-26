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

#include "CoreMinimal.h"  // It is required by *_API macros
#include "Engine/Engine.h"

class IConsoleVariable;

namespace XeSSCommon
{
	constexpr float ON_SCREEN_MESSAGE_DISPLAY_TIME_IN_SECONDS			= 3600.f;

	constexpr int32 ON_SCREEN_MESSAGE_KEY_GROUP_DEFAULT					= 0;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_GROUP_SIZE					= 1000;

	constexpr int32 ON_SCREEN_MESSAGE_KEY_DEFAULT						= -1;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_INCORRECT_SCREEN_PERCENTAGE	= 0;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_ENGINE_VERSION	= 1;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_IN_EDITOR			= 2;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_RHI				= 3;
	constexpr int32 ON_SCREEN_MESSAGE_KEY_UNSUPPORTED_DEFAULT			= 4;

	inline void AddErrorMessageToScreen(const FString& ErrorMessage, int32 Key = ON_SCREEN_MESSAGE_KEY_DEFAULT, int32 KeyGroup = ON_SCREEN_MESSAGE_KEY_GROUP_DEFAULT)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(ON_SCREEN_MESSAGE_KEY_GROUP_SIZE * KeyGroup + Key, ON_SCREEN_MESSAGE_DISPLAY_TIME_IN_SECONDS, FColor::Red, ErrorMessage);
		}
	}

	inline void RemoveMessageFromScreen(int32 Key, int32 KeyGroup = ON_SCREEN_MESSAGE_KEY_GROUP_DEFAULT)
	{
		if (GEngine)
		{
			GEngine->RemoveOnScreenDebugMessage(ON_SCREEN_MESSAGE_KEY_GROUP_SIZE * KeyGroup + Key);
		}
	}

	inline void UpdateMessageOnScreen(const FString& ErrorMessage, int32 Key = ON_SCREEN_MESSAGE_KEY_DEFAULT, bool bShow = true, int32 KeyGroup = ON_SCREEN_MESSAGE_KEY_GROUP_DEFAULT)
	{
		if (bShow)
		{
			AddErrorMessageToScreen(ErrorMessage, Key, KeyGroup);
		}
		else
		{
			RemoveMessageFromScreen(Key, KeyGroup);
		}
	}

	inline const TCHAR* ToString(bool bBool) { return bBool ? TEXT("true") : TEXT("false"); }

	XESSCOMMON_API void PushThirdPartyDllDirectory();
	XESSCOMMON_API void PopThirdPartyDllDirectory();

	/**
	 * Get the file version string from a DLL file.
	 * @param DllFileName The name of the DLL file (e.g., "libxess.dll")
	 * @param OutVersion The output version string
	 * @return true if version was successfully retrieved, false otherwise
	 */
	XESSCOMMON_API bool GetDllFileVersion(const TCHAR* DllFileName, FString& OutVersion);
}
