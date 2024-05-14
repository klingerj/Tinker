#pragma once

#include "Graphics/Common/GraphicsCommon.h"

namespace BindlessSystem
{
    namespace BindlessArrayID
    {
        enum : uint32
        {
            eTexturesRGBA8Sampled,
            eTexturesRGBA8RW,
            // ... TODO: more IDs for different texture types, buffers, etc 
            eConstants,
            eTexturesEnumEnd = eConstants,
            eMax,
            eNumBindlessTextureTypes = eTexturesEnumEnd,
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
