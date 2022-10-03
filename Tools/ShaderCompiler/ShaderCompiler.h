#pragma once

#include "CoreDefines.h"

namespace ShaderCompileErrCode
{
enum : uint32
{
    Success = 0,
    HasWarnings,
    HasErrors,
    NonShaderError,
    Max
};
}

// 1 means all files compiled cleanly, 0 means failure to compile
// TODO: let someone get the warnings buffer
uint32 CompileAllShadersVK();
uint32 CompileAllShadersDX();
