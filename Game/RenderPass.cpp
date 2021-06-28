#include "RenderPass.h"
#include "AssetManager.h"

#include <string.h>

using namespace Tk;
using namespace Platform;

void DrawMeshDataCommand(GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, ResourceHandle indexBufferHandle, ShaderHandle shaderHandle,
    DescriptorHandle* descriptors, const char* debugLabel)
{
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = debugLabel;

    command->m_numIndices = numIndices;
    command->m_numInstances = numInstances;
    command->m_shaderHandle = shaderHandle;
    command->m_indexBufferHandle = indexBufferHandle;
    memcpy(command->m_descriptors, descriptors, sizeof(DescriptorHandle) * MAX_DESCRIPTOR_SETS_PER_SHADER);
    ++graphicsCommandStream->m_numCommands;
}

void StartRenderPass(GameRenderPass* renderPass, Platform::GraphicsCommandStream* graphicsCommandStream)
{
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = renderPass->debugLabel;
    command->m_framebufferHandle = renderPass->framebuffer;
    command->m_renderWidth = renderPass->renderWidth;
    command->m_renderHeight = renderPass->renderHeight;
    ++graphicsCommandStream->m_numCommands;
    ++command;
}

void EndRenderPass(GameRenderPass* renderPass, Platform::GraphicsCommandStream* graphicsCommandStream)
{
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eRenderPassEnd;
    ++graphicsCommandStream->m_numCommands;
    ++command;
}
