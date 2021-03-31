#include "GameRenderPass.h"
#include "AssetManager.h"

void DrawMeshDataCommand(Platform::GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    ResourceHandle indexBufferHandle, ResourceHandle positionBufferHandle, ResourceHandle uvBufferHandle,
    ResourceHandle normalBufferHandle, ShaderHandle shaderHandle, Platform::DescriptorSetDescHandles* descriptors,
    const char* debugLabel)
{
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = debugLabel;

    command->m_numIndices = numIndices;
    command->m_indexBufferHandle = indexBufferHandle;
    command->m_positionBufferHandle = positionBufferHandle;
    command->m_uvBufferHandle = uvBufferHandle;
    command->m_normalBufferHandle = normalBufferHandle;
    command->m_shaderHandle = shaderHandle;
    memcpy(command->m_descriptors, descriptors, sizeof(command->m_descriptors));
    ++graphicsCommandStream->m_numCommands;
}

void RecordAllCommands(GameRenderPass* renderPass, const Platform::PlatformAPIFuncs* platformFuncs, Platform::GraphicsCommandStream* graphicsCommandStream)
{
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Start render pass
    command->m_commandType = Platform::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = renderPass->debugLabel;
    command->m_framebufferHandle = renderPass->framebuffer;
    command->m_renderWidth = renderPass->renderWidth;
    command->m_renderHeight = renderPass->renderHeight;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    // Draw calls
    // TODO: for now this just draws every asset that the asset manager stores. This should later only draw assets that are meant for this render pass.
    for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
    {
        StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

        DrawMeshDataCommand(graphicsCommandStream,
            meshData->m_numIndices,
            meshData->m_indexBuffer.gpuBufferHandle,
            meshData->m_positionBuffer.gpuBufferHandle,
            meshData->m_uvBuffer.gpuBufferHandle,
            meshData->m_normalBuffer.gpuBufferHandle,
            renderPass->shader,
            renderPass->descriptors,
            "Draw asset");
    }

    command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
    // End render pass
    command->m_commandType = Platform::GraphicsCmd::eRenderPassEnd;
    ++graphicsCommandStream->m_numCommands;
    ++command;
}
