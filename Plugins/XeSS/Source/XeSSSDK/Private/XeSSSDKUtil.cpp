/*******************************************************************************
 * Copyright 2026 Intel Corporation
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

#include "XeSSSDKUtil.h"

namespace XeSSSDK
{
	const TCHAR* GetMessage(xess_result_t Result)
	{
		switch (Result)
		{
		case XESS_RESULT_SUCCESS:
			return TEXT("XeSS operation was successful");
		case XESS_RESULT_WARNING_NONEXISTING_FOLDER:
			return TEXT("Folder to store dump data doesn't exist. Write operation skipped");
		case XESS_RESULT_WARNING_OLD_DRIVER:
			return TEXT("An old or outdated driver");
		case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE:
			return TEXT("XeSS not supported on the GPU. An SM 6.4 capable GPU is required");
		case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER:
			return TEXT("An unsupported driver");
		case XESS_RESULT_ERROR_UNINITIALIZED:
			return TEXT("Execute called without initialization");
		case XESS_RESULT_ERROR_INVALID_ARGUMENT:
			return TEXT("Invalid argument such as descriptor handles");
		case XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY:
			return TEXT("Not enough available GPU memory");
		case XESS_RESULT_ERROR_DEVICE:
			return TEXT("Device function such as resource or descriptor creation failed");
		case XESS_RESULT_ERROR_NOT_IMPLEMENTED:
			return TEXT("The function is not implemented");
		case XESS_RESULT_ERROR_INVALID_CONTEXT:
			return TEXT("Invalid context");
		case XESS_RESULT_ERROR_OPERATION_IN_PROGRESS:
			return TEXT("Operation not finished yet");
		case XESS_RESULT_ERROR_UNSUPPORTED:
			return TEXT("Operation not supported in current configuration");
		case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY:
			return TEXT("The library cannot be loaded");
		case XESS_RESULT_ERROR_WRONG_CALL_ORDER:
			return TEXT("Call to function done in invalid order");
		case XESS_RESULT_ERROR_UNKNOWN:
			return TEXT("Unknown internal failure");
		default:
			return TEXT("Unknown error code");
		}
	}

	const TCHAR* GetMessage(xell_result_t Result)
	{
		switch (Result)
		{
		case XELL_RESULT_SUCCESS:
			return TEXT("XeLL operation was successful");
		case XELL_RESULT_ERROR_UNSUPPORTED_DEVICE:
			return TEXT("XeLL not supported on the GPU");
		case XELL_RESULT_ERROR_UNSUPPORTED_DRIVER:
			return TEXT("An unsupported driver");
		case XELL_RESULT_ERROR_UNINITIALIZED:
			return TEXT("Execute called without initialization");
		case XELL_RESULT_ERROR_INVALID_ARGUMENT:
			return TEXT("Invalid argument");
		case XELL_RESULT_ERROR_DEVICE:
			return TEXT("Device function such as resource or descriptor creation failed");
		case XELL_RESULT_ERROR_NOT_IMPLEMENTED:
			return TEXT("The function is not implemented");
		case XELL_RESULT_ERROR_INVALID_CONTEXT:
			return TEXT("Invalid context");
		case XELL_RESULT_ERROR_UNSUPPORTED:
			return TEXT("Operation not supported in current configuration");
		case XELL_RESULT_ERROR_UNKNOWN:
			return TEXT("Unknown internal failure");
		default:
			return TEXT("Unknown error code");
		}
	}

	const TCHAR* GetMessage(xefg_swapchain_result_t Result)
	{
		switch (Result)
		{
		case XEFG_SWAPCHAIN_RESULT_SUCCESS:
			return TEXT("Operation was successful");
		case XEFG_SWAPCHAIN_RESULT_WARNING_OLD_DRIVER:
			return TEXT("An old or outdated driver");
		case XEFG_SWAPCHAIN_RESULT_WARNING_TOO_FEW_FRAMES:
			return TEXT("Frame ID should be over 0");
		case XEFG_SWAPCHAIN_RESULT_WARNING_FRAMES_ID_MISMATCH:
			return TEXT("Data for interpolation came in wrong order");
		case XEFG_SWAPCHAIN_RESULT_WARNING_MISSING_PRESENT_STATUS:
			return TEXT("There is no present status for last present");
		case XEFG_SWAPCHAIN_RESULT_WARNING_RESOURCE_SIZES_MISMATCH:
			return TEXT("Resource sizes for the interpolation don't match between previous and next frames. Interpolation skipped");
		case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DEVICE:
			return TEXT("Operation not supported on the GPU");
		case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED_DRIVER:
			return TEXT("An unsupported driver");
		case XEFG_SWAPCHAIN_RESULT_ERROR_UNINITIALIZED:
			return TEXT("Execute called without initialization");
		case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_ARGUMENT:
			return TEXT("Invalid arguments were provided to the API call");
		case XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE_OUT_OF_MEMORY:
			return TEXT("Not enough available GPU memory");
		case XEFG_SWAPCHAIN_RESULT_ERROR_DEVICE:
			return TEXT("Device function such as resource or descriptor creation failed");
		case XEFG_SWAPCHAIN_RESULT_ERROR_NOT_IMPLEMENTED:
			return TEXT("The function is not implemented");
		case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_CONTEXT:
			return TEXT("Invalid context");
		case XEFG_SWAPCHAIN_RESULT_ERROR_OPERATION_IN_PROGRESS:
			return TEXT("Operation not finished yet");
		case XEFG_SWAPCHAIN_RESULT_ERROR_UNSUPPORTED:
			return TEXT("Operation not supported in current configuration");
		case XEFG_SWAPCHAIN_RESULT_ERROR_CANT_LOAD_LIBRARY:
			return TEXT("The library cannot be loaded");
		case XEFG_SWAPCHAIN_RESULT_ERROR_MISMATCH_INPUT_RESOURCES:
			return TEXT("Input resources do not meet requirements based on UI Mode");
		case XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_OUTPUT_RESOURCES:
			return TEXT("Output resources do not meet requirements");
		case XEFG_SWAPCHAIN_RESULT_ERROR_INCORRECT_INPUT_RESOURCES:
			return TEXT("Input is insufficient to evaluate interpolation");
		case XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_UNSUPPORTED:
			return TEXT("Requirements regarding XeLL Latency Reduction are not met");
		case XEFG_SWAPCHAIN_RESULT_ERROR_LATENCY_REDUCTION_FUNCTION_MISSING:
			return TEXT("XeLL library does not contain required functions");
		case XEFG_SWAPCHAIN_RESULT_ERROR_HRESULT_FAILURE:
			return TEXT("One of the Windows functions has failed. For details, see the logs or debug layer");
		case XEFG_SWAPCHAIN_RESULT_ERROR_DXGI_INVALID_CALL:
			return TEXT("DXGI call failed with invalid call error");
		case XEFG_SWAPCHAIN_RESULT_ERROR_POINTER_STILL_IN_USE:
			return TEXT("XeFG SwapChain pointer is still in use during destroy");
		case XEFG_SWAPCHAIN_RESULT_ERROR_INVALID_DESCRIPTOR_HEAP:
			return TEXT("Invalid or missing descriptor heap");
		case XEFG_SWAPCHAIN_RESULT_ERROR_WRONG_CALL_ORDER:
			return TEXT("Call to function done in invalid order");
		case XEFG_SWAPCHAIN_RESULT_ERROR_UNKNOWN:
			return TEXT("Unknown internal failure");
		default:
			return TEXT("Unknown error code");
		}
	}
}
