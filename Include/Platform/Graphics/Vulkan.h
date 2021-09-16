#pragma once

#include "Core/CoreDefines.h"
#include "Core/Math/VectorTypes.h"
#include "PlatformGameAPI.h"
#include "Platform/ShaderManager.h"

#ifdef _WIN32
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#else
// TODO: include other platform headers
#endif

namespace Tk
{
namespace Platform
{
namespace Graphics
{

struct VkResources; // forward declare - this is defined in VulkanTypes.h
struct VulkanContextResources
{
    bool isInitted = false;
    bool isSwapChainValid = false;
    VkResources* resources = nullptr;
};

typedef struct vulkan_vertex_position
{
    v4f position;
} VulkanVertexPosition;

typedef struct vulkan_vertex_uv
{
    v2f uv;
} VulkanVertexUV;

typedef struct vulkan_vertex_normal
{
    v3f normal;
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
int InitVulkan(VulkanContextResources* vulkanContextResources, const PlatformWindowHandles* platformWindowHandles, uint32 width, uint32 height, uint32 numThreads);
void DestroyVulkan(VulkanContextResources* vulkanContextResources);

void VulkanCreateSwapChain(VulkanContextResources* vulkanContextResources);
void VulkanDestroySwapChain(VulkanContextResources* vulkanContextResources);
void CreateSamplers(VulkanContextResources* vulkanContextResources);

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
    uint32 width, uint32 height, uint32 renderPassID);
void VulkanDestroyFramebuffer(VulkanContextResources* vulkanContextResources, FramebufferHandle handle);

bool VulkanCreateGraphicsPipeline(VulkanContextResources* vulkanContextResources,
    void* vertexShaderCode, uint32 numVertexShaderBytes,
    void* fragmentShaderCode, uint32 numFragmentShaderBytes,
    uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID,
    uint32* descriptorLayoutHandles, uint32 numDescriptorLayoutHandles);
void DestroyPSOPerms(VulkanContextResources* vulkanContextResources, uint32 shaderID);
void VulkanDestroyAllPSOPerms(VulkanContextResources* vulkanContextResources);
void DestroyAllDescLayouts(VulkanContextResources* vulkanContextResources);
void VulkanDestroyAllRenderPasses(VulkanContextResources* vulkanContextResources);

bool VulkanCreateDescriptorLayout(VulkanContextResources* vulkanContextResources, uint32 descriptorLayoutID, const Platform::DescriptorLayout* descriptorLayout);
DescriptorHandle VulkanCreateDescriptor(VulkanContextResources* vulkanContextResources, uint32 descriptorLayoutID);
void VulkanDestroyDescriptor(VulkanContextResources* vulkanContextResources, DescriptorHandle handle);
void VulkanDestroyAllDescriptors(VulkanContextResources* vulkanContextResources);
void VulkanWriteDescriptor(VulkanContextResources* vulkanContextResources, uint32 descriptorLayoutID, DescriptorHandle* descSetHandles, uint32 descSetCount, DescriptorSetDataHandles* descSetDataHandles, uint32 descSetDataCount);
void InitDescriptorPool(VulkanContextResources* vulkanContextResources);
bool VulkanCreateRenderPass(VulkanContextResources* vulkanContextResources, uint32 renderPassID, uint32 numColorRTs, uint32 colorFormat, uint32 startLayout, uint32 endLayout, uint32 depthFormat);

// Memory mapping - probably just for staging buffers
void* VulkanMapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle);
void VulkanUnmapResource(VulkanContextResources* vulkanContextResources, ResourceHandle handle);

// Graphics command recording
void VulkanRecordCommandPushConstant(VulkanContextResources* vulkanContextResources, uint8* data, uint32 sizeInBytes, uint32 shaderID, uint32 blendState, uint32 depthState);
void VulkanRecordCommandDrawCall(VulkanContextResources* vulkanContextResources,
    ResourceHandle indexBufferHandle, uint32 numIndices,
    uint32 numInstances, const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandBindShader(VulkanContextResources* vulkanContextResources,
    uint32 shaderID, uint32 blendState, uint32 depthState,
    const DescriptorHandle* descSetHandles, bool immediateSubmit);
void VulkanRecordCommandMemoryTransfer(VulkanContextResources* vulkanContextResources,
    uint32 sizeInBytes, ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle,
    const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandRenderPassBegin(VulkanContextResources* vulkanContextResources,
    FramebufferHandle framebufferHandle, uint32 renderPassID, uint32 renderWidth, uint32 renderHeight,
    const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandRenderPassEnd(VulkanContextResources* vulkanContextResources, bool immediateSubmit);
void VulkanRecordCommandTransitionLayout(VulkanContextResources*  vulkanContextResources, ResourceHandle imageHandle,
    uint32 startLayout, uint32 endLayout, const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandClearImage(VulkanContextResources* vulkanContextResources, ResourceHandle imageHandle,
    const v4f& clearValue, const char* debugLabel, bool immediateSubmit);

}
}
}
