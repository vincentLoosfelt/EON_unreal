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
// NOTE: keep following order for Unreal 4.27 and below
#include "RHI.h"
#include "RHIResources.h"
#include "DynamicRHI.h"

namespace XeSSCommon
{
	inline bool IsNonValidationD3D12RHI()
	{
		if (!GDynamicRHI)
		{
			return false;
		}
		return FCString::Strcmp(GDynamicRHI->GetName(), TEXT("D3D12")) == 0;
	}

	inline bool IsNonValidationVulkanRHI()
	{
		if (!GDynamicRHI)
		{
			return false;
		}
		return FCString::Strcmp(GDynamicRHI->GetName(), TEXT("Vulkan")) == 0;
	}

	inline bool IsNonValidationD3D11RHI()
	{
		if (!GDynamicRHI)
		{
			return false;
		}
		return FCString::Strcmp(GDynamicRHI->GetName(), TEXT("D3D11")) == 0;
	}
}
