#pragma once

#include "Graphics/Common/GraphicsCommon.h"

namespace BindlessSystem
{
    namespace BindlessArrayID
    {
        enum : uint32
        {
            eTexturesSampled, // TODO get a better name 
            // ... TODO: more IDs for different texture types, buffers, etc 
            eTexturesEnumEnd = eTexturesSampled, // MUST equal the final texture/buffer enum in this enum
            eConstants, // KEEP this one last, before eMax
            eMax,
            eNumBindlessTextureTypes = eTexturesEnumEnd + 1,
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
    
    // Returns the offset into the constant buffer where the first byte of the data provided was written 
    uint32 PushStructIntoConstantBuffer(const void* data, size_t sizeInBytes, size_t alignment);

    Tk::Graphics::DescriptorHandle GetBindlessDescriptorFromID(uint32 bindlessID);
}
