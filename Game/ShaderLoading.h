#pragma once

#include "../Include/PlatformGameAPI.h"

using namespace Tinker;

Platform::ShaderHandle LoadShader(const Platform::PlatformAPIFuncs* platformFuncs,
    const char* vertexShaderFileName,
    const char* fragmentShaderFileName,
    Platform::GraphicsPipelineParams* params);

void FreeShaderBytecodeMemory();
void ResetShaderBytecodeAllocator();
