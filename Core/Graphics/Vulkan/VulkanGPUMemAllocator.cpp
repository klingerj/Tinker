#include "VulkanGPUMemAllocator.h"

#include <vulkan/vulkan.h>

namespace Tk
{
namespace Platform
{
namespace Graphics
{

// Passthrough allocator
const GPUMemAllocRecord& GPU_PassthroughAllocator::Alloc(uint32 size, uint32 resourceType)
{
    GPUMemAllocRecord& record = m_allocs[m_numTotalAllocs++];
    record.size = size;
    record.offset = 0; // TODO: get this?

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = size;
    memAllocInfo.memoryTypeIndex = 0 /*memTypeIndex*/; // TODO: get this?

    VkResult result = VK_SUCCESS;// vkAllocateMemory(device, &allocInfo, nullptr, deviceMemory);
    if (result != VK_SUCCESS)
    {
        Core::Utility::LogMsg("Platform", "Failed to allocate gpu memory!", Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
    }

    return record;
}

void GPU_PassthroughAllocator::Dealloc(const GPUMemAllocRecord& record)
{
    --m_numTotalAllocs;

    // Call free memory
}

// TODO: a real allocator
}
}
}
