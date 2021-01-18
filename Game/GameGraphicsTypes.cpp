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

