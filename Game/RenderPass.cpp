#include "RenderPass.h"
#include "AssetManager.h"

#include <string.h>

using namespace Tk;
using namespace Core;

void DrawMeshDataCommand(Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, Graphics::ResourceHandle indexBufferHandle,
    uint32 shaderID, uint32 blendState, uint32 depthState,
    Graphics::DescriptorHandle* descriptors, const char* debugLabel)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCmd::eDrawCall;
    command->debugLabel = debugLabel;

    command->m_numIndices = numIndices;
    command->m_numInstances = numInstances;
    command->m_shader = shaderID;
    command->m_blendState = blendState;
    command->m_depthState = depthState;
    command->m_indexBufferHandle = indexBufferHandle;
    memcpy(command->m_descriptors, descriptors, sizeof(Graphics::DescriptorHandle) * MAX_DESCRIPTOR_SETS_PER_SHADER);
    ++graphicsCommandStream->m_numCommands;
}

void StartRenderPass(GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = renderPass->debugLabel;
    command->m_framebufferHandle = renderPass->framebuffer;
    command->m_renderPassID = renderPass->renderPassID;
    command->m_renderWidth = renderPass->renderWidth;
    command->m_renderHeight = renderPass->renderHeight;
    ++graphicsCommandStream->m_numCommands;
    ++command;
}

void EndRenderPass(GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCmd::eRenderPassEnd;
    command->debugLabel = renderPass->debugLabel;
    ++graphicsCommandStream->m_numCommands;
    ++command;
}
