#pragma once

#include "Allocators.h"
#include "Platform/PlatformGameAPI.h"

namespace Tk
{
namespace Core
{
namespace Graphics
{

struct ShaderManager
{
public:
    ShaderManager() {}
    ~ShaderManager() {}

    void Startup();
    void Shutdown();

    void LoadAllShaders(uint32 windowWidth, uint32 windowHeight);
    void LoadAllShaderResources(uint32 windowWidth, uint32 windowHeight);
    void CreateWindowDependentResources(uint32 newWindowWidth, uint32 newWindowHeight);
    void ReloadShaders(uint32 newWindowWidth, uint32 newWindowHeight);

private:
    bool LoadShader(const char* vertexShaderFileName, const char* fragmentShaderFileName,
        uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID,
        uint32* descLayouts, uint32 numDescLayouts);
    void CreateAllRenderPasses();

    Tk::Memory::LinearAllocator<> shaderBytecodeAllocator;
};

extern ShaderManager g_ShaderManager;

}
}
}
