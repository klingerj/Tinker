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

    void LoadAllShaders(const Tk::Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight);

private:
    bool LoadShader(const Tk::Platform::PlatformAPIFuncs* platformFuncs,
        const char* vertexShaderFileName, const char* fragmentShaderFileName,
        uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID,
        uint32* descLayouts, uint32 numDescLayouts);

    Tk::Memory::LinearAllocator<> shaderBytecodeAllocator;
};

extern ShaderManager g_ShaderManager;

}
}