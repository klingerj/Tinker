#pragma once

#include "Generated/ShaderDescriptors_Reflection.h"

// This system is very in flux and how it will be exactly used
// is not fully clear. It is meant to serve as a central place
// for systems (gameplay, graphics) to reference named data.

struct DataRepository
{
  ShaderDescriptors::AllGlobals ShaderConstants_Globals;
  // TODO other data, tracking for field validity, etc.
};

namespace DataRepo
{
  extern DataRepository g_theDataRepository;

  void Init();
  void FlushShaderConstants();
} //namespace DataRepo
