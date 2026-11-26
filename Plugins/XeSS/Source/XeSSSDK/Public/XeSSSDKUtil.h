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

#include "xell/xell.h"
#include "xess/xess.h"
#include "xess_fg/xefg_swapchain.h"

namespace XeSSSDK
{
	// Valid SDK log level range is [LOG_LEVEL_MIN, LOG_LEVEL_MAX].
	static constexpr int32 LOG_LEVEL_MIN = 0;
	static constexpr int32 LOG_LEVEL_MAX = 3;

	// Named XeSSLogLevel to avoid hiding global declaration of 'LogLevel'.
	inline bool IsLogLevelValid(int32 XeSSLogLevel)
	{
		return XeSSLogLevel >= LOG_LEVEL_MIN && XeSSLogLevel <= LOG_LEVEL_MAX;
	}

	/**
	 * @brief Get a descriptive string message for a xess_result_t error code.
	 * @param Result The XeSS result code
	 * @return Pointer to a human-readable description of the error
	 */
	XESSSDK_API const TCHAR* GetMessage(xess_result_t Result);

	/**
	 * @brief Get a descriptive string message for a xell_result_t error code.
	 * @param Result The XeLL result code
	 * @return Pointer to a human-readable description of the error
	 */
	XESSSDK_API const TCHAR* GetMessage(xell_result_t Result);

	/**
	 * @brief Get a descriptive string message for a xefg_swapchain_result_t error code.
	 * @param Result The XeFG swap chain result code
	 * @return Pointer to a human-readable description of the error
	 */
	XESSSDK_API const TCHAR* GetMessage(xefg_swapchain_result_t Result);
}
