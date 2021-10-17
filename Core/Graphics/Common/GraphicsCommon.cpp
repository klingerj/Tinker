#include "GraphicsCommon.h"
#include "Platform/PlatformCommon.h"
#include "Utility/Logging.h"

#ifdef VULKAN
#include "Graphics/Vulkan/Vulkan.h"
#endif

namespace Tk
{
namespace Core
{
namespace Graphics
{

void CreateContext(const Tk::Platform::PlatformWindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight)
{
    int result = 0;

    #ifdef VULKAN
    g_vulkanContextResources = {};
    result = InitVulkan(&g_vulkanContextResources, windowHandles, windowWidth, windowHeight);
    #endif

    if (result)
        Core::Utility::LogMsg("Platform", "Failed to init graphics backend!", Core::Utility::LogSeverity::eCritical);
}

void RecreateContext(const Tk::Platform::PlatformWindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight)
{
    #ifdef VULKAN
    DestroyVulkan(&g_vulkanContextResources);
    InitVulkan(&g_vulkanContextResources, windowHandles, windowWidth, windowHeight);
    #endif
}

void WindowResize()
{
    #ifdef VULKAN
    VulkanDestroyAllPSOPerms(&g_vulkanContextResources);
    VulkanDestroyAllRenderPasses(&g_vulkanContextResources);
    VulkanDestroySwapChain(&g_vulkanContextResources);
    VulkanCreateSwapChain(&g_vulkanContextResources);
    #endif
}

void WindowMinimized()
{
    #ifdef VULKAN
    g_vulkanContextResources.isSwapChainValid = false;
    #endif
}

void DestroyContext()
{
    #ifdef VULKAN
    DestroyVulkan(&g_vulkanContextResources);
    #endif
}

void DestroyAllPSOPerms()
{
    #ifdef VULKAN
    VulkanDestroyAllPSOPerms(&g_vulkanContextResources);
    #endif
}

bool AcquireFrame()
{
    #ifdef VULKAN
    if (g_vulkanContextResources.isSwapChainValid)
        return AcquireFrame(&g_vulkanContextResources);
    
    return false;
    #endif
}

void ProcessGraphicsCommandStream(const GraphicsCommandStream* graphicsCommandStream, bool immediateSubmit)
{
    TINKER_ASSERT(graphicsCommandStream->m_numCommands <= graphicsCommandStream->m_maxCommands);

    const bool multithreadedCmdRecording = false;

    if (multithreadedCmdRecording)
    {
        // TODO: do this
    }
    else
    {
        // Track number of instances for proper indexing into uniform buffer of instance data
        uint32 instanceCount = 0;
        uint32 currentShaderID = SHADER_ID_MAX;
        uint32 currentBlendState = BlendState::eMax;
        uint32 currentDepthState = DepthState::eMax;

        for (uint32 i = 0; i < graphicsCommandStream->m_numCommands; ++i)
        {
            TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < GraphicsCmd::eMax);

            const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[i];

            switch (currentCmd.m_commandType)
            {
                case GraphicsCmd::eDrawCall:
                {
                    #ifdef VULKAN
                    currentShaderID   = currentCmd.m_shader;
                    currentBlendState = currentCmd.m_blendState;
                    currentDepthState = currentCmd.m_depthState;

                    Graphics::VulkanRecordCommandBindShader(&g_vulkanContextResources,
                        currentShaderID, currentBlendState, currentDepthState,
                        &currentCmd.m_descriptors[0], immediateSubmit);

                    // Push constant
                    {
                        uint32 data[4] = {};
                        data[0] = instanceCount;
                        Graphics::VulkanRecordCommandPushConstant(&g_vulkanContextResources, (uint8*)data, sizeof(uint32) * 4, currentShaderID, currentBlendState, currentDepthState);
                    }

                    Graphics::VulkanRecordCommandDrawCall(&g_vulkanContextResources,
                        currentCmd.m_indexBufferHandle, currentCmd.m_numIndices,
                        currentCmd.m_numInstances, currentCmd.debugLabel, immediateSubmit);

                    #endif

                    instanceCount += currentCmd.m_numInstances;
                    break;
                }

                case GraphicsCmd::eMemTransfer:
                {
                    #ifdef VULKAN
                    Graphics::VulkanRecordCommandMemoryTransfer(&g_vulkanContextResources,
                        currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle,
                        currentCmd.debugLabel, immediateSubmit);
                    #endif

                    break;
                }

                case GraphicsCmd::eRenderPassBegin:
                {
                    instanceCount = 0;
                    #ifdef VULKAN
                    Graphics::VulkanRecordCommandRenderPassBegin(&g_vulkanContextResources,
                        currentCmd.m_framebufferHandle, currentCmd.m_renderPassID,
                        currentCmd.m_renderWidth, currentCmd.m_renderHeight,
                        currentCmd.debugLabel, immediateSubmit);
                    #endif

                    break;
                }

                case GraphicsCmd::eRenderPassEnd:
                {
                    #ifdef VULKAN
                    Graphics::VulkanRecordCommandRenderPassEnd(&g_vulkanContextResources, immediateSubmit);
                    #endif

                    break;
                }

                case GraphicsCmd::eLayoutTransition:
                {
                    #ifdef VULKAN
                    Graphics::VulkanRecordCommandTransitionLayout(&g_vulkanContextResources, currentCmd.m_imageHandle,
                        currentCmd.m_startLayout, currentCmd.m_endLayout,
                        currentCmd.debugLabel, immediateSubmit);
                    #endif

                    break;
                }

                case GraphicsCmd::eClearImage:
                {
                    #ifdef VULKAN
                    Graphics::VulkanRecordCommandClearImage(&g_vulkanContextResources, currentCmd.m_imageHandle,
                        currentCmd.m_clearValue, currentCmd.debugLabel, immediateSubmit);
                    #endif

                    break;
                }

                default:
                {
                    // Invalid command type
                    TINKER_ASSERT(0);
                    break;
                }
            }
        }
    }
}

void BeginFrameRecording()
{
    #ifdef VULKAN
    BeginVulkanCommandRecording(&g_vulkanContextResources);
    #endif
}

void EndFrameRecording()
{
    #ifdef VULKAN
    EndVulkanCommandRecording(&g_vulkanContextResources);
    #endif
}

void SubmitFrameToGPU()
{
    #ifdef VULKAN
    Graphics::VulkanSubmitFrame(&g_vulkanContextResources);
    #endif
}

SUBMIT_CMDS_IMMEDIATE(SubmitCmdsImmediate)
{
    #ifdef VULKAN
    Graphics::BeginVulkanCommandRecordingImmediate(&g_vulkanContextResources);
    ProcessGraphicsCommandStream(graphicsCommandStream, true);
    Graphics::EndVulkanCommandRecordingImmediate(&g_vulkanContextResources);
    #endif
}

CREATE_RESOURCE(CreateResource)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateResource(&g_vulkanContextResources, resDesc);
    #endif
}

DESTROY_RESOURCE(DestroyResource)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyResource(&g_vulkanContextResources, handle);
    #endif
}

