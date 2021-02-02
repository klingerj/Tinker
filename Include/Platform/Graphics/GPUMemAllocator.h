#pragma once

#include "../../../Include/Core/Utilities/Logging.h"
#include "VulkanTypes.h"

#define MAX_GPU_ALLOCS 4096

namespace Tinker
{
namespace Platform
{
namespace Graphics
{

typedef struct gpu_memory_alloc_record
{
    uint32 size;
    uint32 offset;
    uint32 type;
} GPUMemAllocRecord;

enum
{
    eGPUResourceTypeBufferLinear,
    eGPUResourceTypeImageLinear,
    eGPUResourceTypeImageNonLinear, // TODO: use buffer image granularity
    eGPUResourceTypeMax
};

// GPU Memory Allocator API
class GPU_AllocatorBase
{
protected:
    uint32 m_numTotalAllocs = 0;
    GPUMemAllocRecord m_allocs[MAX_GPU_ALLOCS] = {};

public:
    virtual const GPUMemAllocRecord& Alloc(uint32 size, uint32 resourceType) = 0;
    virtual void Dealloc(const GPUMemAllocRecord& record) = 0;
};

class GPU_PassthroughAllocator : public GPU_AllocatorBase
{
public:
    GPU_PassthroughAllocator() {}
    ~GPU_PassthroughAllocator() {}

    const GPUMemAllocRecord& Alloc(uint32 size, uint32 resourceType) override;
    void Dealloc(const GPUMemAllocRecord& record) override;
};

}
}
}
