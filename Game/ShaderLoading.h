#pragma once

#include "PlatformGameAPI.h"

#ifdef _SHADERS_SPV_DIR
#define SHADERS_SPV_PATH STRINGIFY(_SHADERS_SPV_DIR)
#else
//#define SHADERS_SPV_PATH "..\\Shaders\\spv\\"
#endif

Tk::Platform::ShaderHandle LoadShader(const Tk::Platform::PlatformAPIFuncs* platformFuncs,
    const char* vertexShaderFileName,
    const char* fragmentShaderFileName,
    Tk::Platform::GraphicsPipelineParams* params);

void FreeShaderBytecodeMemory();
void ResetShaderBytecodeAllocator();
