#include "VulkanTypes.h"
#include "Utility/Logging.h"
#include <cstring>

namespace Tk
{
  namespace Graphics
  {
    VulkanContextResources g_vulkanContextResources = {};

    void VulkanMemoryAllocator::Init(uint32 TotalAllocSize, uint32 MemoryTypeIndex,
                                     VkMemoryPropertyFlagBits MemPropertyFlags,
                                     VkDeviceSize AllocGranularity)
    {
      m_GPUMemory = VK_NULL_HANDLE;
      m_AllocSizeLimit = TotalAllocSize;
      m_LastAllocOffset = 0;
      m_NumAllocs = 0;
      m_MemFlags = MemPropertyFlags;
      m_AllocGranularity = AllocGranularity;

      VkMemoryAllocateInfo memAllocInfo = {};
      memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAllocInfo.allocationSize = m_AllocSizeLimit;
      memAllocInfo.memoryTypeIndex = MemoryTypeIndex;
      VkResult result = vkAllocateMemory(g_vulkanContextResources.device, &memAllocInfo,
                                         nullptr, &m_GPUMemory);
      if (result != VK_SUCCESS)
      {
        Core::Utility::LogMsg("Platform",
                              "GPU mem allocator failed to allocate gpu memory!",
                              Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
      }
    }

    void VulkanMemoryAllocator::Destroy()
    {
      vkFreeMemory(g_vulkanContextResources.device, m_GPUMemory, nullptr);
      m_GPUMemory = VK_NULL_HANDLE;
    }

    VulkanMemAlloc VulkanMemoryAllocator::Alloc(VkMemoryRequirements allocReqs)
    {
      uint64 allocSize = RoundValueToPow2(allocReqs.size, m_AllocGranularity);

      uint64 alignment = Max(m_AllocGranularity, allocReqs.alignment);
      TINKER_ASSERT(ISPOW2(alignment));
      uint64 nextAllocOffset =
        RoundValueToPow2(m_LastAllocOffset, alignment); // ensure alloc offset is aligned
      uint64 nextAllocEnd = nextAllocOffset + allocSize;

      if (nextAllocEnd > m_AllocSizeLimit)
      {
        TINKER_ASSERT(0);
        Core::Utility::LogMsg("Platform", "Vulkan gpu mem allocator ran out of memory!",
                              Core::Utility::LogSeverity::eInfo);
      }

      m_LastAllocOffset = nextAllocEnd;
      ++m_NumAllocs;

      VulkanMemAlloc alloc;
      alloc.allocMem = m_GPUMemory;
      alloc.allocOffset = nextAllocOffset;
      alloc.allocSize = allocSize;
      alloc.mappedMemPtr = m_MappedMemPtr; // may be nullptr
      return alloc;
    }

    uint32 ChooseMemoryTypeBits(uint32 requiredMemoryTypeBits,
                                VkMemoryPropertyFlags memPropertyFlags)
    {
      VkPhysicalDeviceMemoryProperties memProperties;
      vkGetPhysicalDeviceMemoryProperties(g_vulkanContextResources.physicalDevice,
                                          &memProperties);

      uint32 memTypeIndex = 0xff'ff'ff'ff;
      // TODO: make this smarter
      // For now, pick the first memory heap that remotely matches
      for (uint32 uiMemType = 0; uiMemType < memProperties.memoryTypeCount; ++uiMemType)
      {
        if (((1 << uiMemType) & requiredMemoryTypeBits)
            && (memProperties.memoryTypes[uiMemType].propertyFlags & memPropertyFlags)
                 == memPropertyFlags)
        {
          memTypeIndex = uiMemType;
          break;
        }
      }
      return memTypeIndex;
    }

    VkResult CreateBuffer(VkBufferCreateFlags flags, VkDeviceSize size,
                          VkBufferUsageFlags usage, VkSharingMode sharingMode,
                          VkBuffer* outBuffer)
    {
      VkBufferCreateInfo bufferCreateInfo = {};
      bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCreateInfo.flags = flags;
      bufferCreateInfo.size = size;
      bufferCreateInfo.usage = usage;
      bufferCreateInfo.sharingMode = sharingMode;
      return vkCreateBuffer(g_vulkanContextResources.device, &bufferCreateInfo, nullptr,
                            outBuffer);
    }

    VkResult CreateImage(VkImageCreateFlags flags, VkImageType imageType, VkFormat format,
                         VkExtent3D extent, uint32 mipLevels, uint32 arrayLayers,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkSharingMode sharingMode, VkImage* outImage)
    {
      VkImageCreateInfo imageCreateInfo = {};
      imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      imageCreateInfo.imageType = imageType;
      imageCreateInfo.extent = extent;
      imageCreateInfo.mipLevels = mipLevels;
      imageCreateInfo.arrayLayers = arrayLayers;
      imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCreateInfo.format = format;
      imageCreateInfo.tiling = tiling;
      imageCreateInfo.usage = usage;
      imageCreateInfo.flags = flags;
      return vkCreateImage(g_vulkanContextResources.device, &imageCreateInfo, nullptr,
                           outImage);
    }

    static void DbgSetObjectNameBase(uint64 handle, VkObjectType type, const char* name)
    {
#ifdef ENABLE_VULKAN_DEBUG_LABELS
      VkDebugUtilsObjectNameInfoEXT info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, NULL, type, handle, name
      };

      g_vulkanContextResources.pfnSetDebugUtilsObjectNameEXT(
        g_vulkanContextResources.device, &info);
#endif
    }

