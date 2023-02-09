#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Graphics
{

namespace GPUTimestamps
{
namespace ID
{
    // TODO: ideally we don't have to have this hard-coded
    enum : uint32
    {
        BeginFrame = 0,
        MainViewZPrepass,
        MainViewRender,
        DebugUI,
        BlitToSwapChain,
        Max
    };
}

    void IncrementTimestampCount();
    void* GetRawCPUSideTimestampBuffer();
    void ProcessTimestamps();

    float GetTimestampValueByID(uint32 timestampID);
}

}
}