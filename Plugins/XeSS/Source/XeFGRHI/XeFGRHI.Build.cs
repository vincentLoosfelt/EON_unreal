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

using System.IO;
using UnrealBuildTool;

public class XeFGRHI : ModuleRules
{
	public XeFGRHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",

				"XeSSCommon",
				"XeSSUnreal",
			}
		);

		// TODO(sunzhuoshi): refactor to use private dependency module: XeFGCore
		// For XeFGTypes.h
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../XeFG/Public"));

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject",
				"D3D12RHI",
				"Engine",
				"Projects",
				"Renderer",
				"RHI",
				"Slate",
				"SlateCore",

				"XeLLCore",
				"XeSSSDK",
			}
		);

		// For XeLLDelegates.h
		// Note: Relative paths in PrivateIncludePaths are resolved from plugin source directory.
		PrivateIncludePaths.Add("XeLL/Private");

#if !UE_5_3_OR_LATER
		// For SystemTextures.h
		PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"));
#endif

		// Required by XeSSUnrealWindowsD3D12ThirdParty.h
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}
