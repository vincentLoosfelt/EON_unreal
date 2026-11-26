/*******************************************************************************
 * Copyright 2025 Intel Corporation
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

namespace XeSSCommon
{
	/**
	 * Separate logger for XeSS plugin that works in Shipping configuration.
	 * Logs to xess.log file in the same directory as Unreal log files.
	 */
	class XESSCOMMON_API FXeSSLogger
	{
	public:
		/**
		 * Initialize the logger. This should be called once at plugin startup.
		 * Creates/overwrites the xess.log file in the project log directory.
		 * @return true if logger initialized successfully, false otherwise
		 */
		static bool Initialize();

		/**
		 * Log a formatted message to the xess.log file.
		 * @param Format - Format string
		 * @param ... - Format arguments
		 */
		static void Log(const TCHAR* Format, ...);

	private:
		static FCriticalSection LogMutex;
		static FString LogFilePath;
		static bool bInitialized;
	};
}

/**
 * Macro to log messages using XeSSLogger while maintaining compatibility with UE_LOG.
 * This logs to both the XeSS log file (if initialized) and the standard UE log.
 * Usage: XESS_LOG(LogCategory, Verbosity, Format, ...);
 */
#define XESS_LOG(CategoryName, Verbosity, Format, ...) \
	do { \
		UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
		XeSSCommon::FXeSSLogger::Log(TEXT("[%s] ") Format, TEXT(#CategoryName), ##__VA_ARGS__); \
	} while (0)
