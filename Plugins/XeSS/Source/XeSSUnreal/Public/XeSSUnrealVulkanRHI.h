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
class FRHICommandListBase;
class FRHITexture;
class FVulkanDynamicRHI;
typedef struct VkCommandBuffer_T* VkCommandBuffer;
typedef struct VkDevice_T* VkDevice;
typedef struct VkImageView_T* VkImageView;
typedef struct VkImage_T* VkImage;
typedef struct VkInstance_T* VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;

#pragma warning(push)
#pragma warning(disable: 4471)
typedef enum VkFormat VkFormat;
#pragma warning(pop)

namespace XeSSUnreal
{
	XESSUNREAL_API void AddEnabledInstanceExtensionsAndLayers(const TArray<const ANSICHAR*>& InstanceExtensions, const TArray<const ANSICHAR*>& InstanceLayers);
	XESSUNREAL_API void AddEnabledDeviceExtensionsAndLayers(const TArray<const ANSICHAR*>& DeviceExtensions, const TArray<const ANSICHAR*>& DeviceLayers);

	XESSUNREAL_API VkCommandBuffer VulkanRHIGetNativeCommandBuffer(FRHICommandListBase& ExecutingCmdList);
	XESSUNREAL_API VkDevice GetVulkanDevice();
	XESSUNREAL_API VkFormat GetVulkanFormat(FRHITexture* Texture);
	XESSUNREAL_API VkImage GetVulkanImage(FRHITexture* Texture);
	XESSUNREAL_API VkImageView GetVulkanImageView(FRHITexture* Texture);
	XESSUNREAL_API VkInstance GetVulkanInstance();
	XESSUNREAL_API VkPhysicalDevice GetVulkanPhysicalDevice();
	XESSUNREAL_API void VulkanRHIFinishExternalComputeWork(VkCommandBuffer CommandBuffer);
}
#endif