    void DbgSetImageObjectName(uint64 handle, const char* name)
    {
      DbgSetObjectNameBase(handle, VK_OBJECT_TYPE_IMAGE, name);
    }

    void DbgSetBufferObjectName(uint64 handle, const char* name)
    {
      DbgSetObjectNameBase(handle, VK_OBJECT_TYPE_BUFFER, name);
    }

    void DbgStartMarker(VkCommandBuffer commandBuffer, const char* debugLabel)
    {
#if defined(ENABLE_VULKAN_DEBUG_LABELS)
      VkDebugUtilsLabelEXT label = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        NULL,
        debugLabel,
        { 0.0f, 0.0f, 0.0f, 0.0f },
      };
      g_vulkanContextResources.pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
#endif
    }

    void DbgEndMarker(VkCommandBuffer commandBuffer)
    {
#if defined(ENABLE_VULKAN_DEBUG_LABELS)
      g_vulkanContextResources.pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
    }

    CommandBuffer CreateCommandBuffer()
    {
      VkCommandBuffer cmdBuffers[MAX_FRAMES_IN_FLIGHT] = {};

      VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
      commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      commandBufferAllocInfo.commandPool = g_vulkanContextResources.commandPool;
      commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

      VkResult result = vkAllocateCommandBuffers(g_vulkanContextResources.device,
                                                 &commandBufferAllocInfo, &cmdBuffers[0]);
      if (result != VK_SUCCESS)
      {
        Core::Utility::LogMsg("Platform",
                              "Failed to allocate Vulkan primary frame command buffers!",
                              Core::Utility::LogSeverity::eCritical);
        TINKER_ASSERT(0);
      }

      CommandBuffer commandBuffer = {};
      for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
      {
        commandBuffer.apiObjectHandles[i] = (uint64)cmdBuffers[i];
      }
      return commandBuffer;
    }

    // NOTE: Must correspond the enums in PlatformGameAPI.h
    static VkPipelineColorBlendAttachmentState VulkanBlendStates[BlendState::eMax] = {};
    static DepthCullState VulkanDepthStates[DepthState::eMax] = {};
    static VkImageLayout VulkanImageLayouts[ImageLayout::eMax] = {};
    static VkFormat VulkanImageFormats[ImageFormat::eMax] = {};
    static VkDescriptorType VulkanDescriptorTypes[DescriptorType::eMax] = {};
    static VkBufferUsageFlags VulkanBufferUsageFlags[BufferUsage::eMax] = {};
    static VkImageAspectFlags VulkanImageAspectFlags[ImageFormat::eMax] = {};
    static VkMemoryPropertyFlagBits VulkanMemPropertyFlags[BufferUsage::eMax] = {};
    static VkPipelineBindPoint VulkanBindPoints[BindPoint::eMax] = {};

