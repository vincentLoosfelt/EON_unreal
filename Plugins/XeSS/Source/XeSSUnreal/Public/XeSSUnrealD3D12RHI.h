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

#if PLATFORM_WINDOWS
#include "XeSSCommonMacros.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>  // NOLINT(build/include_order)
#include "Windows/HideWindowsPlatformTypes.h"

class FRHICommandListBase;
class FRHITexture;

namespace XeSSUnreal
{
	XESSUNREAL_API ID3D12Device* D3D12RHIGetDevice();
	XESSUNREAL_API ID3D12Resource* D3D12RHIGetResource(FRHITexture* Texture);

	XESSUNREAL_API ID3D12CommandQueue* D3D12RHIGetCommandQueue();
	XESSUNREAL_API ID3D12GraphicsCommandList* D3D12RHIGetGraphicsCommandList(FRHICommandListBase& ExecutingCmdList);
	XESSUNREAL_API void D3D12RHIFinishExternalComputeWork(FRHICommandListBase& ExecutingCmdList, ID3D12GraphicsCommandList* CommandList);

#if XESS_ENGINE_WITH_XEFG_API
	XESSUNREAL_API void TransitionResource(FRHITexture* Texture, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, uint32 SubResource, FRHICommandListBase& ExecutingCmdList);
#endif
}
#endif
