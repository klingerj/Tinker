#include "GameRenderPass.h"
#include "AssetManager.h"

#include <string.h>

void DrawMeshDataCommand(GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, ResourceHandle indexBufferHandle, ResourceHandle positionBufferHandle,
    ResourceHandle uvBufferHandle, ResourceHandle normalBufferHandle, ShaderHandle shaderHandle,
    DescriptorSetDescHandles* descriptors, const char* debugLabel)
{
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = debugLabel;

    command->m_numIndices = numIndices;
    command->m_numInstances = numInstances;
    command->m_indexBufferHandle = indexBufferHandle;
    command->m_positionBufferHandle = positionBufferHandle;
    command->m_uvBufferHandle = uvBufferHandle;
    command->m_normalBufferHandle = normalBufferHandle;
    command->m_shaderHandle = shaderHandle;
    memcpy(command->m_descriptors, descriptors, sizeof(command->m_descriptors));
    ++graphicsCommandStream->m_numCommands;
}

void StartRenderPass(GameRenderPass* renderPass, Platform::GraphicsCommandStream* graphicsCommandStream)
{
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

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
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eRenderPassEnd;
    ++graphicsCommandStream->m_numCommands;
    ++command;
}

