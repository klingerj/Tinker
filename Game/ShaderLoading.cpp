#include "ShaderLoading.h"
#include "Core/Allocators.h"

static Memory::LinearAllocator shaderBytecodeAllocator;
const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 10;

using namespace Tk;

void FreeShaderBytecodeMemory()
{
    shaderBytecodeAllocator.ExplicitFree();
}

void ResetShaderBytecodeAllocator()
{
    FreeShaderBytecodeMemory();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);
}

Platform::ShaderHandle LoadShader(const Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Platform::GraphicsPipelineParams* params)
{
    uint8* vertexShaderBuffer = nullptr;
    uint8* fragmentShaderBuffer = nullptr;
    uint32 vertexShaderFileSize = 0, fragmentShaderFileSize = 0;

    if (vertexShaderFileName)
    {
        vertexShaderFileSize = platformFuncs->GetFileSize(vertexShaderFileName);
        vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
        platformFuncs->ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    }

    if (fragmentShaderFileName)
    {
        fragmentShaderFileSize = platformFuncs->GetFileSize(fragmentShaderFileName);
        fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
        platformFuncs->ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);
    }

    return platformFuncs->CreateGraphicsPipeline(vertexShaderBuffer, vertexShaderFileSize,
        fragmentShaderBuffer, fragmentShaderFileSize,
        params->blendState, params->depthState,
        params->viewportWidth, params->viewportHeight,
        params->framebufferHandle,
        params->descriptorHandles, params->numDescriptorHandles);
}
