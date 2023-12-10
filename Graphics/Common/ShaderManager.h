#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Graphics
{
namespace ShaderManager
{
    void Startup();
    void Shutdown();

    void LoadAllShaders();
    void LoadAllShaderResources();
    void ReloadShaders();
}
}
}
