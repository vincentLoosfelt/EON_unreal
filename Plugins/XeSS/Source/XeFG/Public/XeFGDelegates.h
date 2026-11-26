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

#include "Delegates/Delegate.h"

class FSceneView;

/**
 * Delegate signature for determining whether XeFG operations should be skipped for a given view.
 * Return true to skip XeFG processing for the view, false to allow XeFG processing.
 * 
 * Example usage:
 * 
 *   FXeFGDelegates::ShouldSkipView.BindLambda([](const FSceneView& View) -> bool
 *   {
 *       // Skip XeFG for specific view types (e.g., reflection captures, cinematic sequences)
 *       return View.Family->ViewMode == VMI_ReflectionOverride;
 *   });
 */
DECLARE_DELEGATE_RetVal_OneParam(bool, FXeFGShouldSkipView, const FSceneView&);

class XEFGCORE_API FXeFGDelegates
{
public:
	/**
	 * Delegate called before XeFG operations are executed for a view.
	 * If bound and returns true, XeFG processing will be skipped for that view.
	 */
	static FXeFGShouldSkipView ShouldSkipView;
};
