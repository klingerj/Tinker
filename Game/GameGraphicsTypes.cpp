#include "GameGraphicsTypes.h"

void UpdateDynamicBufferCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands, DynamicBuffer* dynamicBuffer, uint32 bufferSizeInBytes)
{
    Platform::GraphicsCommand command;
    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;

    command.m_sizeInBytes = bufferSizeInBytes;
    command.m_srcBufferHandle = dynamicBuffer->stagingBufferHandle;
    command.m_dstBufferHandle = dynamicBuffer->gpuBufferHandle;
    graphicsCommands.push_back(command);
}

void DrawMeshDataCommand(std::vector<Platform::GraphicsCommand>& graphicsCommands, uint32 numIndices,
        uint32 indexBufferHandle, uint32 positionBufferHandle, uint32 normalBufferHandle,
        uint32 shaderHandle, Platform::DescriptorSetDataHandles* descriptors)
{
    Platform::GraphicsCommand command;
    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;

    command.m_numIndices = numIndices;
    command.m_indexBufferHandle = indexBufferHandle;
    command.m_positionBufferHandle = positionBufferHandle;
    command.m_normalBufferHandle = normalBufferHandle;
    //command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = shaderHandle;
    memcpy(command.m_descriptors, descriptors, sizeof(command.m_descriptors));
    graphicsCommands.push_back(command);
}