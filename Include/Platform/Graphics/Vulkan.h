#pragma once

#include "../../Core/CoreDefines.h"
#include "../../Core/Math/VectorTypes.h"
#include "../../Core/Allocators.h"

#ifdef _WIN32
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#else
// TODO: include other platform headers
#endif
#include <vulkan/vulkan.h>

#define VULKAN_RESOURCE_POOL_MAX 256

#define VULKAN_NUM_SUPPORTED_DESCRIPTOR_TYPES 2
#define VULKAN_DESCRIPTOR_POOL_MAX_UNIFORM_BUFFERS 16
#define VULKAN_DESCRIPTOR_POOL_MAX_SAMPLED_IMAGES 16

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
                    VkImage image;
                };
            } VulkanMemResource;

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

            typedef struct vulkan_context_res
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

                VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
                VkSampler linearSampler = VK_NULL_HANDLE;
                Memory::PoolAllocator<VulkanDescriptorResource> vulkanDescriptorResourcePool;
                Memory::PoolAllocator<VulkanPipelineResource> vulkanPipelineResourcePool;
                Memory::PoolAllocator<VulkanMemResource> vulkanMemResourcePool;
                Memory::PoolAllocator<VkImageView> vulkanImageViewPool;
                // TODO: move this stuff elsewhere
                Memory::PoolAllocator<VkFramebuffer> vulkanFramebufferPool;
                Memory::PoolAllocator<VkRenderPass> vulkanRenderPassPool;
                VkFence fence = VK_NULL_HANDLE;
                VkSemaphore swapChainImageAvailableSemaphore = VK_NULL_HANDLE;
                VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
                VkCommandBuffer* commandBuffers = nullptr;
                VkCommandPool commandPool = VK_NULL_HANDLE;
            } VulkanContextResources;

            typedef struct vulkan_vertex_position
            {
                Core::Math::v4f position;
            } VulkanVertexPosition;

            typedef struct vertex_normal
            {
                Core::Math::v3f normal;
            } VulkanVertexNormal;

            typedef union vulkan_buffer_data
            {
                // Host-visible buffer with mapped memory
                struct
                {
                    uint32 hostBufferHandle;
                    void* mappedMemory;
                };

                // Buffer with no mapped memory
                uint32 gpuBufferHandle;
            } VulkanBufferData;

            typedef struct platform_window_handles
            {
                #ifdef _WIN32
                HINSTANCE instance;
                HWND windowHandle;
                #else
                // TODO: other platform window handles/pointers
                #endif
            } PlatformWindowHandles;

            int InitVulkan(VulkanContextResources* vulkanContextResources, const PlatformWindowHandles* platformWindowHandles, uint32 width, uint32 height);
            void DestroyVulkan(VulkanContextResources* vulkanContextResources);

            void VulkanCreateSwapChain(VulkanContextResources* vulkanContextResources);
            void VulkanDestroySwapChain(VulkanContextResources* vulkanContextResources);

            void AllocGPUMemory(VulkanContextResources* vulkanContextResources, VkDeviceMemory* deviceMem,
                VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);

            void VulkanSubmitFrame(VulkanContextResources* vulkanContextResources);

            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources);

            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                VkBuffer& buffer, VkDeviceMemory& deviceMemory);
            VulkanBufferData VulkanCreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes, uint32 bufferUsage);
            void VulkanDestroyBuffer(VulkanContextResources* vulkanContextResources, uint32 handle, uint32 bufferUsage);

            uint32 VulkanCreateFramebuffer(VulkanContextResources* vulkanContextResources,
                uint32* imageViewResourceHandles, uint32 numImageViewResourceHandles,
                uint32 width, uint32 height, uint32 renderPassHandle); // TODO: fb parameters e.g. format, attachments
            void VulkanDestroyFramebuffer(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateImageViewResource(VulkanContextResources* vulkanContextResources, uint32 imageResourceHandle); // TODO: parameters
            void VulkanDestroyImageViewResource(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateImageResource(VulkanContextResources* vulkanContextResources, uint32 width, uint32 height); // TODO: parameters
            void VulkanDestroyImageResource(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources, void* vertexShaderCode,
                uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 blendState,
                uint32 depthState, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassHandle,
                uint32 descriptorHandle);
            void VulkanDestroyGraphicsPipeline(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateRenderPass(VulkanContextResources* vulkanContextResources, uint32 startLayout, uint32 endLayout); // TODO: more parameters
            void VulkanDestroyRenderPass(VulkanContextResources* vulkanContextResources, uint32 handle);

            uint32 VulkanCreateDescriptor(VulkanContextResources* vulkanContextResources, DescriptorLayout* descLayout);
            void VulkanDestroyDescriptor(VulkanContextResources* vulkanContextResources, uint32 handle);
            void VulkanDestroyAllDescriptors(VulkanContextResources* vulkanContextResources);
            void VulkanWriteDescriptor(VulkanContextResources*  vulkanContextResources, DescriptorLayout* descLayout, uint32 descSetHandle, DescriptorSetDataHandles* descSetHandles);
            void InitDescriptorPool(VulkanContextResources* vulkanContextResources);

            void CreateSamplers(VulkanContextResources* vulkanContextResources);

            // Graphics command recording
            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                uint32 positionBufferHandle, uint32 normalBufferHandle,
                uint32 indexBufferHandle, uint32 numIndices,
                const char* debugLabel);
            void VulkanRecordCommandBindShader(VulkanContextResources* vulkanContextResources,
                uint32 shaderHandle, const DescriptorSetDataHandles* descSetHandles);
            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
                uint32 sizeInBytes, uint32 srcBufferHandle, uint32 dstBufferHandle,
                const char* debugLabel);
            void VulkanRecordCommandImageCopy(VulkanContextResources* vulkanContextResources,
                uint32 srcImgHandle, uint32 dstImgHandle, uint32 width, uint32 height);
            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
                uint32 renderPassHandle, uint32 framebufferHandle, uint32 renderWidth, uint32 renderHeight,
                const char* debugLabel);
            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources);
        }
    }
}
