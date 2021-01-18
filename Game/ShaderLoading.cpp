#include "ShaderLoading.h"
#include "../Include/Core/Allocators.h"

static Memory::LinearAllocator shaderBytecodeAllocator;
const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 10;

using namespace Tinker;

void ResetShaderAllocator()
{
    shaderBytecodeAllocator.Free();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);
}

Platform::ShaderHandle LoadShader(const Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Platform::GraphicsPipelineParams* params)
{
    // get file size, load entire file
    uint32 vertexShaderFileSize = platformFuncs->GetFileSize(vertexShaderFileName);
    uint32 fragmentShaderFileSize = platformFuncs->GetFileSize(fragmentShaderFileName);
    TINKER_ASSERT(vertexShaderFileSize > 0);
    TINKER_ASSERT(fragmentShaderFileSize > 0);

    uint8* vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
    uint8* fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);

    platformFuncs->ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    platformFuncs->ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);

    return platformFuncs->CreateGraphicsPipeline(vertexShaderBuffer, vertexShaderFileSize, fragmentShaderBuffer, fragmentShaderFileSize, params->blendState, params->depthState, params->viewportWidth, params->viewportHeight, params->framebufferHandle, params->descriptorHandle);
}
