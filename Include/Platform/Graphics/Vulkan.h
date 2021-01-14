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
                ResourceDesc resDesc;
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
                VkImageView* swapChainImageViews = nullptr;
                VkFramebuffer* swapChainFramebuffers = nullptr;
                uint32 numSwapChainImages = 0;
                VkRenderPass swapChainRenderPass = VK_NULL_HANDLE;
                uint32 currentSwapChainImage = TINKER_INVALID_HANDLE;
                uint32 windowWidth = 0;
                uint32 windowHeight = 0;

                VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
                VkSampler linearSampler = VK_NULL_HANDLE;
                Memory::PoolAllocator<VulkanDescriptorResource> vulkanDescriptorResourcePool;
                Memory::PoolAllocator<VulkanPipelineResource> vulkanPipelineResourcePool;
                Memory::PoolAllocator<VulkanMemResource> vulkanMemResourcePool;
                // TODO: move this stuff elsewhere
                Memory::PoolAllocator<VkFramebuffer> vulkanFramebufferPool;
                Memory::PoolAllocator<VkRenderPass> vulkanRenderPassPool;
                VkFence fence = VK_NULL_HANDLE;
                VkSemaphore swapChainImageAvailableSemaphore = VK_NULL_HANDLE;
                VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
                VkCommandBuffer* commandBuffers = nullptr;
                VkCommandPool commandPool = VK_NULL_HANDLE;
                VkCommandBuffer commandBuffer_Immediate = VK_NULL_HANDLE;
            } VulkanContextResources;

            typedef struct vulkan_vertex_position
            {
                Core::Math::v4f position;
            } VulkanVertexPosition;

            typedef struct vulkan_vertex_uv
            {
                Core::Math::v2f uv;
            } VulkanVertexUV;

            typedef struct vulkan_vertex_normal
            {
                Core::Math::v3f normal;
            } VulkanVertexNormal;

            typedef struct platform_window_handles
            {
                #ifdef _WIN32
                HINSTANCE instance;
                HWND windowHandle;
                #else
                // TODO: other platform window handles/pointers
                #endif
            } PlatformWindowHandles;

            // Init/destroy - called one time
            int InitVulkan(VulkanContextResources* vulkanContextResources, const PlatformWindowHandles* platformWindowHandles, uint32 width, uint32 height);
            void DestroyVulkan(VulkanContextResources* vulkanContextResources);

            void VulkanCreateSwapChain(VulkanContextResources* vulkanContextResources);
            void VulkanDestroySwapChain(VulkanContextResources* vulkanContextResources);

            // Helpers - TODO, could remove these declarations
            void AllocGPUMemory(VulkanContextResources* vulkanContextResources, VkDeviceMemory* deviceMem,
                VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);
            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                VkBuffer& buffer, VkDeviceMemory& deviceMemory);
            void CreateImageView(VulkanContextResources* vulkanContextResources, VkImage image, VkImageView* imageView);
            void CreateFramebuffer(VulkanContextResources* vulkanContextResources, VkImageView* imageViews, uint32 numImageViews,
                uint32 width, uint32 height, VkRenderPass renderPass, VkFramebuffer* frameBuffer);
            void CreateRenderPass(VulkanContextResources* vulkanContextResources, VkImageLayout startLayout, VkImageLayout endLayout, VkRenderPass* renderPass);

            // Frame command recording
            void VulkanSubmitFrame(VulkanContextResources* vulkanContextResources);
            
            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void BeginVulkanCommandRecordingImmediate(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecordingImmediate(VulkanContextResources* vulkanContextResources);

            // Main Graphics API - GPU resource create/destroy functions
            ResourceHandle VulkanCreateResource(VulkanContextResources* vulkanContextResources, const ResourceDesc& resDesc);
            void VulkanDestroyResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle);

            ResourceHandle VulkanCreateFramebuffer(VulkanContextResources* vulkanContextResources,
                ResourceHandle* imageViewResourceHandles, uint32 numImageViewResourceHandles,
                uint32 width, uint32 height, ResourceHandle renderPassHandle); // TODO: fb parameters e.g. format, attachments
            void VulkanDestroyFramebuffer(VulkanContextResources* vulkanContextResources, ResourceHandle handle);

            ShaderHandle VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources, void* vertexShaderCode,
                uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 blendState,
                uint32 depthState, uint32 viewportWidth, uint32 viewportHeight, ResourceHandle renderPassHandle,
                DescriptorHandle descriptorHandle);
            void VulkanDestroyGraphicsPipeline(VulkanContextResources* vulkanContextResources, ShaderHandle handle);

            ResourceHandle VulkanCreateRenderPass(VulkanContextResources* vulkanContextResources, uint32 startLayout, uint32 endLayout); // TODO: more parameters
            void VulkanDestroyRenderPass(VulkanContextResources* vulkanContextResources, ResourceHandle handle);

            DescriptorHandle VulkanCreateDescriptor(VulkanContextResources* vulkanContextResources, DescriptorLayout* descLayout);
            void VulkanDestroyDescriptor(VulkanContextResources* vulkanContextResources, DescriptorHandle handle);
            void VulkanDestroyAllDescriptors(VulkanContextResources* vulkanContextResources);
            void VulkanWriteDescriptor(VulkanContextResources*  vulkanContextResources, DescriptorLayout* descLayout, DescriptorHandle descSetHandle, DescriptorSetDataHandles* descSetHandles);
            void InitDescriptorPool(VulkanContextResources* vulkanContextResources);

            void CreateSamplers(VulkanContextResources* vulkanContextResources);

            // Memory mapping - probably just for staging buffers
            void* VulkanMapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle);
            void VulkanUnmapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle);

            // Graphics command recording
            void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
                ResourceHandle positionBufferHandle, ResourceHandle uvBufferHandle,
                ResourceHandle normalBufferHandle, ResourceHandle indexBufferHandle, uint32 numIndices,
                const char* debugLabel, bool immediateSubmit);
            void VulkanRecordCommandBindShader(VulkanContextResources* vulkanContextResources,
                ShaderHandle shaderHandle, const DescriptorSetDescHandles* descSetHandles,
                bool immediateSubmit);
            void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
                uint32 sizeInBytes, ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle,
                const char* debugLabel, bool immediateSubmit);
            void VulkanRecordCommandImageCopy(VulkanContextResources* vulkanContextResources,
                ResourceHandle srcImgHandle, ResourceHandle dstImgHandle, uint32 width, uint32 height,
                bool immediateSubmit);
            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
                ResourceHandle renderPassHandle, ResourceHandle framebufferHandle, uint32 renderWidth, uint32 renderHeight,
                const char* debugLabel, bool immediateSubmit);
            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources, bool immediateSubmit);
        }
    }
}
