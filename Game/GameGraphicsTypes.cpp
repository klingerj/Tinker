#include "GameGraphicsTypes.h"

void CopyStagingBufferToGPUBufferCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands,
    ResourceHandle stagingBufferHandle, ResourceHandle gpuBufferHandle, uint32 bufferSizeInBytes,
    const char* debugLabel)
{
    Platform::GraphicsCommand command = {};
    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
    command.debugLabel = debugLabel;

    command.m_sizeInBytes = bufferSizeInBytes;
    command.m_srcBufferHandle = stagingBufferHandle;
    command.m_dstBufferHandle = gpuBufferHandle;
    graphicsCommands.push_back(command);
}

void DrawMeshDataCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands, uint32 numIndices,
    ResourceHandle indexBufferHandle, ResourceHandle positionBufferHandle, ResourceHandle uvBufferHandle,
    ResourceHandle normalBufferHandle, ShaderHandle shaderHandle, Platform::DescriptorSetDescHandles* descriptors,
    const char* debugLabel)
{
    Platform::GraphicsCommand command = {};
    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;
    command.debugLabel = debugLabel;

    command.m_numIndices = numIndices;
    command.m_indexBufferHandle = indexBufferHandle;
    command.m_positionBufferHandle = positionBufferHandle;
    command.m_uvBufferHandle = uvBufferHandle;
    command.m_normalBufferHandle = normalBufferHandle;
    command.m_shaderHandle = shaderHandle;
    memcpy(command.m_descriptors, descriptors, sizeof(command.m_descriptors));
    graphicsCommands.push_back(command);
}
