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

    /*typedef struct bindless_id
    {
        uint32 uiID; // meant to be passed to the gpu with draw calls. May include validation bits
    } BindlessID;

    BindlessID RegisterResource(Tk::Graphics::ResourceHandle res);
    uint32 QueueDeregisterResource(BindlessID bindlessID);*/
}
