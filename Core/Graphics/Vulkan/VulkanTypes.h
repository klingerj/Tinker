#pragma once

#include "CoreDefines.h"
#include "Allocators.h"
#include "Graphics/Common/GraphicsCommon.h"

#include <vulkan/vulkan.h>

#define VULKAN_RESOURCE_POOL_MAX 512

#define VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES 3
#define VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS 64
#define VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES 64
#define VULKAN_DESCRIPTOR_POOL_MAX_STORAGE_BUFFERS 64

#define VULKAN_MAX_RENDERTARGETS MAX_MULTIPLE_RENDERTARGETS
#define VULKAN_MAX_RENDERTARGETS_WITH_DEPTH VULKAN_MAX_RENDERTARGETS + 1 // +1 for depth

#define VULKAN_MAX_FRAMES_IN_FLIGHT 2

namespace Tk
{
namespace Core
{
namespace Graphics
{

typedef struct vulkan_mem_resource
{
    VkDeviceMemory deviceMemory;

    union
    {
        VkBuffer buffer;

        // Group images with an image view
        struct
        {
            VkImage image;
            VkImageView imageView;
        };
    };
} VulkanMemResource;

typedef struct vulkan_framebuffer_resource
{
    VkFramebuffer framebuffer;
    VkClearValue clearValues[VULKAN_MAX_RENDERTARGETS_WITH_DEPTH]; // + 1 for depth
    uint32 numClearValues;
} VulkanFramebufferResource;

typedef struct vulkan_descriptor_resource
{
    VkDescriptorSet descriptorSet;
} VulkanDescriptorResource;

// Chains of resources for multiple swap chain images
typedef struct
{
    VulkanMemResource resourceChain[VULKAN_MAX_FRAMES_IN_FLIGHT];
    ResourceDesc resDesc;
} VulkanMemResourceChain;

typedef struct
{
    VulkanDescriptorResource resourceChain[VULKAN_MAX_FRAMES_IN_FLIGHT];
} VulkanDescriptorChain;

typedef struct
{
    VkDescriptorSetLayout layout;
    Core::Graphics::DescriptorLayout bindings;
} VulkanDescriptorLayout;

typedef struct
{
    VkFence Fence;
    VkSemaphore GPUWorkCompleteSema;
    VkSemaphore PresentCompleteSema;
} VulkanVirtualFrameSyncData;

struct VulkanContextResources
{
    bool isInitted = false;
    bool isSwapChainValid = false;
    
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = NULL;
    PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT = NULL;
    PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT = NULL;
    PFN_vkCmdInsertDebugUtilsLabelEXT pfnCmdInsertDebugUtilsLabelEXT = NULL;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32 graphicsQueueIndex = TINKER_INVALID_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentationQueue = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkExtent2D swapChainExtent = { 0, 0 };
    VkFormat swapChainFormat = VK_FORMAT_UNDEFINED;
    VkImage* swapChainImages = nullptr;
    VkImageView* swapChainImageViews = nullptr;
    VkFramebuffer* swapChainFramebuffers = nullptr;
    uint32 numSwapChainImages = 0;

    uint32 currentSwapChainImage = TINKER_INVALID_HANDLE;
    uint32 currentVirtualFrame = 0;
    uint32 windowWidth = 0;
    uint32 windowHeight = 0;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkSampler linearSampler = VK_NULL_HANDLE;
    Tk::Core::PoolAllocator<VulkanMemResourceChain> vulkanMemResourcePool;
    Tk::Core::PoolAllocator<VulkanDescriptorChain> vulkanDescriptorResourcePool;
    VulkanVirtualFrameSyncData virtualFrameSyncData[VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkCommandBuffer* commandBuffers = nullptr;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_Immediate = VK_NULL_HANDLE;

    enum
    {
        eMaxShaders      = SHADER_ID_MAX,
        eMaxBlendStates  = BlendState::eMax,
        eMaxDepthStates  = DepthState::eMax,
        eMaxDescLayouts  = DESCLAYOUT_ID_MAX,
    };
    struct PSOPerms
    {
        VkPipeline       graphicsPipeline[eMaxShaders][eMaxBlendStates][eMaxDepthStates];
        VkPipelineLayout pipelineLayout[eMaxShaders];
    } psoPermutations;
    VulkanDescriptorLayout descLayouts[eMaxDescLayouts];
};
extern VulkanContextResources g_vulkanContextResources;

// Helpers
void AllocGPUMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceMemory* deviceMem,
    VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);
void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, uint32 sizeInBytes,
    VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
    VkBuffer* buffer, VkDeviceMemory* deviceMemory);
void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectMask, VkImage image, VkImageView* imageView, uint32 arrayEles);
VkShaderModule CreateShaderModule(const char* shaderCode, uint32 numShaderCodeBytes, VkDevice device);

void InitVulkanDataTypesPerEnum();
const VkPipelineColorBlendAttachmentState& GetVkBlendState(uint32 gameBlendState);
const VkPipelineDepthStencilStateCreateInfo& GetVkDepthState(uint32 gameDepthState);
const VkImageLayout& GetVkImageLayout(uint32 gameImageLayout);
const VkFormat& GetVkImageFormat(uint32 gameImageFormat);
const VkDescriptorType& GetVkDescriptorType(uint32 gameDescriptorType);

}
}
}
