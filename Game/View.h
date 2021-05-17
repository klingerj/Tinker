#pragma once

#include "PlatformGameAPI.h"
#include "Core/Math/VectorTypes.h"
#include "Core/Containers/Vector.h"
#include "GameGraphicsTypes.h"

using namespace Tk;
using namespace Core;
using namespace Math;
using namespace Containers;

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
    alignas(CACHE_LINE) m4f m_viewMatrix;
    alignas(CACHE_LINE) m4f m_projMatrix;
    alignas(CACHE_LINE) m4f m_viewProjMatrix;
    alignas(CACHE_LINE) Vector<Instance> m_instances;
    Vector<DescriptorData_Instance> m_instanceData;

    // Sorted copies of instance data
    Vector<Instance> m_instances_sorted;
    Vector<DescriptorData_Instance> m_instanceData_sorted;

    uint32 m_maxInstances;
    uint32 m_numInstances;
    uint8 m_instanceListDirty;
};
void Init(View* view, uint32 maxInstances);
void Update(View* view, DescriptorSetDataHandles* descDataHandles, const Platform::PlatformAPIFuncs* platformFuncs);

uint32 CreateInstance(View* view, uint32 assetID);
void DestroyInstance(View* view, uint32 instanceID);
void SetInstanceData(View* view, uint32 instanceID, const DescriptorData_Instance* data);

struct GameRenderPass;
void RecordRenderPassCommands(View* view, GameRenderPass* renderPass, Tk::Platform::GraphicsCommandStream* graphicsCommandStream, Platform::DescriptorSetDescHandles* descriptors);
