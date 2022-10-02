#pragma once

#include "CoreDefines.h"

namespace ShaderCompileErrCode
{
enum : uint32
{
    Success = 0,
    HasWarnings,
    HasErrors,
    Max
};
}

uint32 CompileAllShadersVK();
uint32 CompileAllShadersDX();
