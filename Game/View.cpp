#include "View.h"
#include "AssetManager.h"
#include "RenderPass.h"
#include "Core/Sorting.h"
#include "Platform/Graphics/GraphicsCommon.h"

#include <string.h>

using namespace Tk;
using namespace Platform;
using namespace Core;
using namespace Containers;

void Init(View* view, uint32 maxInstances)
{
    *view = {};
    view->m_maxInstances = maxInstances;
    view->m_numInstances = 0;
    view->m_instances.Resize(maxInstances);
    view->m_instanceData.Resize(maxInstances);

    view->m_instances_sorted.Resize(maxInstances);
    view->m_instanceData_sorted.Resize(maxInstances);
}

CMP_LT_FUNC(CompareLessThan_InstanceByAssetID)
{
    return ((Instance*)A)->m_assetID < ((Instance*)B)->m_assetID;
}

void Update(View* view, Graphics::DescriptorSetDataHandles* descDataHandles)
{
    DescriptorData_Global globalData = {};
    globalData.viewProj = view->m_projMatrix * view->m_viewMatrix;

    // Globals
    Graphics::ResourceHandle bufferHandle = descDataHandles[0].handles[0];
    void* descDataBufferMemPtr_Global = Graphics::MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Global, &globalData, sizeof(DescriptorData_Global));
    Graphics::UnmapResource(bufferHandle);

    // Sort instances for batching of draw calls
    uint32 activeInstanceCounter = 0;
    if (view->m_instanceListDirty)
    {
        view->m_instanceListDirty = 0;

        for (uint32 uiInstance = 0; activeInstanceCounter < view->m_numInstances; ++uiInstance)
        {
            // Copy instance over if active (has not been destroyed)
            const Instance& instance = view->m_instances[uiInstance];
            if (instance.m_flags & Instance::eIsActive)
            {
                memcpy(&view->m_instances_sorted[activeInstanceCounter++], &instance, sizeof(Instance));
            }
        }

        // Sort instances based on asset ID
        Core::MergeSort((Instance*)&view->m_instances_sorted[0], view->m_numInstances, CompareLessThan_InstanceByAssetID);
    }

    // Copy over sorted transforms list
    activeInstanceCounter = 0;
    for (uint32 uiInstance = 0; activeInstanceCounter < view->m_numInstances; ++uiInstance)
    {
        uint32 instanceOrigHandle = view->m_instances_sorted[uiInstance].m_handleToSelf;
        TINKER_ASSERT(instanceOrigHandle <= view->m_maxInstances - 1);
        view->m_instanceData_sorted[activeInstanceCounter++] = view->m_instanceData[instanceOrigHandle];
    }
    const DescriptorData_Instance* instanceData = (const DescriptorData_Instance*)view->m_instanceData_sorted.Data();
    bufferHandle = descDataHandles[1].handles[0];

    void* descDataBufferMemPtr_Instance = Graphics::MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Instance, instanceData, sizeof(DescriptorData_Instance) * view->m_numInstances);
    Graphics::UnmapResource(bufferHandle);
}

uint32 CreateInstance(View* view, uint32 assetID)
{
    TINKER_ASSERT(view->m_numInstances < view->m_maxInstances);

    view->m_instanceListDirty = 1;

    Instance newInstance = {};
    newInstance.m_assetID = assetID;
    newInstance.m_flags |= Instance::eIsActive;

    // Find unused instance ID
    uint32 newInstanceID = view->m_maxInstances;
    for (uint32 i = 0; i < view->m_maxInstances; ++i)
    {
        if (!(view->m_instances[i].m_flags & Instance::eIsActive))
        {
            newInstanceID = i;
            newInstance.m_handleToSelf = newInstanceID;
            break;
        }
        if (i == view->m_maxInstances - 1)
        {
            // If we are here, we exceeded the number of available instances
            TINKER_ASSERT(0);
        }
    }
    view->m_instances[newInstanceID] = newInstance;
    ++view->m_numInstances;

    return newInstanceID;
}

void DestroyInstance(View* view, uint32 instanceID)
{
    TINKER_ASSERT(instanceID <= view->m_maxInstances - 1);
    TINKER_ASSERT(view->m_numInstances > 0);
    TINKER_ASSERT(view->m_instances[instanceID].m_flags & Instance::eIsActive);

    view->m_instanceListDirty = 1;
    view->m_instances[instanceID].m_flags &= ~Instance::eIsActive;
    --view->m_numInstances;
}

void SetInstanceData(View* view, uint32 instanceID, const DescriptorData_Instance* data)
{
    TINKER_ASSERT(instanceID <= view->m_numInstances - 1);
    memcpy(&view->m_instanceData[instanceID], data, sizeof(DescriptorData_Instance));
}

void RecordRenderPassCommands(View* view, GameRenderPass* renderPass, Graphics::GraphicsCommandStream* graphicsCommandStream,
    uint32 shaderID, uint32 blendState, uint32 depthState, Graphics::DescriptorHandle* descriptors)
{
    if (view->m_numInstances > 0)
    {
        uint32 currentAssetID = view->m_instances_sorted[0].m_assetID;
        uint32 currentNumInstances = 1;
        uint32 uiInstance = 1;
        while (uiInstance <= view->m_numInstances)
        {
            bool finalDrawCall = (uiInstance == view->m_numInstances);
            // Scan for changes in asset id among instances, and batch draw calls
            uint32 nextAssetID = TINKER_INVALID_HANDLE;
            if (!finalDrawCall) nextAssetID = view->m_instances_sorted[uiInstance].m_assetID;
            if (finalDrawCall || nextAssetID != currentAssetID)
            {
                // TODO: hard-coded index
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
