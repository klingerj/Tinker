#pragma once

#include "Core/CoreDefines.h"
#include "Core/Allocators.h"

#include <vulkan/vulkan.h>

#define VULKAN_RESOURCE_POOL_MAX 256

#define VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES 2
#define VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS 16
#define VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES 16

#define VULKAN_MAX_RENDERTARGETS 1
#define VULKAN_MAX_RENDERTARGETS_WITH_DEPTH VULKAN_MAX_RENDERTARGETS + 1 // +1 for depth
// TODO: support multiple render targets more fully

#define VULKAN_MAX_SWAP_CHAIN_IMAGES 3
#define VULKAN_MAX_FRAMES_IN_FLIGHT 2


namespace Tinker
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
        VkRenderPass renderPass;
        VkClearValue clearValues[VULKAN_MAX_RENDERTARGETS_WITH_DEPTH]; // + 1 for depth
        uint32 numClearValues;
    } VulkanFramebufferResource;

    typedef struct vulkan_pipeline_resource
    {
        VkPipeline graphicsPipeline;
        VkPipelineLayout pipelineLayout;
    } VulkanPipelineResource;

    typedef struct vulkan_descriptor_resource
    {
        VkDescriptorSetLayout descriptorLayout;
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
        VkImage* swapChainImages = nullptr;
        VkImageView* swapChainImageViews = nullptr;
        FramebufferHandle swapChainFramebufferHandle = DefaultFramebufferHandle_Invalid;
        uint32 numSwapChainImages = 0;
        uint32 currentSwapChainImage = 0;
        uint32 currentFrame = 0;
        uint32 windowWidth = 0;
        uint32 windowHeight = 0;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkSampler linearSampler = VK_NULL_HANDLE;
        Memory::PoolAllocator<VulkanMemResourceChain> vulkanMemResourcePool;
        Memory::PoolAllocator<VulkanPipelineResource> vulkanPipelineResourcePool;
        Memory::PoolAllocator<VulkanDescriptorChain> vulkanDescriptorResourcePool;
        Memory::PoolAllocator<VulkanFramebufferResourceChain> vulkanFramebufferResourcePool;
        VkFence fences[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
        VkFence* imageInFlightFences = nullptr;
        VkSemaphore swapChainImageAvailableSemaphores[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
        VkSemaphore renderCompleteSemaphores[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
        VkCommandBuffer* commandBuffers = nullptr;
        VkCommandBuffer* threaded_secondaryCommandBuffers = nullptr;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer_Immediate = VK_NULL_HANDLE;
    };

    // Helpers
    void AllocGPUMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceMemory* deviceMem,
        VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);
    void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, uint32 sizeInBytes,
        VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
        VkBuffer& buffer, VkDeviceMemory& deviceMemory);
    void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectMask, VkImage image, VkImageView* imageView);
    void CreateFramebuffer(VkDevice device, VkImageView* colorRTs, uint32 numColorRTs, VkImageView depthRT,
        uint32 width, uint32 height, VkRenderPass renderPass, VkFramebuffer* frameBuffer);
    void CreateRenderPass(VkDevice device, uint32 numColorAttachments, VkFormat colorFormat, VkImageLayout startLayout, VkImageLayout endLayout, VkFormat depthFormat, VkRenderPass* renderPass);
}
}
}
