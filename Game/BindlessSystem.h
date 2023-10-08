#pragma once

#include "Graphics/Common/GraphicsCommon.h"

namespace BindlessSystem
{
    namespace BindlessArrayID
    {
        enum : uint32
        {
            eTexturesSampled, // TODO get a better name 
            eMax
        };
    }

    enum : uint32
    {
        BindlessIndexMax = DESCRIPTOR_BINDLESS_ARRAY_LIMIT,
    };

    void Create();
    void Destroy();
    void ResetFrame();
    void Flush();

    // Returns the index that this resource will be populated at in the bindless descriptor for this frame
    uint32 BindResourceForFrame(Tk::Graphics::ResourceHandle resource, uint32 bindlessArrayID);

    Tk::Graphics::DescriptorHandle GetBindlessDescriptorFromID(uint32 bindlessID);
}
