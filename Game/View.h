#pragma once

#include "Platform/PlatformGameAPI.h"
#include "Asset/AssetLibrary.h"
#include "Math/VectorTypes.h"
#include "DataStructures/Vector.h"
#include "GraphicsTypes.h"

struct Instance
{
    enum
    {
        eIsActive = 0x1,
    };
    uint32 m_handleToSelf;
    uint32 m_assetID;
    uint32 m_flags;
};

struct View
{
    Tk::Core::Asset::AssetLibrary m_AssetLib;

    alignas(CACHE_LINE) m4f m_viewMatrix;
    alignas(CACHE_LINE) m4f m_projMatrix;
    alignas(CACHE_LINE) m4f m_viewProjMatrix;
    alignas(CACHE_LINE) Tk::Core::Vector<Instance> m_instances;
    Tk::Core::Vector<DescriptorData_Instance> m_instanceData;

    // Sorted copies of instance data
    Tk::Core::Vector<Instance> m_instances_sorted;
    Tk::Core::Vector<DescriptorData_Instance> m_instanceData_sorted;

    uint32 m_maxInstances;
    uint32 m_numInstances;
    uint8 m_instanceListDirty;
};
void Init(View* view, uint32 maxInstances);
void Update(View* view, Tk::Core::Graphics::DescriptorSetDataHandles* descDataHandles);

uint32 CreateInstance(View* view, uint32 assetID);
void DestroyInstance(View* view, uint32 instanceID);
void SetInstanceData(View* view, uint32 instanceID, const DescriptorData_Instance* data);

struct GameRenderPass;
void RecordRenderPassCommands(View* view, GameRenderPass* renderPass, Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream,
    uint32 shaderID, uint32 blendState, uint32 depthState, Tk::Core::Graphics::DescriptorHandle* descriptors);
