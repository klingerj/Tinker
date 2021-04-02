#pragma once

#include "PlatformGameAPI.h"

using namespace Tinker;

#ifdef _SHADERS_SPV_DIR
#define SHADERS_SPV_PATH STRINGIFY(_SHADERS_SPV_DIR)
#else
//#define SHADERS_SPV_PATH "..\\Shaders\\spv\\"
#endif

Platform::ShaderHandle LoadShader(const Platform::PlatformAPIFuncs* platformFuncs,
    const char* vertexShaderFileName,
    const char* fragmentShaderFileName,
    Platform::GraphicsPipelineParams* params);

void FreeShaderBytecodeMemory();
void ResetShaderBytecodeAllocator();
