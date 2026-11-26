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

#include "XeSSCommonMacros.h"

#if XESS_ENGINE_VERSION_GEQ(5, 3)
#include "TemporalUpscaler.h"
#else
#include "PostProcess/TemporalAA.h"
#include "SceneRendering.h"
#endif

namespace XeSSUnreal
{
#if XESS_ENGINE_VERSION_GEQ(5, 3)
	using XTemporalUpscaler = UE::Renderer::Private::ITemporalUpscaler;
	using XTemporalUpscalerHistory = UE::Renderer::Private::ITemporalUpscaler::IHistory;
	using XTemporalUpscalerInputs = UE::Renderer::Private::ITemporalUpscaler::FInputs;
#else
	using XTemporalUpscaler = ITemporalUpscaler;
	using XTemporalUpscalerHistory = ICustomTemporalAAHistory;
	using XTemporalUpscalerInputs = ITemporalUpscaler::FPassInputs;
#endif

#if XESS_ENGINE_VERSION_GEQ(5, 3)
	using XTemporalUpscalerOutputs = UE::Renderer::Private::ITemporalUpscaler::FOutputs;
#elif XESS_ENGINE_VERSION_GEQ(5, 0)
	using XTemporalUpscalerOutputs = ITemporalUpscaler::FOutputs;
#else
	struct XTemporalUpscalerOutputs {};  // Dummy type to unify code across Unreal versions
#endif
}
