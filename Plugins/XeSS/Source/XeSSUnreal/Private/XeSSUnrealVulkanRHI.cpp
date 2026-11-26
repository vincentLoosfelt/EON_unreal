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

#include "XeSSUnrealVulkanRHI.h"

#if PLATFORM_WINDOWS
#include "XeSSCommonMacros.h"

#include "VulkanRHIPrivate.h"  // Required by VulkanDynamicRHI.h
#include "VulkanContext.h"
#include "VulkanDynamicRHI.h"
#include "VulkanPendingState.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "IVulkanDynamicRHI.h"
#else
#include "VulkanRHIBridge.h"
#endif

#include "XeSSCommonRHI.h"

static FVulkanDynamicRHI* GetVulkanRHI()
{
	check(XeSSCommon::IsNonValidationVulkanRHI());
	return static_cast<FVulkanDynamicRHI*>(GDynamicRHI);
}

namespace XeSSUnreal
{
	void AddEnabledInstanceExtensionsAndLayers(const TArray<const ANSICHAR*>& InstanceExtensions, const TArray<const ANSICHAR*>& InstanceLayers)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		IVulkanDynamicRHI::AddEnabledInstanceExtensionsAndLayers(InstanceExtensions, InstanceLayers);
#else
		VulkanRHIBridge::AddEnabledInstanceExtensionsAndLayers(InstanceExtensions, InstanceLayers);
#endif
	}

	void AddEnabledDeviceExtensionsAndLayers(const TArray<const ANSICHAR*>& DeviceExtensions, const TArray<const ANSICHAR*>& DeviceLayers)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		IVulkanDynamicRHI::AddEnabledDeviceExtensionsAndLayers(DeviceExtensions, DeviceLayers);
#else
		VulkanRHIBridge::AddEnabledDeviceExtensionsAndLayers(DeviceExtensions, DeviceLayers);
#endif
	}

	VkCommandBuffer VulkanRHIGetNativeCommandBuffer(FRHICommandListBase& ExecutingCmdList)
	{
		check(XeSSCommon::IsNonValidationVulkanRHI());

#if XESS_ENGINE_VERSION_GEQ(5, 8)
		return static_cast<IVulkanDynamicRHI*>(GDynamicRHI)->RHIGetVkCommandBuffer(ExecutingCmdList);
#elif XESS_ENGINE_VERSION_GEQ(5, 1)
		return static_cast<IVulkanDynamicRHI*>(GDynamicRHI)->RHIGetActiveVkCommandBuffer();
#else
		return (VkCommandBuffer)GetVulkanRHI()->GetDevice()->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBufferDirect()->GetHandle();
#endif
	}

	VkDevice GetVulkanDevice()
	{
#if XESS_ENGINE_VERSION_GEQ(5, 7)
		return GetVulkanRHI()->GetDevice()->GetHandle();
#else
		return (VkDevice)GetVulkanRHI()->GetDevice()->GetInstanceHandle();
#endif
	}

	VkFormat GetVulkanFormat(FRHITexture* Texture)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 1)
		return static_cast<VkFormat>(static_cast<FVulkanTexture*>(Texture->GetTexture2D())->ViewFormat);
#else
		return static_cast<VkFormat>(static_cast<FVulkanTexture2D*>(Texture->GetTexture2D())->Surface.ViewFormat);
#endif
	}

	VkImage GetVulkanImage(FRHITexture* Texture)
	{
		return (VkImage)Texture->GetNativeResource();
	}

	VkImageView GetVulkanImageView(FRHITexture* Texture)
	{
#if XESS_ENGINE_VERSION_GEQ(5, 3)
		return static_cast<FVulkanTexture*>(Texture->GetTexture2D())->DefaultView->GetTextureView().View;
#elif XESS_ENGINE_VERSION_GEQ(5, 1)
		return static_cast<VkImageView>(static_cast<FVulkanTexture*>(Texture->GetTexture2D())->DefaultView.View);
#else
		return static_cast<VkImageView>(static_cast<FVulkanTexture2D*>(Texture->GetTexture2D())->DefaultView.View);
#endif
	}

	VkInstance GetVulkanInstance()
	{
		return (VkInstance)GetVulkanRHI()->GetInstance();
	}

	VkPhysicalDevice GetVulkanPhysicalDevice()
	{
		return (VkPhysicalDevice)GetVulkanRHI()->GetDevice()->GetPhysicalHandle();
	}

	void VulkanRHIFinishExternalComputeWork(VkCommandBuffer CommandBuffer)
	{
		check(XeSSCommon::IsNonValidationVulkanRHI());

#if XESS_ENGINE_VERSION_GEQ(5, 1)
		static_cast<IVulkanDynamicRHI*>(GDynamicRHI)->RHIFinishExternalComputeWork(CommandBuffer);
#else
		check(CommandBuffer == VulkanRHIGetNativeCommandBuffer(FRHICommandListExecutor::GetImmediateCommandList()));

		FVulkanCommandListContextImmediate& ImmediateContext = GetVulkanRHI()->GetDevice()->GetImmediateContext();
		ImmediateContext.GetPendingComputeState()->Reset();
		ImmediateContext.GetPendingGfxState()->Reset();
#endif
	}
}
#endif
