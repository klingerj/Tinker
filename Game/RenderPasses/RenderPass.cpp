#include "RenderPass.h"
#include "Game/View.h"
#include "Game/Scene.h"
#include "Game/AssetManager.h"

#include <string.h>

using namespace Tk;

void StartRenderPass(GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    graphicsCommandStream->CmdRenderPassBegin(renderPass->renderWidth, renderPass->renderHeight, renderPass->numColorRTs, renderPass->colorRTs, renderPass->depthRT, renderPass->debugLabel);
    graphicsCommandStream->CmdSetViewport(0.0f, 0.0f, (float)renderPass->renderWidth, (float)renderPass->renderHeight, DEPTH_MIN, DEPTH_MAX, "Set render pass viewport state");
    graphicsCommandStream->CmdSetScissor(0, 0, renderPass->renderWidth, renderPass->renderHeight, "Set render pass scissor state");
}

void EndRenderPass(GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    graphicsCommandStream->CmdRenderPassEnd(renderPass->debugLabel);
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
                StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(currentAssetID);
                
                descriptors[2] = meshData->m_descriptor;

                const uint32 data = instanceCount;
                graphicsCommandStream->CmdPushConstant(shaderID, (uint8*)&data, sizeof(uint32), "Mesh push constant");

                graphicsCommandStream->CmdDraw(meshData->m_numIndices, currentNumInstances, 0, 0, shaderID,
                    blendState, depthState, meshData->m_indexBuffer.gpuBufferHandle, MAX_DESCRIPTOR_SETS_PER_SHADER, descriptors, "Draw asset");
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
