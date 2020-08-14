#pragma once

#include "../Core/CoreDefines.h"
#include "../Core/Math/VectorTypes.h"
#include "../Core/Allocators.h"
#include "GraphicsTypes.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>

#define VULKAN_RESOURCE_POOL_MAX 256

namespace Tinker
{
    namespace Platform
    {
        namespace Graphics
        {
            typedef struct vulkan_resource
            {
                VkDeviceMemory deviceMemory;
                union
                {
                    VkBuffer buffer;
                    VkImage image;
                };
            } VulkanMemResource;

            typedef struct vulkan_pipeline
            {
                VkPipeline graphicsPipeline;
                VkPipelineLayout pipelineLayout;
            } VulkanPipelineResource;

            typedef struct vulkan_context_res
            {
                bool isInitted = false;
                bool isSwapChainValid = false;
                VkInstance instance = VK_NULL_HANDLE;
                VkDebugUtilsMessengerEXT debugMessenger;
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
                uint32* swapChainImageViewHandles = nullptr;
                uint32* swapChainFramebufferHandles = nullptr;
                uint32 numSwapChainImages = 0;
                uint32 swapChainRenderPassHandle = TINKER_INVALID_HANDLE;
                uint32 currentSwapChainImage = TINKER_INVALID_HANDLE;
                uint32 windowWidth = 0;
                uint32 windowHeight = 0;

                Memory::PoolAllocator<VulkanPipelineResource> vulkanPipelineResourcePool;
                Memory::PoolAllocator<VulkanMemResource> vulkanMemResourcePool;
                Memory::PoolAllocator<VkImageView> vulkanImageViewPool;
                // TODO: move this stuff elsewhere
                Memory::PoolAllocator<VkFramebuffer> vulkanFramebufferPool;
                Memory::PoolAllocator<VkRenderPass> vulkanRenderPassPool;
                Memory::PoolAllocator<void*> vulkanMappedMemPtrPool;
                VkFence fence = VK_NULL_HANDLE;
                VkSemaphore swapChainImageAvailableSemaphore = VK_NULL_HANDLE;
                VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
                VkCommandBuffer* commandBuffers = nullptr;
                VkCommandPool commandPool = VK_NULL_HANDLE;
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

            void CreateSwapChain(VulkanContextResources* vulkanContextResources);
            void DestroySwapChain(VulkanContextResources* vulkanContextResources);

            void AllocGPUMemory(VulkanContextResources* vulkanContextResources, VkDeviceMemory* deviceMem,
                VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);

            void SubmitFrame(VulkanContextResources* vulkanContextResources);
            void WaitForIdleDevice(VulkanContextResources* vulkanContextResources);

            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources);

            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                VkBuffer& buffer, VkDeviceMemory& deviceMemory);
            uint32 CreateVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, BufferType bufferType);
            VulkanStagingBufferData CreateStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes);
            void DestroyVertexBuffer(VulkanContextResources* vulkanContextResources, uint32 handle);
            void DestroyStagingBuffer(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateFramebuffer(VulkanContextResources* vulkanContextResources,
                uint32* imageViewResourceHandles, uint32 numImageViewResourceHandles,
                uint32 width, uint32 height, uint32 renderPassHandle); // TODO: fb parameters e.g. format, attachments
            void DestroyFramebuffer(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 CreateImageViewResource(VulkanContextResources* vulkanContextResources, uint32 imageResourceHandle); // TODO: parameters
            void DestroyImageViewResource(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 CreateImageResource(VulkanContextResources* vulkanContextResources, uint32 width, uint32 height); // TODO: parameters
            void DestroyImageResource(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources, void* vertexShaderCode,
                    uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 blendState,
                    uint32 depthState, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassHandle);
            void VulkanDestroyGraphicsPipeline(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateRenderPass(VulkanContextResources* vulkanContextResources); // TODO: more parameters, e.g. format
            void VulkanDestroyRenderPass(VulkanContextResources* vulkanContextResources, uint32 handle);

            // Graphics command recording
            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                uint32 vertexBufferHandle, uint32 indexBufferHandle, uint32 numIndices, uint32 numVertices);
            void VulkanRecordCommandBindShader(VulkanContextResources* vulkanContextResources,
                uint32 shaderHandle);
            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
                uint32 sizeInBytes, uint32 srcBufferHandle, uint32 dstBufferHandle);
            void VulkanRecordCommandImageCopy(VulkanContextResources* vulkanContextResources,
                uint32 srcImgHandle, uint32 dstImgHandle, uint32 width, uint32 height);

            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
                uint32 renderPassHandle, uint32 framebufferHandle, uint32 renderWidth, uint32 renderHeight);
            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources);
        }
    }
}
