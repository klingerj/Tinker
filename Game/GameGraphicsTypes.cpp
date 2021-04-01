#include "GameGraphicsTypes.h"

/*
void CopyStagingBufferToGPUBufferCommand(Tinker::Platform::GraphicsCommandStream* graphicsCommandStream,
    ResourceHandle stagingBufferHandle, ResourceHandle gpuBufferHandle, uint32 bufferSizeInBytes,
    const char* debugLabel)
{
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
    command->m_commandType = Platform::GraphicsCmd::eMemTransfer;
    command->debugLabel = debugLabel;
    command->m_sizeInBytes = bufferSizeInBytes;
    command->m_srcBufferHandle = stagingBufferHandle;
    command->m_dstBufferHandle = gpuBufferHandle;
    ++graphicsCommandStream->m_numCommands;
}
*/
