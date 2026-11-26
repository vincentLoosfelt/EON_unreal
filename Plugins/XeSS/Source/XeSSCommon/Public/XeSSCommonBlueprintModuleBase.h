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

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "XeSSCommonMacros.h"

class FXeSSCommonCoreModuleBase;

class FXeSSCommonBlueprintModuleBase : public IModuleInterface
{
public:
	XESSCOMMON_API void StartupModule() override;
	XESSCOMMON_API void ShutdownModule() override;

private:
	virtual const TCHAR* GetCoreModuleName() const = 0;
	virtual const TCHAR* GetModuleName() const = 0;
	virtual void OnCoreModuleLoaded(FXeSSCommonCoreModuleBase& CoreModule) = 0;
	virtual void OnCoreModuleUnloaded() = 0;

	void OnModulesChanges(FName ModuleName, EModuleChangeReason ReasonForChange);
};

// Module name convention: <FeatureName>Core for the core module and <FeatureName>Blueprint for the blueprint module
// e.g., XeSSCore and XeSSBlueprint
#define XESS_DECLARE_BLUEPRINT_MODULE(FeatureName) \
class F##FeatureName##BlueprintModule : public FXeSSCommonBlueprintModuleBase \
{ \
private: \
	const TCHAR* GetCoreModuleName() const override { return TEXT(XESS_STRINGIFY(FeatureName##Core)); } \
	const TCHAR* GetModuleName() const override { return TEXT(XESS_STRINGIFY(FeatureName##Blueprint)); } \
	void OnCoreModuleLoaded(FXeSSCommonCoreModuleBase& CoreModule) override; \
	void OnCoreModuleUnloaded() override; \
};

#define XESS_IMPLEMENT_BLUEPRINT_MODULE(FeatureName) \
void F##FeatureName##BlueprintModule::OnCoreModuleLoaded(FXeSSCommonCoreModuleBase& CoreModule) \
{ \
	U##FeatureName##BlueprintLibrary::Init(CoreModule); \
} \
void F##FeatureName##BlueprintModule::OnCoreModuleUnloaded() \
{ \
	U##FeatureName##BlueprintLibrary::Deinit(); \
} \
IMPLEMENT_MODULE(F##FeatureName##BlueprintModule, FeatureName##Blueprint)