MAP_RESOURCE(MapResource)
{
    #ifdef VULKAN
    return Graphics::VulkanMapResource(&g_vulkanContextResources, handle);
    #endif
}

UNMAP_RESOURCE(UnmapResource)
{
    #ifdef VULKAN
    Graphics::VulkanUnmapResource(&g_vulkanContextResources, handle);
    #endif
}

CREATE_FRAMEBUFFER(CreateFramebuffer)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateFramebuffer(&g_vulkanContextResources,
        rtColorHandles, numRTColorHandles, rtDepthHandle,
        width, height, renderPassID);
    #endif
}

DESTROY_FRAMEBUFFER(DestroyFramebuffer)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyFramebuffer(&g_vulkanContextResources, handle);
    #endif
}

CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateGraphicsPipeline(&g_vulkanContextResources,
        vertexShaderCode, numVertexShaderBytes, fragmentShaderCode, numFragmentShaderBytes,
        shaderID, viewportWidth, viewportHeight, renderPassID, descriptorHandles, numDescriptorHandles);
    #endif
}

CREATE_DESCRIPTOR(CreateDescriptor)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateDescriptor(&g_vulkanContextResources, descLayoutID);
    #endif
}

DESTROY_DESCRIPTOR(DestroyDescriptor)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyDescriptor(&g_vulkanContextResources, handle);
    #endif
}

DESTROY_ALL_DESCRIPTORS(DestroyAllDescriptors)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyAllDescriptors(&g_vulkanContextResources);
    #endif
}

WRITE_DESCRIPTOR(WriteDescriptor)
{
    #ifdef VULKAN
    Graphics::VulkanWriteDescriptor(&g_vulkanContextResources, descLayoutID, descSetHandle, descSetDataHandles, descSetDataCount);
    #endif
}

CREATE_DESCRIPTOR_LAYOUT(CreateDescriptorLayout)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateDescriptorLayout(&g_vulkanContextResources, descLayoutID, descLayout);
    #endif
}

CREATE_RENDERPASS(CreateRenderPass)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateRenderPass(&g_vulkanContextResources, renderPassID, numColorRTs, colorFormat, startLayout, endLayout, depthFormat);
    #endif
}

DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline)
{
    #ifdef VULKAN
    Graphics::DestroyPSOPerms(&g_vulkanContextResources, shaderID);
    #endif
}

}
}
}
