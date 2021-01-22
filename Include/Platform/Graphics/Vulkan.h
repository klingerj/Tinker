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
                // TODO: move this stuff elsewhere
                VkFence fences[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
                VkFence* imageInFlightFences = nullptr;
                VkSemaphore swapChainImageAvailableSemaphores[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
                VkSemaphore renderCompleteSemaphores[VULKAN_MAX_FRAMES_IN_FLIGHT] = {};
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

            // Helpers
            void AllocGPUMemory(VulkanContextResources* vulkanContextResources, VkDeviceMemory* deviceMem,
                VkMemoryRequirements memRequirements, VkMemoryPropertyFlags memPropertyFlags);
            void CreateBuffer(VulkanContextResources* vulkanContextResources, uint32 sizeInBytes,
                VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                VkBuffer& buffer, VkDeviceMemory& deviceMemory);
            void CreateImageView(VulkanContextResources* vulkanContextResources, VkImage image, VkImageView* imageView);
            void CreateFramebuffer(VulkanContextResources* vulkanContextResources, VkImageView* colorRTs, uint32 numColorRTs, VkImageView depthRT,
                uint32 width, uint32 height, VkRenderPass renderPass, VkFramebuffer* frameBuffer);
            void CreateRenderPass(VulkanContextResources* vulkanContextResources, uint32 numColorAttachments, VkFormat colorFormat, VkImageLayout startLayout, VkImageLayout endLayout, VkFormat depthFormat, VkRenderPass* renderPass);

            // Frame command recording
            void AcquireFrame(VulkanContextResources* vulkanContextResources);
            void VulkanSubmitFrame(VulkanContextResources* vulkanContextResources);
            
            void BeginVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecording(VulkanContextResources* vulkanContextResources);
            void BeginVulkanCommandRecordingImmediate(VulkanContextResources* vulkanContextResources);
            void EndVulkanCommandRecordingImmediate(VulkanContextResources* vulkanContextResources);

            // Main Graphics API - GPU resource create/destroy functions
            ResourceHandle VulkanCreateResource(VulkanContextResources* vulkanContextResources, const ResourceDesc& resDesc);
            void VulkanDestroyResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle);

            FramebufferHandle VulkanCreateFramebuffer(VulkanContextResources* vulkanContextResources,
                ResourceHandle* rtColorHandles, uint32 numRTColorHandles, ResourceHandle rtDepthHandle,
                uint32 colorEndLayout, uint32 width, uint32 height);
            void VulkanDestroyFramebuffer(VulkanContextResources* vulkanContextResources, FramebufferHandle handle);

            ShaderHandle VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources, void* vertexShaderCode,
                uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes,
                uint32 blendState, uint32 depthState, uint32 viewportWidth, uint32 viewportHeight,
                FramebufferHandle framebufferHandle, DescriptorHandle descriptorHandle);
            void VulkanDestroyGraphicsPipeline(VulkanContextResources* vulkanContextResources, ShaderHandle handle);

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
            void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
                FramebufferHandle framebufferHandle, uint32 renderWidth, uint32 renderHeight,
                const char* debugLabel, bool immediateSubmit);
            void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources, bool immediateSubmit);
            void VulkanRecordCommandTransitionLayout(VulkanContextResources*  vulkanContextResources, ResourceHandle imageHandle,
                uint32 startLayout, uint32 endLayout, const char* debugLabel, bool immediateSubmit);
            void VulkanRecordCommandClearImage(VulkanContextResources* vulkanContextResources, ResourceHandle imageHandle,
                const Core::Math::v4f& clearValue, const char* debugLabel, bool immediateSubmit);
        }
    }
}