    void InitVulkanDataTypesPerEnum()
    {
      VkPipelineColorBlendAttachmentState blendStateProperties = {};
      blendStateProperties.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                                            | VK_COLOR_COMPONENT_G_BIT
                                            | VK_COLOR_COMPONENT_B_BIT
                                            | VK_COLOR_COMPONENT_A_BIT;
      blendStateProperties.blendEnable = VK_TRUE;
      blendStateProperties.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      blendStateProperties.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      blendStateProperties.colorBlendOp = VK_BLEND_OP_ADD;
      blendStateProperties.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      blendStateProperties.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      blendStateProperties.alphaBlendOp = VK_BLEND_OP_ADD;
      VulkanBlendStates[BlendState::eAlphaBlend] = blendStateProperties;
      blendStateProperties.blendEnable = VK_FALSE;
      VulkanBlendStates[BlendState::eReplace] = blendStateProperties;
      VulkanBlendStates[BlendState::eNoColorAttachment] = {};

      VkCompareOp depthCompareOps[DepthCompareOp::eMax] = {};
      depthCompareOps[DepthCompareOp::eLeOrEqual] = VK_COMPARE_OP_LESS_OR_EQUAL;
      depthCompareOps[DepthCompareOp::eGeOrEqual] = VK_COMPARE_OP_GREATER_OR_EQUAL;
      TINKER_ASSERT(DEPTH_OP < ARRAYCOUNT(depthCompareOps));

      VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
      depthStencilState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depthStencilState.depthCompareOp = depthCompareOps[DEPTH_OP];
      depthStencilState.depthBoundsTestEnable = VK_FALSE;
      depthStencilState.minDepthBounds = DEPTH_MIN;
      depthStencilState.maxDepthBounds = DEPTH_MAX;
      depthStencilState.stencilTestEnable = VK_FALSE;
      depthStencilState.front = {};
      depthStencilState.back = {};
      depthStencilState.depthTestEnable = VK_FALSE;
      depthStencilState.depthWriteEnable = VK_FALSE;
      VulkanDepthStates[DepthState::eOff_NoCull].cullMode = VK_CULL_MODE_NONE;
      VulkanDepthStates[DepthState::eOff_CCW].depthState = depthStencilState;
      VulkanDepthStates[DepthState::eOff_CCW].cullMode = VK_CULL_MODE_BACK_BIT;

      depthStencilState.depthTestEnable = VK_TRUE;
      depthStencilState.depthWriteEnable = VK_TRUE;
      VulkanDepthStates[DepthState::eTestOnWriteOn_CCW].depthState = depthStencilState;
      VulkanDepthStates[DepthState::eTestOnWriteOn_CCW].cullMode = VK_CULL_MODE_BACK_BIT;

      depthStencilState.depthWriteEnable = VK_FALSE;
      VulkanDepthStates[DepthState::eTestOnWriteOff_CCW].depthState = depthStencilState;
      VulkanDepthStates[DepthState::eTestOnWriteOff_CCW].cullMode = VK_CULL_MODE_BACK_BIT;

      VulkanImageLayouts[ImageLayout::eUndefined] = VK_IMAGE_LAYOUT_UNDEFINED;
      VulkanImageLayouts[ImageLayout::eShaderRead] =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      VulkanImageLayouts[ImageLayout::eTransferDst] =
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      VulkanImageLayouts[ImageLayout::eDepthOptimal] =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      VulkanImageLayouts[ImageLayout::eRenderOptimal] =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      VulkanImageLayouts[ImageLayout::eGeneral] = VK_IMAGE_LAYOUT_GENERAL;
      VulkanImageLayouts[ImageLayout::ePresent] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VulkanImageFormats[ImageFormat::Invalid] = VK_FORMAT_UNDEFINED;
      VulkanImageFormats[ImageFormat::BGRA8_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;
      VulkanImageFormats[ImageFormat::RGBA8_SRGB] = VK_FORMAT_R8G8B8A8_SRGB;
      VulkanImageFormats[ImageFormat::RGBA8] = VK_FORMAT_R8G8B8A8_UNORM;
      VulkanImageFormats[ImageFormat::RGBA16_Float] = VK_FORMAT_R16G16B16A16_SFLOAT;
      VulkanImageFormats[ImageFormat::Depth_32F] = VK_FORMAT_D32_SFLOAT;
      VulkanImageFormats[ImageFormat::TheSwapChainFormat] = VK_FORMAT_UNDEFINED;

      VulkanDescriptorTypes[DescriptorType::eBuffer] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      VulkanDescriptorTypes[DescriptorType::eDynamicBuffer] =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      VulkanDescriptorTypes[DescriptorType::eSampledImage] =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      VulkanDescriptorTypes[DescriptorType::eSSBO] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      VulkanDescriptorTypes[DescriptorType::eStorageImage] =
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      VulkanDescriptorTypes[DescriptorType::eArrayOfTextures] =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      VulkanDescriptorTypes[DescriptorType::eArrayOfTexturesRW] =
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

      VulkanBufferUsageFlags[BufferUsage::eVertex] =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // vertex buffers are actually SSBOs for now
      VulkanBufferUsageFlags[BufferUsage::eIndex] =
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      VulkanBufferUsageFlags[BufferUsage::eTransient] =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      VulkanBufferUsageFlags[BufferUsage::eTransientIndex] =
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      VulkanBufferUsageFlags[BufferUsage::eStaging] = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      VulkanBufferUsageFlags[BufferUsage::eUniform] = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

      VulkanImageAspectFlags[ImageFormat::Invalid] = VK_IMAGE_ASPECT_NONE;
      VulkanImageAspectFlags[ImageFormat::RGBA16_Float] = VK_IMAGE_ASPECT_COLOR_BIT;
      VulkanImageAspectFlags[ImageFormat::BGRA8_SRGB] = VK_IMAGE_ASPECT_COLOR_BIT;
      VulkanImageAspectFlags[ImageFormat::RGBA8_SRGB] = VK_IMAGE_ASPECT_COLOR_BIT;
      VulkanImageAspectFlags[ImageFormat::RGBA8] = VK_IMAGE_ASPECT_COLOR_BIT;
      VulkanImageAspectFlags[ImageFormat::Depth_32F] = VK_IMAGE_ASPECT_DEPTH_BIT;
      VulkanImageAspectFlags[ImageFormat::TheSwapChainFormat] = VK_IMAGE_ASPECT_COLOR_BIT;

      VulkanMemPropertyFlags[BufferUsage::eVertex] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      VulkanMemPropertyFlags[BufferUsage::eIndex] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      VulkanMemPropertyFlags[BufferUsage::eTransient] =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      VulkanMemPropertyFlags[BufferUsage::eTransientIndex] =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      VulkanMemPropertyFlags[BufferUsage::eStaging] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      VulkanMemPropertyFlags[BufferUsage::eUniform] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

      VulkanBindPoints[BindPoint::eGraphics] = VK_PIPELINE_BIND_POINT_GRAPHICS;
      VulkanBindPoints[BindPoint::eCompute] = VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    const VkPipelineColorBlendAttachmentState& GetVkBlendState(uint32 gameBlendState)
    {
      TINKER_ASSERT(gameBlendState < BlendState::eMax);
      return VulkanBlendStates[gameBlendState];
    }

    const DepthCullState& GetVkDepthCullState(uint32 gameDepthCullState)
    {
      TINKER_ASSERT(gameDepthCullState < DepthState::eMax);
      return VulkanDepthStates[gameDepthCullState];
    }

    VkImageLayout GetVkImageLayout(uint32 gameImageLayout)
    {
      TINKER_ASSERT(gameImageLayout < ImageLayout::eMax);
      return VulkanImageLayouts[gameImageLayout];
    }

    VkFormat GetVkImageFormat(uint32 gameImageFormat)
    {
      TINKER_ASSERT(gameImageFormat < ImageFormat::eMax);
      if (gameImageFormat == ImageFormat::TheSwapChainFormat)
      {
        return g_vulkanContextResources.swapChainFormat;
      }
      else
      {
        return VulkanImageFormats[gameImageFormat];
      }
    }

    VkDescriptorType GetVkDescriptorType(uint32 gameDescriptorType)
    {
      TINKER_ASSERT(gameDescriptorType < DescriptorType::eMax);
      return VulkanDescriptorTypes[gameDescriptorType];
    }

    VkBufferUsageFlags GetVkBufferUsageFlags(uint32 bufferUsage)
    {
      TINKER_ASSERT(bufferUsage < BufferUsage::eMax);
      return VulkanBufferUsageFlags[bufferUsage];
    }

    VkMemoryPropertyFlags GetVkMemoryPropertyFlags(uint32 bufferUsage)
    {
      TINKER_ASSERT(bufferUsage < BufferUsage::eMax);
      return VulkanMemPropertyFlags[bufferUsage];
    }

    VkPipelineBindPoint GetVkBindPoint(uint32 bindPoint)
    {
      TINKER_ASSERT(bindPoint < BindPoint::eMax);
      return VulkanBindPoints[bindPoint];
    }

    VkImageAspectFlags GetVkImageAspectFlags(uint32 format)
    {
      TINKER_ASSERT(format < ImageFormat::eMax);
      return VulkanImageAspectFlags[format];
    }
  } //namespace Graphics
} //namespace Tk
