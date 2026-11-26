
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

#include "XeSSUnrealD3D12RHI.h"

#if PLATFORM_WINDOWS
#include "XeSSCommonMacros.h"
#include "XeSSCommonRHI.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "ID3D12DynamicRHI.h"  // For ID3D12DynamicRHI
#else
#include "D3D12RHIPrivate.h"  // For FD3D12DynamicRHI
#endif

namespace XeSSUnreal
{

ID3D12Device* D3D12RHIGetDevice()
{
	check(XeSSCommon::IsNonValidationD3D12RHI());

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// No SLI/Crossfire support required, only 1 GPU node
	return GetDynamicRHI<ID3D12DynamicRHI>()->RHIGetDevice(0);
#else
	return static_cast<FD3D12DynamicRHI*>(GDynamicRHI)->GetAdapter().GetD3DDevice();
#endif
}

ID3D12Resource* D3D12RHIGetResource(FRHITexture* Texture)
{
	check(XeSSCommon::IsNonValidationD3D12RHI());
	check(Texture);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return GetDynamicRHI<ID3D12DynamicRHI>()->RHIGetResource(Texture);
#else
	return GetD3D12TextureFromRHITexture(Texture)->GetResource()->GetResource();
#endif
}

ID3D12CommandQueue* D3D12RHIGetCommandQueue()
{
	check(XeSSCommon::IsNonValidationD3D12RHI());

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return GetDynamicRHI<ID3D12DynamicRHI>()->RHIGetCommandQueue();
#else
	return static_cast<FD3D12DynamicRHI*>(GDynamicRHI)->RHIGetD3DCommandQueue();
#endif
}

ID3D12GraphicsCommandList* D3D12RHIGetGraphicsCommandList(FRHICommandListBase& ExecutingCmdList)
{
	check(XeSSCommon::IsNonValidationD3D12RHI());

	// No SLI/Crossfire support required, only 1 GPU node
	int DeviceIndex = 0;
#if XESS_ENGINE_VERSION_GEQ(5, 5)
	return GetDynamicRHI<ID3D12DynamicRHI>()->RHIGetGraphicsCommandList(ExecutingCmdList, DeviceIndex);
#elif XESS_ENGINE_VERSION_GEQ(5, 1)
	return GetDynamicRHI<ID3D12DynamicRHI>()->RHIGetGraphicsCommandList(DeviceIndex);
#else
	return static_cast<FD3D12DynamicRHI*>(GDynamicRHI)->GetAdapter().GetDevice(DeviceIndex)->GetCommandContext().CommandListHandle.GraphicsCommandList();
#endif
}

void D3D12RHIFinishExternalComputeWork(FRHICommandListBase& ExecutingCmdList, ID3D12GraphicsCommandList* CommandList)
{
	check(XeSSCommon::IsNonValidationD3D12RHI());
	check(CommandList);

	// No SLI/Crossfire support required, only 1 GPU node
	int DeviceIndex = 0;
#if XESS_ENGINE_VERSION_GEQ(5, 5)
	GetDynamicRHI<ID3D12DynamicRHI>()->RHIFinishExternalComputeWork(ExecutingCmdList, DeviceIndex, CommandList);
#elif XESS_ENGINE_VERSION_GEQ(5, 1)
	GetDynamicRHI<ID3D12DynamicRHI>()->RHIFinishExternalComputeWork(DeviceIndex, CommandList);
#else
	FD3D12CommandContext& CommandContext = static_cast<FD3D12DynamicRHI*>(GDynamicRHI)->GetAdapter().GetDevice(DeviceIndex)->GetCommandContext();
	FD3D12StateCache& StateCache = CommandContext.StateCache;
	StateCache.ForceSetComputeRootSignature();
	StateCache.GetDescriptorCache()->SetCurrentCommandList(CommandContext.CommandListHandle);
#endif
}

#if XESS_ENGINE_WITH_XEFG_API
void TransitionResource(FRHITexture* Texture, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, uint32 SubResource, FRHICommandListBase& ExecutingCmdList)
{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// Use DirectX API directly instead of D3D12DynamicRHI->RHITransitionResource to make it simple and synchronized
	ID3D12GraphicsCommandList* CommandList = XeSSUnreal::D3D12RHIGetGraphicsCommandList(ExecutingCmdList);
	ID3D12Resource* Resource = static_cast<ID3D12Resource*>(Texture->GetNativeResource());
	CD3DX12_RESOURCE_BARRIER Barriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(Resource, StateBefore, StateAfter, SubResource)
	};
	CommandList->ResourceBarrier(1, Barriers);
#else
	check(GDynamicRHI);

	FD3D12DynamicRHI* D3D12RHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
	const FD3D12TextureBase* D3D12Texture = GetD3D12TextureFromRHITexture(Texture);
	FD3D12Device* D3D12Device = D3D12Texture->GetParentDevice();
	#if XESS_ENGINE_VERSION_EQU(5, 0)
	D3D12RHI->TransitionResource(D3D12Device->GetDefaultCommandContext().CommandListHandle, D3D12Texture->GetResource(), StateBefore, StateAfter, SubResource, FD3D12DynamicRHI::ETransitionMode::Apply);
	#else
	D3D12RHI->TransitionResource(D3D12Device->GetDefaultCommandContext().CommandListHandle, D3D12Texture->GetResource(), StateAfter, SubResource);
	#endif
#endif
}
#endif

}
#endif
