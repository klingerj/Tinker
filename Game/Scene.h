#pragma once

#include "DataStructures/Vector.h"
#include "Generated/ShaderDescriptors_Reflection.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "GraphicsTypes.h"

struct InputManager;

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

struct Scene
{
  Tk::Core::Vector<Instance> m_instances;
  Tk::Core::Vector<ShaderDescriptors::InstanceData_Basic> m_instanceData;

  // Sorted copies of instance data
  Tk::Core::Vector<Instance> m_instances_sorted;
  Tk::Core::Vector<ShaderDescriptors::InstanceData_Basic> m_instanceData_sorted;
  uint32 m_firstInstanceDataByteOffset = 0;

  uint32 m_maxInstances;
  uint32 m_numInstances;
  uint8 m_instanceListDirty;
};

void Init(Scene* scene, uint32 maxInstances, InputManager* inputManager);
void Update(Scene* scene);

uint32 CreateInstance(Scene* scene, uint32 assetID);
void DestroyInstance(Scene* scene, uint32 instanceID);
void SetInstanceData(Scene* scene, uint32 instanceID,
                     const ShaderDescriptors::InstanceData_Basic* data);
