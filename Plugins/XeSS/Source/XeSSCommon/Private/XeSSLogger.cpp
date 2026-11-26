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

#include "XeSSLogger.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "XeSSUnrealCoreIncludes.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "XeSSCommonMacros.h"

namespace XeSSCommon
{
	FCriticalSection FXeSSLogger::LogMutex;
	FString FXeSSLogger::LogFilePath;
	bool FXeSSLogger::bInitialized = false;

	bool FXeSSLogger::Initialize()
	{
		FScopeLock Lock(&LogMutex);

		if (bInitialized)
		{
			return true;
		}

		// Get the log directory path (same as Unreal log files)
		FString LogDir = FPaths::ProjectLogDir();
		LogFilePath = FPaths::Combine(LogDir, TEXT("xess.log"));

		// Try to write the initial log file
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Try to create/overwrite the file
		TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenWrite(*LogFilePath, false, false));
		if (!FileHandle)
		{
			// File might be locked/occupied, skip initialization
			// This is not an error - we just won't log to file
			bInitialized = false;
			return false;
		}

		// Write header
		FString Header = FString::Printf(TEXT("XeSS Plugin Log - %s\n"), *FDateTime::Now().ToString());
		FTCHARToUTF8 Converter(*Header);
		FileHandle->Write((const uint8*)Converter.Get(), Converter.Length());
		FileHandle->Flush();

		bInitialized = true;
		return true;
	}

	void FXeSSLogger::Log(const TCHAR* Format, ...)
	{
		FScopeLock Lock(&LogMutex);
		if (!bInitialized)
		{
			return;
		}
		// Format the message
		TCHAR Buffer[4096];
#if XESS_ENGINE_VERSION_GEQ(5, 4)
		GET_TYPED_VARARGS(TCHAR, Buffer, UE_ARRAY_COUNT(Buffer), UE_ARRAY_COUNT(Buffer) - 1, Format, Format);
#else
		// Disable deprecation warning for GET_VARARGS in UE 5.3 and below
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		GET_VARARGS(Buffer, UE_ARRAY_COUNT(Buffer), UE_ARRAY_COUNT(Buffer) - 1, Format, Format);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif

		// Add timestamp and newline
		FString Message = FString::Printf(TEXT("[%s] %s\n"), *FDateTime::Now().ToString(TEXT("%Y.%m.%d-%H.%M.%S")), Buffer);

		// Append to file
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenWrite(*LogFilePath, true, false));

		if (FileHandle)
		{
			FTCHARToUTF8 Converter(*Message);
			FileHandle->Seek(FileHandle->Size());  // Seek to end for append
			FileHandle->Write((const uint8*)Converter.Get(), Converter.Length());
			FileHandle->Flush();
		}
		// If we fail to write, we silently skip (file might be locked)
	}
}
