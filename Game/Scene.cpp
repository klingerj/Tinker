#include "Scene.h"
#include "InputManager.h"
#include "BindlessSystem.h"

#include "Sorting.h"

void Init(Scene* scene, uint32 maxInstances, InputManager* inputManager)
{
    *scene = {};
    scene->m_maxInstances = maxInstances;
    scene->m_instances.Resize(maxInstances);
    scene->m_instanceData.Resize(maxInstances);

    scene->m_instances_sorted.Resize(maxInstances);
    scene->m_instanceData_sorted.Resize(maxInstances);
}

CMP_LT_FUNC(CompareLessThan_InstanceByAssetID)
{
    return ((Instance*)A)->m_assetID < ((Instance*)B)->m_assetID;
}

void Update(Scene* scene)
{
    // Sort instances for batching of draw calls
    uint32 activeInstanceCounter = 0;
    if (scene->m_instanceListDirty)
    {
        scene->m_instanceListDirty = 0;

        for (uint32 uiInstance = 0; activeInstanceCounter < scene->m_numInstances; ++uiInstance)
        {
            // Copy instance over if active (has not been destroyed)
            const Instance& instance = scene->m_instances[uiInstance];
            if (instance.m_flags & Instance::eIsActive)
            {
                memcpy(&scene->m_instances_sorted[activeInstanceCounter++], &instance, sizeof(Instance));
            }
        }

        // Sort instances based on asset ID
        Tk::Core::MergeSort((Instance*)&scene->m_instances_sorted[0], scene->m_numInstances, CompareLessThan_InstanceByAssetID);
    }

    // Copy over sorted transforms list
    activeInstanceCounter = 0;
    for (uint32 uiInstance = 0; activeInstanceCounter < scene->m_numInstances; ++uiInstance)
    {
        uint32 instanceOrigHandle = scene->m_instances_sorted[uiInstance].m_handleToSelf;
        TINKER_ASSERT(instanceOrigHandle <= scene->m_maxInstances - 1);
        scene->m_instanceData_sorted[activeInstanceCounter++] = scene->m_instanceData[instanceOrigHandle];
    }
    const ShaderDescriptors::InstanceData_Basic* instanceData = (const ShaderDescriptors::InstanceData_Basic*)scene->m_instanceData_sorted.Data();
    const uint32 instanceDataSizeInBytes = sizeof(ShaderDescriptors::InstanceData_Basic) * scene->m_instanceData_sorted.Size();
    const uint32 instanceDataAlignInBytes = alignof(ShaderDescriptors::InstanceData_Basic);
    // Push every model matrix into the constant buffer 
    scene->m_firstInstanceDataByteOffset = BindlessSystem::PushStructIntoConstantBuffer(instanceData, instanceDataSizeInBytes, instanceDataAlignInBytes);
}

uint32 CreateInstance(Scene* scene, uint32 assetID)
{
    TINKER_ASSERT(scene->m_numInstances <= scene->m_maxInstances);

    scene->m_instanceListDirty = 1;

    Instance newInstance = {};
    newInstance.m_assetID = assetID;
    newInstance.m_flags |= Instance::eIsActive;

    // Find unused instance ID
    uint32 newInstanceID = scene->m_maxInstances;
    for (uint32 i = 0; i < scene->m_maxInstances; ++i)
    {
        if (!(scene->m_instances[i].m_flags & Instance::eIsActive))
        {
            newInstanceID = i;
            newInstance.m_handleToSelf = newInstanceID;
            break;
        }
        if (i == scene->m_maxInstances - 1)
        {
            // If we are here, we exceeded the number of available instances
            TINKER_ASSERT(0);
        }
    }
    scene->m_instances[newInstanceID] = newInstance;
    ++scene->m_numInstances;

    return newInstanceID;
}

void DestroyInstance(Scene* scene, uint32 instanceID)
{
    TINKER_ASSERT(instanceID <= scene->m_maxInstances - 1);
    TINKER_ASSERT(scene->m_numInstances > 0);
    TINKER_ASSERT(scene->m_instances[instanceID].m_flags & Instance::eIsActive);

    scene->m_instanceListDirty = 1;
    scene->m_instances[instanceID].m_flags &= ~Instance::eIsActive;
    --scene->m_numInstances;
}

void SetInstanceData(Scene* scene, uint32 instanceID, const ShaderDescriptors::InstanceData_Basic* data)
{
    TINKER_ASSERT(instanceID <= scene->m_numInstances - 1);
    memcpy(&scene->m_instanceData[instanceID], data, sizeof(ShaderDescriptors::InstanceData_Basic));
}
