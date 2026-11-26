/*******************************************************************************
 * Copyright 2023 Intel Corporation
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
#include "xess/xess.h"

namespace XeSS
{
	// QUALITY EDIT:
	constexpr int32 QUALITY_SETTING_MIN = XESS_QUALITY_SETTING_ULTRA_PERFORMANCE;
	constexpr int32 QUALITY_SETTING_MAX = XESS_QUALITY_SETTING_AA;
	constexpr int32 QUALITY_SETTING_COUNT = QUALITY_SETTING_MAX - QUALITY_SETTING_MIN + 1;

	// Use 16384(D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) to avoid potential API errors,
	// which is required by some legacy XeSS SDK versions when querying optimal input resolution.
	constexpr uint32 OUTPUT_RESOLUTION_MAX_SIDE_LENGTH = 16384;

	inline bool IsValid(xess_quality_settings_t QualitySetting)
	{
		return QualitySetting >= QUALITY_SETTING_MIN && QualitySetting <= QUALITY_SETTING_MAX;
	}

	inline int32 ToIndex(xess_quality_settings_t QualitySetting)
	{
		check(IsValid(QualitySetting));
		return QualitySetting - QUALITY_SETTING_MIN;
	}

	inline int32 ToCVarInt(xess_quality_settings_t QualitySetting)
	{
		check(IsValid(QualitySetting));
		return QualitySetting - QUALITY_SETTING_MIN;
	}

	inline xess_quality_settings_t ToXeSSQualitySetting(int32 CVarInt)
	{
		xess_quality_settings_t QualitySetting = static_cast<xess_quality_settings_t>(QUALITY_SETTING_MIN + CVarInt);

		if (IsValid(QualitySetting))
		{
			return QualitySetting;
		}
		return XESS_QUALITY_SETTING_BALANCED;
	}
}
