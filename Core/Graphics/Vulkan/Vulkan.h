#pragma once

#include "CoreDefines.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"

namespace Tk
{
namespace Platform
{
struct PlatformWindowHandles;
}
}

namespace Tk
{
namespace Core
{
namespace Graphics
{

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

// Init/destroy - called one time
int InitVulkan(const Tk::Platform::PlatformWindowHandles* platformWindowHandles, uint32 width, uint32 height);
void DestroyVulkan();

void VulkanCreateSwapChain();
void VulkanDestroySwapChain();
void CreateSamplers();

// Frame command recording
bool VulkanAcquireFrame();
void VulkanSubmitFrame();

void BeginVulkanCommandRecording();
void EndVulkanCommandRecording();
void BeginVulkanCommandRecordingImmediate();
void EndVulkanCommandRecordingImmediate();

// Graphics API - resource create/destroy functions
ResourceHandle VulkanCreateResource(const ResourceDesc& resDesc);
void VulkanDestroyResource(ResourceHandle handle);

FramebufferHandle VulkanCreateFramebuffer(ResourceHandle* rtColorHandles, uint32 numRTColorHandles,
    ResourceHandle rtDepthHandle, uint32 width, uint32 height, uint32 renderPassID);
void VulkanDestroyFramebuffer(FramebufferHandle handle);

bool VulkanCreateGraphicsPipeline(void* vertexShaderCode, uint32 numVertexShaderBytes,
    void* fragmentShaderCode, uint32 numFragmentShaderBytes,
    uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight,
    uint32 numColorRTs, const uint32* colorRTFormats, uint32 depthFormat,
    uint32* descriptorLayoutHandles, uint32 numDescriptorLayoutHandles);
void DestroyPSOPerms(uint32 shaderID);
void VulkanDestroyAllPSOPerms();
void DestroyAllDescLayouts();

bool VulkanCreateDescriptorLayout(uint32 descriptorLayoutID, const DescriptorLayout* descriptorLayout);
DescriptorHandle VulkanCreateDescriptor(uint32 descriptorLayoutID);
void VulkanDestroyDescriptor(DescriptorHandle handle);
void VulkanDestroyAllDescriptors();
void VulkanWriteDescriptor(uint32 descriptorLayoutID, DescriptorHandle descSetHandle, const DescriptorSetDataHandles* descSetDataHandles, uint32 descSetDataCount);
void InitDescriptorPool();

// Memory mapping - probably just for staging buffers
void* VulkanMapResource(ResourceHandle handle);
void VulkanUnmapResource(ResourceHandle handle);

// Graphics command recording
void VulkanRecordCommandPushConstant(uint8* data, uint32 sizeInBytes, uint32 shaderID, uint32 blendState, uint32 depthState);
void VulkanRecordCommandDrawCall(ResourceHandle indexBufferHandle, uint32 numIndices,
    uint32 numInstances, const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandBindShader(uint32 shaderID, uint32 blendState, uint32 depthState,
    const DescriptorHandle* descSetHandles, bool immediateSubmit);
void VulkanRecordCommandMemoryTransfer(uint32 sizeInBytes, ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle,
    const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandRenderPassBegin(uint32 numColorRTs, const ResourceHandle* colorRTs, ResourceHandle depthRT,
    uint32 renderWidth, uint32 renderHeight, const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandRenderPassEnd(bool immediateSubmit);
void VulkanRecordCommandTransitionLayout(ResourceHandle imageHandle, uint32 startLayout, uint32 endLayout,
    const char* debugLabel, bool immediateSubmit);
void VulkanRecordCommandClearImage(ResourceHandle imageHandle,
    const v4f& clearValue, const char* debugLabel, bool immediateSubmit);

}
}
}
