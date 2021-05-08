#include "View.h"
#include "AssetManager.h"
#include "GameRenderPass.h"
#include "Core/Sorting.h"

#include <string.h>

using namespace Tinker;
using namespace Core;
using namespace Math;
using namespace Containers;

void Init(View* view, uint32 maxInstances)
{
    *view = {};
    view->m_maxInstances = maxInstances;
    view->m_numInstances = 0;
    view->m_instances.Reserve(maxInstances);
    view->m_instanceData.Reserve(maxInstances);

    view->m_instances_sorted.Resize(maxInstances);
    view->m_instanceData_sorted.Resize(maxInstances);
}

bool CompareInstanceByAssetID_LessThan(const Instance& a, const Instance& b)
{
    return a.m_assetID < b.m_assetID;
}

void Update(View* view, DescriptorSetDataHandles* descDataHandles, const Platform::PlatformAPIFuncs* platformFuncs)
{
    DescriptorData_Global globalData = {};
    globalData.viewProj = view->m_projMatrix * view->m_viewMatrix;

    // Global
    Platform::ResourceHandle bufferHandle = descDataHandles[0].handles[0];
    void* descDataBufferMemPtr_Global = platformFuncs->MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Global, &globalData, sizeof(DescriptorData_Global));
    platformFuncs->UnmapResource(bufferHandle);

    // Instance
    {
        // Sort all instances
        // TODO: still need to handle destroyed instances
        // TODO: only copy the array over if the instance vector is dirty
        memcpy(&view->m_instances_sorted[0], view->m_instances.Data(), sizeof(Instance) * view->m_numInstances);
        Core::MergeSort((Instance*)&view->m_instances_sorted[0], view->m_numInstances, CompareInstanceByAssetID_LessThan);
        for (uint32 uiInstance = 0; uiInstance < view->m_instances.Size(); ++uiInstance)
        {
            uint32 instanceOrigHandle = view->m_instances_sorted[uiInstance].m_handleToSelf;
            TINKER_ASSERT(instanceOrigHandle <= view->m_numInstances - 1);
            view->m_instanceData_sorted[uiInstance] = view->m_instanceData[instanceOrigHandle];
        }
    }
    const DescriptorData_Instance* instanceData = &view->m_instanceData_sorted[0];
    bufferHandle = descDataHandles[1].handles[0];

    void* descDataBufferMemPtr_Instance = platformFuncs->MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Instance, instanceData, sizeof(DescriptorData_Instance) * view->m_numInstances);
    platformFuncs->UnmapResource(bufferHandle);
}

uint32 CreateInstance(View* view, uint32 assetID)
{
    TINKER_ASSERT(view->m_numInstances < view->m_maxInstances);
    Instance newInstance = {};
    newInstance.m_assetID = assetID;

    uint32 newInstanceID = view->m_numInstances;
    newInstance.m_handleToSelf = newInstanceID;
    view->m_instances.PushBackRaw(newInstance);
    ++view->m_numInstances;

    view->m_instanceData.PushBackRaw({}); // identity matrix
    DescriptorData_Instance data = {};
    data.modelMatrix = m4f(1.0f);
    SetInstanceData(view, newInstanceID, &data);

    return newInstanceID;
}

void SetInstanceData(View* view, uint32 instanceID, const DescriptorData_Instance* data)
{
    TINKER_ASSERT(instanceID <= view->m_numInstances - 1);
    memcpy(&view->m_instanceData[instanceID], data, sizeof(DescriptorData_Instance));
}

void RecordRenderPassCommands(View* view, GameRenderPass* renderPass, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream, Platform::DescriptorSetDescHandles* descriptors)
{
    StartRenderPass(renderPass, graphicsCommandStream);

    for (uint32 uiInstance = 0; uiInstance < view->m_instances.Size(); ++uiInstance)
    {
        uint32 assetID = view->m_instances[uiInstance].m_assetID;
        StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(assetID);

        DrawMeshDataCommand(graphicsCommandStream,
            meshData->m_numIndices,
            1,
            meshData->m_indexBuffer.gpuBufferHandle,
            meshData->m_positionBuffer.gpuBufferHandle,
            meshData->m_uvBuffer.gpuBufferHandle,
            meshData->m_normalBuffer.gpuBufferHandle,
            renderPass->shader,
            descriptors,
            "Draw asset");
    }

    EndRenderPass(renderPass, graphicsCommandStream);
}

