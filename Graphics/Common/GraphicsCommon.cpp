#include "GraphicsCommon.h"
#include "Platform/PlatformCommon.h"
#include "Utility/Logging.h"

#ifdef VULKAN
#include "Graphics/Vulkan/Vulkan.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#endif

namespace Tk
{
namespace Graphics
{

uint32 MultiBufferedStatusFromBufferUsage[] = 
{
    0u,
    0u,
    1u,
    1u,
    0u,
    1u,
};
static_assert(ARRAYCOUNT(MultiBufferedStatusFromBufferUsage) == BufferUsage::eMax); // Don't forget to add one here if enum is added to

void CreateContext(const Tk::Platform::PlatformWindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight)
{
    int result = 0;

    #ifdef VULKAN
    result = InitVulkan(windowHandles, windowWidth, windowHeight);
    #endif

    if (result)
        Core::Utility::LogMsg("Platform", "Failed to init graphics backend!", Core::Utility::LogSeverity::eCritical);
}

void RecreateContext(const Tk::Platform::PlatformWindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight)
{
    #ifdef VULKAN
    DestroyVulkan();
    InitVulkan(windowHandles, windowWidth, windowHeight);
    #endif
}

void WindowResize()
{
    #ifdef VULKAN
    VulkanDestroyAllPSOPerms();
    VulkanDestroySwapChain();
    VulkanCreateSwapChain();
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
    DestroyVulkan();
    #endif
}

void DestroyAllPSOPerms()
{
    #ifdef VULKAN
    VulkanDestroyAllPSOPerms();
    #endif
}

bool AcquireFrame()
{
    #ifdef VULKAN
    if (g_vulkanContextResources.isSwapChainValid)
        return VulkanAcquireFrame();
    
    return false;
    #else
    return false;
    #endif
}

void ProcessGraphicsCommandStream(const GraphicsCommandStream* graphicsCommandStream, bool immediateSubmit)
{
    TINKER_ASSERT(graphicsCommandStream->m_numCommands <= graphicsCommandStream->m_maxCommands);

    const bool multithreadedCmdRecording = false;

    if (multithreadedCmdRecording)
    {
        // TODO: implement :)
    }
    else
    {
        uint32 currentShaderID = SHADER_ID_MAX;
        uint32 currentBlendState = BlendState::eMax;
        uint32 currentDepthState = DepthState::eMax;
        DescriptorHandle currDescriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        {
            currDescriptors[i] = Graphics::DefaultDescHandle_Invalid;
        }

        for (uint32 i = 0; i < graphicsCommandStream->m_numCommands; ++i)
        {
            TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < GraphicsCmd::eMax);

            const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[i];

            switch (currentCmd.m_commandType)
            {
                case GraphicsCmd::eDrawCall:
                {
                    // TODO: this is kind of suboptimal, probably restructure the draw call api a little in the future
                    const bool psoChange =
                        currentShaderID != currentCmd.m_shader ||
                        (currentBlendState != currentCmd.m_blendState) ||
                        (currentDepthState != currentCmd.m_depthState);
                    
                    bool descChange = currentShaderID != currentCmd.m_shader;
                    for (uint32 uiDesc = 0; !descChange && uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
                    {
                        descChange = descChange || (currDescriptors[uiDesc] != currentCmd.m_descriptors[uiDesc]);
                    }

                    currentShaderID = currentCmd.m_shader;
                    currentBlendState = currentCmd.m_blendState;
                    currentDepthState = currentCmd.m_depthState;

                    if (psoChange)
                    {
                        RecordCommandBindShader(currentShaderID, currentBlendState, currentDepthState, immediateSubmit);
                    }

                    if (descChange)
                    {
                        RecordCommandBindDescriptor(currentShaderID, &currentCmd.m_descriptors[0], immediateSubmit);
                    }

                    RecordCommandDrawCall(currentCmd.m_indexBufferHandle, currentCmd.m_numIndices,
                        currentCmd.m_numInstances, currentCmd.m_vertOffset, currentCmd.m_indexOffset,
                        currentCmd.debugLabel, immediateSubmit);
                    break;
                }

                case GraphicsCmd::eMemTransfer:
                {
                    RecordCommandMemoryTransfer(currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle,
                        currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCmd::ePushConstant:
                {
                    RecordCommandPushConstant(&currentCmd.m_pushConstantData[0], ARRAYCOUNT(currentCmd.m_pushConstantData) * sizeof(uint8), currentCmd.m_shaderForLayout);

                    break;
                }

                case GraphicsCmd::eSetScissor:
                {
                    RecordCommandSetScissor(currentCmd.m_scissorOffsetX, currentCmd.m_scissorOffsetY, currentCmd.m_scissorWidth, currentCmd.m_scissorHeight);

                    break;
                }

                case GraphicsCmd::eRenderPassBegin:
                {
                    RecordCommandRenderPassBegin(currentCmd.m_numColorRTs, &currentCmd.m_colorRTs[0], currentCmd.m_depthRT,
                        currentCmd.m_renderWidth, currentCmd.m_renderHeight, currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCmd::eRenderPassEnd:
                {
                    RecordCommandRenderPassEnd(immediateSubmit);

                    break;
                }

                case GraphicsCmd::eLayoutTransition:
                {
                    RecordCommandTransitionLayout(currentCmd.m_imageHandle,
                        currentCmd.m_startLayout, currentCmd.m_endLayout,
                        currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCmd::eClearImage:
                {
                    RecordCommandClearImage(currentCmd.m_imageHandle,
                        currentCmd.m_clearValue, currentCmd.debugLabel, immediateSubmit);

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
    BeginVulkanCommandRecording();
    #endif
}

void EndFrameRecording()
{
    #ifdef VULKAN
    EndVulkanCommandRecording();
    #endif
}

void SubmitFrameToGPU()
{
    #ifdef VULKAN
    Graphics::VulkanSubmitFrame();
    #endif
}

SUBMIT_CMDS_IMMEDIATE(SubmitCmdsImmediate)
{
    #ifdef VULKAN
    Graphics::BeginVulkanCommandRecordingImmediate();
    ProcessGraphicsCommandStream(graphicsCommandStream, true);
    Graphics::EndVulkanCommandRecordingImmediate();
    #endif
}

CREATE_RESOURCE(CreateResource)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateResource(resDesc);
    #else
    return Graphics::DefaultResHandle_Invalid;
    #endif
}

DESTROY_RESOURCE(DestroyResource)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyResource(handle);
    #endif
}

MAP_RESOURCE(MapResource)
{
    #ifdef VULKAN
    return Graphics::VulkanMapResource(handle);
    #else
    return NULL;
    #endif
}

UNMAP_RESOURCE(UnmapResource)
{
    #ifdef VULKAN
    Graphics::VulkanUnmapResource(handle);
    #endif
}

CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateGraphicsPipeline(vertexShaderCode, numVertexShaderBytes, fragmentShaderCode, numFragmentShaderBytes,
        shaderID, viewportWidth, viewportHeight, numColorRTs, colorRTFormats, depthFormat, descriptorHandles, numDescriptorHandles);
    #else
    return false;
    #endif
}

CREATE_DESCRIPTOR(CreateDescriptor)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateDescriptor(descLayoutID);
    #else
    return DefaultDescHandle_Invalid;
    #endif
}

DESTROY_DESCRIPTOR(DestroyDescriptor)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyDescriptor(handle);
    #endif
}

DESTROY_ALL_DESCRIPTORS(DestroyAllDescriptors)
{
    #ifdef VULKAN
    Graphics::VulkanDestroyAllDescriptors();
    #endif
}

WRITE_DESCRIPTOR(WriteDescriptor)
{
    #ifdef VULKAN
    Graphics::VulkanWriteDescriptor(descLayoutID, descSetHandle, descSetDataHandles);
    #endif
}

CREATE_DESCRIPTOR_LAYOUT(CreateDescriptorLayout)
{
    #ifdef VULKAN
    return Graphics::VulkanCreateDescriptorLayout(descLayoutID, descLayout);
    #else
    return false;
    #endif

}

DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline)
{
    #ifdef VULKAN
    Graphics::DestroyPSOPerms(shaderID);
    #endif
}

}
}
