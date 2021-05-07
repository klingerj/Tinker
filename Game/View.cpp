#include "View.h"
#include "AssetManager.h"
#include "GameRenderPass.h"

#include <string.h>

using namespace Tinker;
using namespace Core;
using namespace Math;
using namespace Containers;

void Init(View* view)
{
    *view = {};
}

void Update(View* view, DescriptorSetDataHandles* descDataHandles, const Platform::PlatformAPIFuncs* platformFuncs)
{
    DescriptorData_Global globalData = {};
    globalData.viewProj = view->m_projMatrix * view->m_viewMatrix;

    // TODO: set the whole array of instance data
    DescriptorData_Instance* instanceData = &view->m_instanceData[0];

    // Global
    Platform::ResourceHandle bufferHandle = descDataHandles[0].handles[0];
    void* descDataBufferMemPtr_Global = platformFuncs->MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Global, &globalData, sizeof(DescriptorData_Global));
    platformFuncs->UnmapResource(bufferHandle);

    // Instance
    bufferHandle = descDataHandles[1].handles[0];
    void* descDataBufferMemPtr_Instance = platformFuncs->MapResource(bufferHandle);
    memcpy(descDataBufferMemPtr_Instance, instanceData, sizeof(DescriptorData_Instance));
    platformFuncs->UnmapResource(bufferHandle);
}

uint32 CreateInstance(View* view, uint32 assetID)
{
    Instance newInstance = {};
    newInstance.assetID = assetID;

    uint32 newInstanceID = view->m_instances.Size();
    view->m_instances.PushBackRaw(newInstance);

    view->m_instanceData.PushBackRaw({}); // identity matrix
    DescriptorData_Instance data = {};
    data.modelMatrix = m4f(1.0f);
    SetInstanceData(view, newInstanceID, &data);

    return newInstanceID;
}

void SetInstanceData(View* view, uint32 instanceID, const DescriptorData_Instance* data)
{
    memcpy(&view->m_instanceData[instanceID], data, sizeof(DescriptorData_Instance));
}

void RecordRenderPassCommands(View* view, GameRenderPass* renderPass, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream, Platform::DescriptorSetDescHandles* descriptors)
{
    StartRenderPass(renderPass, graphicsCommandStream);

    for (uint32 uiInstance = 0; uiInstance < view->m_instances.Size(); ++uiInstance)
    {
        uint32 assetID = view->m_instances[uiInstance].assetID;
        StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(assetID);

        DrawMeshDataCommand(graphicsCommandStream,
            meshData->m_numIndices,
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

