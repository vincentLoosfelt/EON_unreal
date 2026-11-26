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

#include "XeSSD3D11RHI.h"

#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "XeSSSDKWrapperD3D11.h"
#include "XeSSUtil.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "xess/xess_d3d11.h"
#include "Windows/HideWindowsPlatformTypes.h"

FXeSSD3D11RHI::FXeSSD3D11RHI()
{
}

FXeSSD3D11RHI::~FXeSSD3D11RHI()
{
}

void FXeSSD3D11RHI::OnXeSSSDKLoaded()
{
	check(XeSSSDKWrapper);

	XeSSSDKWrapperD3D11 = static_cast<FXeSSSDKWrapperD3D11*>(XeSSSDKWrapper.Get());
}

xess_result_t FXeSSD3D11RHI::RHIInitializeXeSS(const FXeSSInitArguments& InArguments, xess_context_handle_t XeSSContext)
{
	xess_d3d11_init_params_t InitParams = {};
	InitParams.outputResolution.x = InArguments.OutputWidth;
	InitParams.outputResolution.y = InArguments.OutputHeight;
	InitParams.initFlags = InArguments.InitFlags;
	InitParams.qualitySetting = XeSS::ToXeSSQualitySetting(InArguments.QualitySetting);

	return XeSSSDKWrapperD3D11->xessD3D11Init(XeSSContext, &InitParams);
}

xess_result_t FXeSSD3D11RHI::RHIExecuteXeSS(const FXeSSExecuteArguments& InArguments, FRHICommandListBase& ExecutingCmdList, xess_context_handle_t XeSSContext)
{
	xess_d3d11_execute_params_t ExecuteParams{};
	ExecuteParams.pColorTexture = XeSSUnreal::D3D11RHIGetResource(InArguments.ColorTexture);
	ExecuteParams.pVelocityTexture = XeSSUnreal::D3D11RHIGetResource(InArguments.VelocityTexture);
	ExecuteParams.pOutputTexture = XeSSUnreal::D3D11RHIGetResource(InArguments.OutputTexture);
	ExecuteParams.jitterOffsetX = InArguments.JitterOffsetX;
	ExecuteParams.jitterOffsetY = InArguments.JitterOffsetY;
	ExecuteParams.resetHistory = InArguments.bCameraCut;
	ExecuteParams.inputWidth = InArguments.SrcViewRect.Width();
	ExecuteParams.inputHeight = InArguments.SrcViewRect.Height();
	ExecuteParams.inputColorBase.x = InArguments.SrcViewRect.Min.X;
	ExecuteParams.inputColorBase.y = InArguments.SrcViewRect.Min.Y;
	ExecuteParams.outputColorBase.x = InArguments.DstViewRect.Min.X;
	ExecuteParams.outputColorBase.y = InArguments.DstViewRect.Min.Y;
	ExecuteParams.exposureScale = 1.0f;

	return XeSSSDKWrapperD3D11->xessD3D11Execute(XeSSContext, &ExecuteParams);
}

xess_result_t FXeSSD3D11RHI::CreateXeSSContext(xess_context_handle_t* OutXeSSContext)
{
	ID3D11Device* Direct3DDevice = XeSSUnreal::D3D11RHIGetDevice();

	check(Direct3DDevice);
	return XeSSSDKWrapperD3D11->xessD3D11CreateContext(Direct3DDevice, OutXeSSContext);
}

xess_result_t FXeSSD3D11RHI::BuildXeSSPipelines(xess_context_handle_t InXeSSContext, uint32_t InInitFlags)
{
	// No XeSS SDK API available
	return XESS_RESULT_SUCCESS;
}
