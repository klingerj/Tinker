#pragma once

#include "Core/Allocators.h"
#include "PlatformGameAPI.h"

namespace Tk
{
namespace Platform
{

struct ShaderManager
{
public:
    ShaderManager() {}
    ~ShaderManager() {}

    void Startup();
    void Shutdown();

private:
    void LoadAllShaders(const Tk::Platform::PlatformAPIFuncs* platformFuncs);
    bool LoadShader(const Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Platform::GraphicsPipelineParams* params);

    Tk::Memory::LinearAllocator<> shaderBytecodeAllocator;

};

extern ShaderManager g_ShaderManager;

}
}


