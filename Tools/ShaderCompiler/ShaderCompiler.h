#pragma once

#include "CoreDefines.h"

namespace Tk
{
  namespace ShaderCompiler
  {
    namespace ErrCode
    {
      enum : uint32
      {
        Success = 0,
        HasWarnings,
        HasErrors,
        NonShaderError,
        Max,
      };
    } //namespace ErrCode

    // 1 means all files compiled cleanly, 0 means failure to compile
    // TODO: expose the errors buffer
    uint32 Init();
    uint32 CompileAllShadersVK();
    uint32 CompileAllShadersDX();
  } //namespace ShaderCompiler
} //namespace Tk
