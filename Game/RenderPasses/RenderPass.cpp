#include "RenderPass.h"
#include "Game/View.h"
#include "Game/Scene.h"

#include <string.h>

using namespace Tk;

void StartRenderPass(GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCommand::eRenderPassBegin;
    command->debugLabel = renderPass->debugLabel;

    uint32 numColorRTs = renderPass->numColorRTs;
    command->m_numColorRTs = numColorRTs;
    if (numColorRTs)
        memcpy(command->m_colorRTs, renderPass->colorRTs, sizeof(Graphics::ResourceHandle) * numColorRTs);
    command->m_depthRT = renderPass->depthRT;
    command->m_renderWidth = renderPass->renderWidth;
    command->m_renderHeight = renderPass->renderHeight;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    // Set scissor state
    command->m_commandType = Graphics::GraphicsCommand::eSetScissor;
    command->debugLabel = "Set render pass scissor state";
    command->m_scissorOffsetX = 0;
    command->m_scissorOffsetY = 0;
    command->m_scissorWidth = renderPass->renderWidth;
    command->m_scissorHeight = renderPass->renderHeight;
    ++graphicsCommandStream->m_numCommands;
}

void EndRenderPass(GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCommand::eRenderPassEnd;
    command->debugLabel = renderPass->debugLabel;
    ++graphicsCommandStream->m_numCommands;
}

void DrawMeshDataCommand(Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 numIndices,
    uint32 numInstances, Graphics::ResourceHandle indexBufferHandle,
    uint32 shaderID, uint32 blendState, uint32 depthState,
    Graphics::DescriptorHandle* descriptors, const char* debugLabel)
{
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCommand::eDrawCall;
    command->debugLabel = debugLabel;

    command->m_numIndices = numIndices;
    command->m_numInstances = numInstances;
    command->m_vertOffset = 0;
    command->m_indexOffset = 0;
    command->m_shader = shaderID;
    command->m_blendState = blendState;
    command->m_depthState = depthState;
    command->m_indexBufferHandle = indexBufferHandle;
    memcpy(command->m_descriptors, descriptors, sizeof(Graphics::DescriptorHandle) * MAX_DESCRIPTOR_SETS_PER_SHADER);
    ++graphicsCommandStream->m_numCommands;
}

void RecordRenderPassCommands(GameRenderPass* renderPass, View* view, Scene* scene,
    Tk::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 shaderID, uint32 blendState, uint32 depthState,
    Tk::Graphics::DescriptorHandle* descriptors)
{
    // Track number of instances for proper indexing into uniform buffer of instance data
    uint32 instanceCount = 0;

    if (scene->m_numInstances > 0)
    {
        uint32 currentAssetID = scene->m_instances_sorted[0].m_assetID;
        uint32 currentNumInstances = 1;
        uint32 uiInstance = 1;
        while (uiInstance <= scene->m_numInstances)
        {
            bool finalDrawCall = (uiInstance == scene->m_numInstances);
            // Scan for changes in asset id among instances, and batch draw calls
            uint32 nextAssetID = TINKER_INVALID_HANDLE;
            if (!finalDrawCall) nextAssetID = scene->m_instances_sorted[uiInstance].m_assetID;
            if (finalDrawCall || nextAssetID != currentAssetID)
            {
                // TODO: ideally this would be named
                descriptors[2] = g_AssetManager.GetMeshGraphicsDataByID(currentAssetID)->m_descriptor;

                StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(currentAssetID);

                {
                    Tk::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
                    command->m_commandType = Tk::Graphics::GraphicsCommand::ePushConstant;
                    command->debugLabel = "Push constant";
                    command->m_shaderForLayout = shaderID;
                    {
                        uint8* data = command->m_pushConstantData;
                        memcpy(data, &instanceCount, sizeof(uint32));
                    }
                    ++graphicsCommandStream->m_numCommands;
                }

                DrawMeshDataCommand(graphicsCommandStream,
                    meshData->m_numIndices,
                    currentNumInstances,
                    meshData->m_indexBuffer.gpuBufferHandle,
                    shaderID,
                    blendState,
                    depthState,
                    descriptors,
                    "Draw asset");
                instanceCount += currentNumInstances;

                currentNumInstances = 1;
                currentAssetID = nextAssetID;
            }
            else
            {
                ++currentNumInstances;
            }

            ++uiInstance;
        }
    }
}
