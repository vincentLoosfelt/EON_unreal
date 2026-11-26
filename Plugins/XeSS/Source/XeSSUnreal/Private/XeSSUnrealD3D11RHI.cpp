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

#include "XeSSUnrealD3D11RHI.h"

#if PLATFORM_WINDOWS
#include "XeSSCommonMacros.h"

#if XESS_ENGINE_VERSION_GEQ(5, 1)
#include "ID3D11DynamicRHI.h"
#else
#include "D3D11RHIPrivate.h"
#endif
#include "XeSSCommonRHI.h"

namespace XeSSUnreal
{

ID3D11Device* D3D11RHIGetDevice()
{
	check(XeSSCommon::IsNonValidationD3D11RHI());

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	// No SLI/Crossfire support required, only 1 GPU node
	return GetDynamicRHI<ID3D11DynamicRHI>()->RHIGetDevice();
#else
	return static_cast<FD3D11DynamicRHI*>(GDynamicRHI)->GetDevice();
#endif
}

ID3D11Resource* D3D11RHIGetResource(FRHITexture* Texture)
{
	check(XeSSCommon::IsNonValidationD3D11RHI());
	check(Texture);

#if XESS_ENGINE_VERSION_GEQ(5, 1)
	return GetDynamicRHI<ID3D11DynamicRHI>()->RHIGetResource(Texture);
#else
	return GetD3D11TextureFromRHITexture(Texture)->GetResource();
#endif
}

}
#endif
