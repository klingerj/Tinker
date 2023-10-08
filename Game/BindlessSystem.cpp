#include "BindlessSystem.h"
#include "Core/DataStructures/Vector.h"

using namespace Tk;

namespace BindlessSystem
{
    static Graphics::DescriptorHandle BindlessDescriptors[BindlessArrayID::eMax]; // API bindless descriptor array objects
    static Tk::Core::Vector<Graphics::ResourceHandle> BindlessDescriptorDataLists[BindlessArrayID::eMax]; // per-frame populated data for each bindless descriptor 
    static uint32 BindlessDescriptorIDs[BindlessArrayID::eMax] = // descriptor ID per bindless descriptor 
    {
        Graphics::DESCLAYOUT_ID_BINDLESS_SAMPLED_TEXTURES
    };
    static uint32 BindlessDescriptorFallbackIDs[BindlessArrayID::eMax] = // fallback resource ID per bindless descriptor 
    {
        Graphics::DefaultTextureID::eBlack2x2
    };
    // TODO static assert 

    Tk::Graphics::DescriptorHandle GetBindlessDescriptorFromID(uint32 bindlessID)
    {
        TINKER_ASSERT(bindlessID < BindlessArrayID::eMax);
        return BindlessDescriptors[bindlessID];
    }

    void ResetFrame()
    {
        for (uint32 i = 0; i < BindlessArrayID::eMax; ++i)
        {
            BindlessDescriptorDataLists[i].Clear();
            BindlessDescriptorDataLists[i].Reserve(DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
        }
    }

    void Create()
    {
        for (uint32 i = 0; i < BindlessArrayID::eMax; ++i)
        {
            TINKER_ASSERT(BindlessDescriptors[i] == Graphics::DefaultDescHandle_Invalid); // no resources should be created yet 
            BindlessDescriptors[i] = Graphics::CreateDescriptor(BindlessDescriptorIDs[i]);
        }
    }

    void Destroy()
    {
        for (uint32 i = 0; i < BindlessArrayID::eMax; ++i)
        {
            if (BindlessDescriptors[i] != Graphics::DefaultDescHandle_Invalid)
            {
                Graphics::DestroyDescriptor(BindlessDescriptors[i]);
                BindlessDescriptors[i] = Graphics::DefaultDescHandle_Invalid;
            }
        }
    }

    void Flush()
    {
        // Pad each list with default/fallback resources, then write to each descriptor object 
        for (uint32 i = 0; i < BindlessArrayID::eMax; ++i)
        {
            Tk::Core::Vector<Graphics::ResourceHandle>& BindlessDescriptorData = BindlessDescriptorDataLists[i];
            const Graphics::ResourceHandle fallbackResource = Graphics::GetDefaultTexture(BindlessDescriptorFallbackIDs[i]).res;

            const uint32 numFallbackResNeeded = DESCRIPTOR_BINDLESS_ARRAY_LIMIT - BindlessDescriptorData.Size();
            for (uint32 uiFallback = 0; uiFallback < numFallbackResNeeded; ++uiFallback)
            {
                BindlessDescriptorData.PushBackRaw(fallbackResource);
            }
            Graphics::WriteDescriptorArray(BindlessDescriptors[i], DESCRIPTOR_BINDLESS_ARRAY_LIMIT, (Graphics::ResourceHandle*)BindlessDescriptorData.Data());
        }
    }

    uint32 BindResourceForFrame(Tk::Graphics::ResourceHandle resource, uint32 bindlessArrayID)
    {
        Tk::Core::Vector<Graphics::ResourceHandle>& BindlessDescriptorData = BindlessDescriptorDataLists[bindlessArrayID];
        const uint32 currentSize = BindlessDescriptorData.Size();
        TINKER_ASSERT(currentSize < DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
        BindlessDescriptorData.PushBackRaw(resource);
        return currentSize;
    }

}
