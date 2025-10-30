#include "DataRepository.h"
#include "Generated/ShaderDescriptors_Reflection.h"
#include "BindlessSystem.h"

namespace DataRepo
{
  DataRepository g_theDataRepository;

  void Init()
  {
    g_theDataRepository = {};
  }

  void FlushShaderConstants()
  {
    // Global constant data must be the first thing submitted to the bindless system 
    const uint32 globalDataByteOffset = BindlessSystem::PushStructIntoConstantBuffer(
      &g_theDataRepository.ShaderConstants_Globals, sizeof(g_theDataRepository.ShaderConstants_Globals), alignof(ShaderDescriptors::AllGlobals));
    TINKER_ASSERT(globalDataByteOffset == 0);
    //(void)firstGlobalDataByteOffset;
  }
}
