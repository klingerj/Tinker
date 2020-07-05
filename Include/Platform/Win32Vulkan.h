#pragma once

#include "../../Include/Core/CoreDefines.h"
#include "../Core/Math/VectorTypes.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>

#define VULKAN_MAX_BUFFERS 8

namespace Tinker
{
    namespace Platform
    {
        namespace Graphics
        {
            typedef struct vulkan_context_res
            {
                VkInstance instance = VK_NULL_HANDLE;
                VkDebugUtilsMessengerEXT debugMessenger;
                VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
                uint32 graphicsQueueIndex = 0xffffffff;
                uint32 presentationQueueIndex = 0xffffffff;
                VkDevice device = VK_NULL_HANDLE;
                VkQueue graphicsQueue = VK_NULL_HANDLE;
                VkQueue presentationQueue = VK_NULL_HANDLE;
                VkSurfaceKHR surface = VK_NULL_HANDLE;
                VkSwapchainKHR swapChain = VK_NULL_HANDLE;
                VkExtent2D swapChainExtent = { 0, 0 };
                VkFormat swapChainFormat = VK_FORMAT_UNDEFINED;
                VkImage* swapChainImages = nullptr;
                VkImageView* swapChainImageViews = nullptr;
                uint32 numSwapChainImages = 0;

                // TODO: move this stuff elsewhere
                uint32 numAllocatedVertexBuffers = 0;
                uint32 numAllocatedStagingBuffers = 0;
                VkBuffer vertexBuffers[VULKAN_MAX_BUFFERS] = {};
                VkBuffer stagingBuffers[VULKAN_MAX_BUFFERS] = {};
                VkDeviceMemory vertexDeviceMemory[VULKAN_MAX_BUFFERS] = {};
                VkDeviceMemory stagingDeviceMemory[VULKAN_MAX_BUFFERS] = {};
                VkFence fence = VK_NULL_HANDLE;
                VkSemaphore swapChainImageAvailableSemaphore = VK_NULL_HANDLE;
                VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
                VkCommandBuffer* commandBuffers = nullptr;
                VkCommandPool commandPool = VK_NULL_HANDLE;
                VkFramebuffer* swapChainFramebuffers = nullptr;
                VkRenderPass renderPass = VK_NULL_HANDLE;
                VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
                VkPipeline pipeline = VK_NULL_HANDLE;
            } VulkanContextResources;

            typedef struct vertex_position
            {
                v4f position;
            } VulkanVertexPosition;

            int InitVulkan(VulkanContextResources* vulkanContextResources, HINSTANCE hInstance, HWND windowHandle, uint32 width, uint32 height);
            void DestroyVulkan(VulkanContextResources* vulkanContextResources);

            void InitGraphicsPipelineResources(VulkanContextResources* vulkanContextResources);
            void InitRenderPassResources(VulkanContextResources* vulkanContextResources);

            void SubmitFrame(VulkanContextResources* vulkanContextResources);
            void WaitForIdleDevice(VulkanContextResources* vulkanContextResources);

            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& deviceMemory);
            uint32 CreateVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes);
            void* CreateStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes);
        }
    }
}
