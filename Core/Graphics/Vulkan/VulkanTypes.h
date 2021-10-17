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

#define VULKAN_MAX_RENDERTARGETS 1
#define VULKAN_MAX_RENDERTARGETS_WITH_DEPTH VULKAN_MAX_RENDERTARGETS + 1 // +1 for depth
// TODO: support multiple render targets more fully

#define VULKAN_MAX_SWAP_CHAIN_IMAGES 3
#define VULKAN_MAX_FRAMES_IN_FLIGHT 2

namespace Tk
{
namespace Platform
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
    VulkanMemResource resourceChain[VULKAN_MAX_SWAP_CHAIN_IMAGES];
    ResourceDesc resDesc;
} VulkanMemResourceChain;

typedef struct
{
    VulkanFramebufferResource resourceChain[VULKAN_MAX_SWAP_CHAIN_IMAGES];
} VulkanFramebufferResourceChain;

typedef struct
{
    VulkanDescriptorResource resourceChain[VULKAN_MAX_SWAP_CHAIN_IMAGES];
} VulkanDescriptorChain;

typedef struct
{
    VkDescriptorSetLayout layout;
    Platform::Graphics::DescriptorLayout bindings;
} VulkanDescriptorLayout;

typedef struct
{
    VkRenderPass renderPassVk;
    uint32 numColorRTs;
    bool hasDepth;
} VulkanRenderPass;

struct VkResources
{
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = NULL;
    PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT = NULL;
    PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT = NULL;
    PFN_vkCmdInsertDebugUtilsLabelEXT pfnCmdInsertDebugUtilsLabelEXT = NULL;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32 graphicsQueueIndex = TINKER_INVALID_HANDLE;
    uint32 presentationQueueIndex = TINKER_INVALID_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentationQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkExtent2D swapChainExtent = { 0, 0 };
    VkFormat swapChainFormat = VK_FORMAT_UNDEFINED;
    VkImage swapChainImages[VULKAN_MAX_SWAP_CHAIN_IMAGES];
    VkImageView swapChainImageViews[VULKAN_MAX_SWAP_CHAIN_IMAGES];
    FramebufferHandle swapChainFramebufferHandle = DefaultFramebufferHandle_Invalid;
    uint32 numSwapChainImages = 0;
    uint32 currentSwapChainImage = 0;
    uint32 currentFrame = 0;
    uint32 windowWidth = 0;
    uint32 windowHeight = 0;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkSampler linearSampler = VK_NULL_HANDLE;
    Memory::PoolAllocator<VulkanMemResourceChain> vulkanMemResourcePool;
    Memory::PoolAllocator<VulkanDescriptorChain> vulkanDescriptorResourcePool;
    Memory::PoolAllocator<VulkanFramebufferResourceChain> vulkanFramebufferResourcePool;
    VkFence fences[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
    VkFence* imageInFlightFences = nullptr;
    VkSemaphore swapChainImageAvailableSemaphores[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
    VkSemaphore renderCompleteSemaphores[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
    VkCommandBuffer* commandBuffers = nullptr;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_Immediate = VK_NULL_HANDLE;

    enum
    {
        eMaxShaders      = SHADER_ID_MAX,
        eMaxBlendStates  = BlendState::eMax,
        eMaxDepthStates  = DepthState::eMax,
        eMaxDescLayouts  = DESCLAYOUT_ID_MAX,
        eMaxRenderPasses = RENDERPASS_ID_MAX,
    };
    struct PSOPerms
    {
        VkPipeline       graphicsPipeline[eMaxShaders][eMaxBlendStates][eMaxDepthStates];
        VkPipelineLayout pipelineLayout[eMaxShaders];
    } psoPermutations;
    VulkanDescriptorLayout descLayouts[eMaxDescLayouts];

    VulkanRenderPass renderPasses[eMaxRenderPasses];
};

// Helpers
void AllocGPUMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceMemory* deviceMem,
    VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);
void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, uint32 sizeInBytes,
    VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
    VkBuffer& buffer, VkDeviceMemory& deviceMemory);
void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectMask, VkImage image, VkImageView* imageView, uint32 arrayEles);
void CreateFramebuffer(VkDevice device, VkImageView* colorRTs, uint32 numColorRTs, VkImageView depthRT,
    uint32 width, uint32 height, VkRenderPass renderPass, VkFramebuffer* frameBuffer);
void CreateRenderPass(VkDevice device, uint32 numColorAttachments, VkFormat colorFormat, VkImageLayout startLayout, VkImageLayout endLayout, VkFormat depthFormat, VkRenderPass* renderPass);
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
