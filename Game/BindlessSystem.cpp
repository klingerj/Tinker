#include "BindlessSystem.h"
#include "Core/Allocators.h"
#include "Core/DataStructures/Vector.h"

using namespace Tk;

namespace BindlessSystem
{
  static const uint32 BindlessConstantDataMaxBytes = 1024 * 32;

  // Bindless texture/buffer arrays
  static Graphics::DescriptorHandle
    BindlessDescriptors[BindlessArrayID::eMax]; // API bindless descriptor array objects
  static Tk::Core::Vector<Graphics::ResourceHandle> BindlessDescriptorDataLists
    [BindlessArrayID::eNumBindlessTextureTypes]; // per-frame populated data for each
                                                 // bindless descriptor
  static uint32 BindlessDescriptorIDs[BindlessArrayID::eMax] = // descriptor ID per
                                                               // bindless descriptor
    { Graphics::DESCLAYOUT_ID_BINDLESS_TEXTURES_RGBA8_SAMPLED,
      Graphics::DESCLAYOUT_ID_BINDLESS_TEXTURES_RGBA8_RW,
      // TODO: other texture/buffer entries in here
      Graphics::DESCLAYOUT_ID_BINDLESS_CONSTANTS };
  static uint32
    BindlessDescriptorFallbackIDs[BindlessArrayID::eNumBindlessTextureTypes] = {
      Graphics::DefaultResourceID::eBlack2x2, Graphics::DefaultResourceID::eBlack2x2RW
    };
  // TODO static assert

  // Constant buffer
  static Tk::Core::LinearAllocator BindlessConstantBufferAllocator;
  static Graphics::ResourceHandle BindlessConstantBuffer =
    Graphics::DefaultResHandle_Invalid;

  Tk::Graphics::DescriptorHandle GetBindlessDescriptorFromID(uint32 bindlessID)
  {
    TINKER_ASSERT(bindlessID < BindlessArrayID::eMax);
    return BindlessDescriptors[bindlessID];
  }

  void ResetFrame()
  {
    for (uint32 i = 0; i < ARRAYCOUNT(BindlessDescriptorDataLists); ++i)
    {
      BindlessDescriptorDataLists[i].Clear();
      BindlessDescriptorDataLists[i].Reserve(DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
    }
    BindlessConstantBufferAllocator.ResetState();
  }

  void Create()
  {
    for (uint32 i = 0; i < ARRAYCOUNT(BindlessDescriptors); ++i)
    {
      TINKER_ASSERT(
        BindlessDescriptors[i]
        == Graphics::DefaultDescHandle_Invalid); // no resources should be created yet
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

    Graphics::DescriptorSetDataHandles descHandles = {};
    descHandles.InitInvalid();
    descHandles.handles[0] = BindlessConstantBuffer;
    Graphics::WriteDescriptorSimple(BindlessDescriptors[BindlessArrayID::eConstants],
                                    &descHandles);
  }

  void Destroy()
  {
    for (uint32 i = 0; i < ARRAYCOUNT(BindlessDescriptors); ++i)
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
  }

  void Flush()
  {
    // Pad each list with default/fallback resources, then write to each descriptor object
    for (uint32 i = 0; i < BindlessArrayID::eNumBindlessTextureTypes; ++i)
    {
      Tk::Core::Vector<Graphics::ResourceHandle>& BindlessDescriptorData =
        BindlessDescriptorDataLists[i];
      const Graphics::ResourceHandle fallbackResource =
        Graphics::GetDefaultResource(BindlessDescriptorFallbackIDs[i]).res;

      const uint32 numFallbackResNeeded =
        DESCRIPTOR_BINDLESS_ARRAY_LIMIT - BindlessDescriptorData.Size();
      for (uint32 uiFallback = 0; uiFallback < numFallbackResNeeded; ++uiFallback)
      {
        BindlessDescriptorData.PushBackRaw(fallbackResource);
      }
      Graphics::WriteDescriptorArray(
        BindlessDescriptors[i], DESCRIPTOR_BINDLESS_ARRAY_LIMIT,
        (Graphics::ResourceHandle*)BindlessDescriptorData.Data(),
        Graphics::DescUpdateConfigFlags::Transient);
    }

    // Write constant buffer from cpu to gpu buffer
    const uint32 memcpySizeInBytes = (uint32)BindlessConstantBufferAllocator.Size();
    TINKER_ASSERT(memcpySizeInBytes <= BindlessConstantDataMaxBytes);
    Tk::Graphics::MemoryMappedBufferPtr bufferPtr =
      Tk::Graphics::MapResource(BindlessConstantBuffer);
    bufferPtr.MemcpyInto(BindlessConstantBufferAllocator.Data(), memcpySizeInBytes);
    // TODO: debug functionality where we memset the remaining to zero or something
    Tk::Graphics::UnmapResource(BindlessConstantBuffer);
  }

  uint32 BindResourceForFrame(Tk::Graphics::ResourceHandle resource,
                              uint32 bindlessArrayID)
  {
    Tk::Core::Vector<Tk::Graphics::ResourceHandle>& BindlessDescriptorData =
      BindlessDescriptorDataLists[bindlessArrayID];
    const uint32 currentSize = BindlessDescriptorData.Size();
    TINKER_ASSERT(currentSize < DESCRIPTOR_BINDLESS_ARRAY_LIMIT);
    BindlessDescriptorData.PushBackRaw(resource);
    return currentSize;
  }

  uint32 PushStructIntoConstantBuffer(const void* srcData, size_t sizeInBytes,
                                      size_t alignment)
  {
    TINKER_ASSERT(sizeInBytes <= BindlessConstantDataMaxBytes);
    void* constantsDataPtr =
      BindlessConstantBufferAllocator.Alloc(sizeInBytes, (uint32)alignment);
    memcpy(constantsDataPtr, srcData, sizeInBytes);
    return (uint32)((uint8*)constantsDataPtr
                    - (uint8*)BindlessConstantBufferAllocator.Data());
  }
} //namespace BindlessSystem
