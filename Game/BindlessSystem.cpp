#include "BindlessSystem.h"
#include "Core/DataStructures/Vector.h"
#include "Core/Allocators.h"

using namespace Tk;

namespace BindlessSystem
{
    static const uint32 BindlessConstantDataMaxBytes = 1024 * 32;

    // Bindless texture/buffer arrays
    static Graphics::DescriptorHandle BindlessDescriptors[BindlessArrayID::eMax]; // API bindless descriptor array objects
    static Tk::Core::Vector<Graphics::ResourceHandle> BindlessDescriptorDataLists[BindlessArrayID::eMax]; // per-frame populated data for each bindless descriptor 
    static uint32 BindlessDescriptorIDs[BindlessArrayID::eMax] = // descriptor ID per bindless descriptor 
    {
        Graphics::DESCLAYOUT_ID_BINDLESS_SAMPLED_TEXTURES
    };
    static uint32 BindlessDescriptorFallbackIDs[BindlessArrayID::eMax] = // fallback resource ID per bindless descriptor 
    {
        Graphics::DefaultResourceID::eBlack2x2
    };
    // TODO static assert 

    // Constant buffer
    static Tk::Core::LinearAllocator BindlessConstantBufferAllocator;
    static Graphics::DescriptorHandle BindlessConstantBufferDescriptor = Graphics::DefaultDescHandle_Invalid;
    static Graphics::ResourceHandle BindlessConstantBuffer = Graphics::DefaultResHandle_Invalid;

    Tk::Graphics::DescriptorHandle GetBindlessConstantBufferDescriptor()
    {
        return BindlessConstantBufferDescriptor;
    }

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
        BindlessConstantBufferAllocator.ResetState();
    }

    void Create()
    {
        for (uint32 i = 0; i < BindlessArrayID::eMax; ++i)
        {
            TINKER_ASSERT(BindlessDescriptors[i] == Graphics::DefaultDescHandle_Invalid); // no resources should be created yet 
            BindlessDescriptors[i] = Graphics::CreateDescriptor(BindlessDescriptorIDs[i]);
        }
        
        // Constant buffer
        BindlessConstantBufferAllocator.Init(BindlessConstantDataMaxBytes, 16);

        Graphics::ResourceDesc desc = {};
        desc.resourceType = Graphics::ResourceType::eBuffer1D;
        desc.debugLabel = "Bindless constant buffer";
        desc.bufferUsage = Graphics::BufferUsage::eTransient;
        desc.dims = v3ui(BindlessConstantDataMaxBytes, 1, 1);
        TINKER_ASSERT(BindlessConstantBuffer == Graphics::DefaultResHandle_Invalid);
        BindlessConstantBuffer = Graphics::CreateResource(desc);

        TINKER_ASSERT(BindlessConstantBufferDescriptor == Graphics::DefaultDescHandle_Invalid);
        BindlessConstantBufferDescriptor = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_BINDLESS_CONSTANTS);
        Graphics::DescriptorSetDataHandles descHandles = {};
        descHandles.InitInvalid();
        descHandles.handles[0] = BindlessConstantBuffer;
        Graphics::WriteDescriptorSimple(BindlessConstantBufferDescriptor, &descHandles);
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

        BindlessConstantBufferAllocator.ExplicitFree();

        Graphics::DestroyResource(BindlessConstantBuffer);
        BindlessConstantBuffer = Graphics::DefaultResHandle_Invalid;

        Graphics::DestroyDescriptor(BindlessConstantBufferDescriptor);
        BindlessConstantBufferDescriptor = Graphics::DefaultDescHandle_Invalid;
    }

    void Flush()
    {
        // Pad each list with default/fallback resources, then write to each descriptor object 
        for (uint32 i = 0; i < BindlessArrayID::eMax; ++i)
        {
            Tk::Core::Vector<Graphics::ResourceHandle>& BindlessDescriptorData = BindlessDescriptorDataLists[i];
            const Graphics::ResourceHandle fallbackResource = Graphics::GetDefaultResource(BindlessDescriptorFallbackIDs[i]).res;

            const uint32 numFallbackResNeeded = DESCRIPTOR_BINDLESS_ARRAY_LIMIT - BindlessDescriptorData.Size();
            for (uint32 uiFallback = 0; uiFallback < numFallbackResNeeded; ++uiFallback)
            {
                BindlessDescriptorData.PushBackRaw(fallbackResource);
            }
            Graphics::WriteDescriptorArray(BindlessDescriptors[i], DESCRIPTOR_BINDLESS_ARRAY_LIMIT, (Graphics::ResourceHandle*)BindlessDescriptorData.Data(), Graphics::DescUpdateConfigFlags::Transient);
        }

        // Write constant buffer from cpu to gpu buffer
        const uint32 memcpySizeInBytes = (uint32)BindlessConstantBufferAllocator.Size();
        TINKER_ASSERT(memcpySizeInBytes <= BindlessConstantDataMaxBytes);
        Tk::Graphics::MemoryMappedBufferPtr bufferPtr = Tk::Graphics::MapResource(BindlessConstantBuffer);
        bufferPtr.MemcpyInto(BindlessConstantBufferAllocator.Data(), memcpySizeInBytes);
        // TODO: debug functionality where we write the entire capacity, or memset the remaining to zero or something 
        Tk::Graphics::UnmapResource(BindlessConstantBuffer);
    }

    uint32 BindResourceForFrame(Tk::Graphics::ResourceHandle resource, uint32 bindlessArrayID)
    {
        Tk::Core::Vector<Tk::Graphics::ResourceHandle>& BindlessDescriptorData = BindlessDescriptorDataLists[bindlessArrayID];
        const uint32 currentSize = BindlessDescriptorData.Size();
        TINKER_ASSERT(currentSize < DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
        BindlessDescriptorData.PushBackRaw(resource);
        return currentSize;
    }

    uint32 PushStructIntoConstantBuffer(const void* srcData, size_t sizeInBytes, size_t alignment)
    {
        TINKER_ASSERT(sizeInBytes <= BindlessConstantDataMaxBytes);
        void* constantsDataPtr = BindlessConstantBufferAllocator.Alloc(sizeInBytes, (uint32)alignment);
        memcpy(constantsDataPtr, srcData, sizeInBytes);
        return (uint32)((uint8*)constantsDataPtr - (uint8*)BindlessConstantBufferAllocator.Data());
    }

}
