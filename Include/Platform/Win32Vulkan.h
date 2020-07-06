#pragma once

#include "../Core/CoreDefines.h"
#include "../Core/Math/VectorTypes.h"
#include "../Core/Allocators.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>

#define VULKAN_MAX_BUFFERS 128

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
                uint32 currentSwapChainImage = 0xffffffff;

                // TODO: move this stuff elsewhere
                Memory::PoolAllocator<VkBuffer> vulkanBufferPool;
                Memory::PoolAllocator<VkDeviceMemory> vulkanDeviceMemoryPool;
                Memory::PoolAllocator<void*> vulkanMappedMemPtrPool;
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

            typedef struct vulkan_staging_buffer_data
            {
                uint32 handle;
                void* mappedMemory;
            } VulkanStagingBufferData;

            int InitVulkan(VulkanContextResources* vulkanContextResources,
                HINSTANCE hInstance, HWND windowHandle,
                uint32 width, uint32 height);
            void DestroyVulkan(VulkanContextResources* vulkanContextResources);

            void InitGraphicsPipelineResources(VulkanContextResources* vulkanContextResources);
            void InitRenderPassResources(VulkanContextResources* vulkanContextResources);

            void SubmitFrame(VulkanContextResources* vulkanContextResources);
            void WaitForIdleDevice(VulkanContextResources* vulkanContextResources);

            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources);

            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                VkBuffer& buffer, VkDeviceMemory& deviceMemory);
            uint32 CreateVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes);
            VulkanStagingBufferData CreateStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes);
            void DestroyVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 handle);
            void DestroyStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 handle);

            // Graphics command recording
            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                uint32 vertexBufferHandle, uint32 indexBufferHandle,
                uint32 numIndices, uint32 numVertices);
            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                uint32 stagingBufferHandle, uint32 vertexBufferHandle, uint32 indexBufferHandle);

            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources);
            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources);
        }
    }
}
