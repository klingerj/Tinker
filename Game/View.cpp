#include "View.h"
#include "Scene.h"
#include "AssetManager.h"
#include "RenderPass.h"
#include "Graphics/Common/GraphicsCommon.h"

void Init(View* view)
{
    *view = {};
}

void Update(View* view, Tk::Core::Graphics::DescriptorSetDataHandles* descDataHandles)
{
    DescriptorData_Global globalData = {};
    globalData.viewProj = view->m_projMatrix * view->m_viewMatrix;

    // Globals
    Tk::Core::Graphics::ResourceHandle bufferHandle = descDataHandles[0].handles[0];
    void* descDataBufferMemPtr_Global = Tk::Core::Graphics::MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Global, &globalData, sizeof(DescriptorData_Global));
    Tk::Core::Graphics::UnmapResource(bufferHandle);
}

// TODO: need to reorganize this, the view doesn't really do anything here
void RecordRenderPassCommands(View* view, Scene* scene, GameRenderPass* renderPass,
    Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 shaderID, uint32 blendState, uint32 depthState,
    Tk::Core::Graphics::DescriptorHandle* descriptors)
{
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
                // TODO: yes, this is kinda bad
                descriptors[2] = g_AssetManager.GetMeshGraphicsDataByID(currentAssetID)->m_descriptor;

                StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(currentAssetID);
                DrawMeshDataCommand(graphicsCommandStream,
                        meshData->m_numIndices,
                        currentNumInstances,
                        meshData->m_indexBuffer.gpuBufferHandle,
                        shaderID,
                        blendState,
                        depthState,
                        descriptors,
                        "Draw asset");

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
