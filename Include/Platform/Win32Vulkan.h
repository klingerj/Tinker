#pragma once

#include "../../Include/Core/CoreDefines.h"
#include "../Core/Math/VectorTypes.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>

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
                VkBuffer buffers[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
                VkDeviceMemory deviceMemory[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
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

            void* CreateVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes);
        }
    }
}
