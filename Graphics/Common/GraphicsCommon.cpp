#include "GraphicsCommon.h"
#include "GPUTimestamps.h"
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

static DefaultTexture DefaultTextures[DefaultTextureID::eMax] = {};

void CreateContext(const Tk::Platform::WindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight)
{
    int result = 0;

    #ifdef VULKAN
    result = InitVulkan(windowHandles, windowWidth, windowHeight);
    #endif

    if (result)
        Core::Utility::LogMsg("Platform", "Failed to init graphics backend!", Core::Utility::LogSeverity::eCritical);
}

void RecreateContext(const Tk::Platform::WindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight)
{
    DestroyContext();

    #ifdef VULKAN
    InitVulkan(windowHandles, windowWidth, windowHeight);
    #endif
}

void WindowResize()
{
    DestroyAllPSOPerms();
    DestroySwapChain();
    CreateSwapChain();
}

bool AcquireFrame()
{
    #ifdef VULKAN
    return g_vulkanContextResources.isSwapChainValid && VulkanAcquireFrame();
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
            TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < GraphicsCommand::eMax);

            const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[i];

            switch (currentCmd.m_commandType)
            {
                case GraphicsCommand::eDrawCall:
                {
                    const bool psoChange =
                        currentShaderID != currentCmd.m_shader ||
                        (currentBlendState != currentCmd.m_blendState) ||
                        (currentDepthState != currentCmd.m_depthState);
                    
                    currentShaderID = currentCmd.m_shader;
                    currentBlendState = currentCmd.m_blendState;
                    currentDepthState = currentCmd.m_depthState;

                    if (psoChange)
                    {
                        RecordCommandBindShader(currentShaderID, currentBlendState, currentDepthState, immediateSubmit);
                    }

                    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
                    {
                        if (currDescriptors[uiDesc] != currentCmd.m_descriptors[uiDesc])
                        {
                            currDescriptors[uiDesc] = currentCmd.m_descriptors[uiDesc];

                            if (currentCmd.m_descriptors[uiDesc] != Graphics::DefaultDescHandle_Invalid)
                            {
                                RecordCommandBindDescriptor(currentShaderID, BindPoint::eGraphics, currentCmd.m_descriptors[uiDesc], uiDesc, immediateSubmit);
                            }
                        }
                    }

                    RecordCommandDrawCall(currentCmd.m_indexBufferHandle, currentCmd.m_numIndices,
                        currentCmd.m_numInstances, currentCmd.m_vertOffset, currentCmd.m_indexOffset,
                        currentCmd.debugLabel, immediateSubmit);
                    break;
                }

                case GraphicsCommand::eDispatch:
                {
                    // currently binds pso unconditionally, but it's probably fine 
                    RecordCommandBindComputeShader(currentCmd.m_shader, immediateSubmit);

                    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
                    {
                        if (currDescriptors[uiDesc] != currentCmd.m_descriptors[uiDesc])
                        {
                            currDescriptors[uiDesc] = currentCmd.m_descriptors[uiDesc];
                            
                            if (currentCmd.m_descriptors[uiDesc] != Graphics::DefaultDescHandle_Invalid)
                            {
                                RecordCommandBindDescriptor(currentCmd.m_shader, BindPoint::eCompute, currentCmd.m_descriptors[uiDesc], uiDesc, immediateSubmit);
                            }
                        }
                    }

                    RecordCommandDispatch(currentCmd.m_threadGroupsX, currentCmd.m_threadGroupsY, currentCmd.m_threadGroupsZ, currentCmd.debugLabel, immediateSubmit);
                    break;
                }

                case GraphicsCommand::eMemTransfer:
                {
                    RecordCommandMemoryTransfer(currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle,
                        currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCommand::ePushConstant:
                {
                    RecordCommandPushConstant(&currentCmd.m_pushConstantData[0], ARRAYCOUNT(currentCmd.m_pushConstantData) * sizeof(uint8), currentCmd.m_shaderForLayout);

                    break;
                }

                case GraphicsCommand::eSetScissor:
                {
                    RecordCommandSetScissor(currentCmd.m_scissorOffsetX, currentCmd.m_scissorOffsetY, currentCmd.m_scissorWidth, currentCmd.m_scissorHeight);

                    break;
                }

                case GraphicsCommand::eRenderPassBegin:
                {
                    RecordCommandRenderPassBegin(currentCmd.m_numColorRTs, &currentCmd.m_colorRTs[0], currentCmd.m_depthRT,
                        currentCmd.m_renderWidth, currentCmd.m_renderHeight, currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCommand::eRenderPassEnd:
                {
                    RecordCommandRenderPassEnd(immediateSubmit);

                    break;
                }

                case GraphicsCommand::eLayoutTransition:
                {
                    RecordCommandTransitionLayout(currentCmd.m_imageHandle,
                        currentCmd.m_startLayout, currentCmd.m_endLayout,
                        currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCommand::eClearImage:
                {
                    RecordCommandClearImage(currentCmd.m_imageHandle,
                        currentCmd.m_clearValue, currentCmd.debugLabel, immediateSubmit);

                    break;
                }

                case GraphicsCommand::eGPUTimestamp:
                {
                    TINKER_ASSERT(GPUTimestamps::GetMostRecentRecordedTimestampCount() <= GPU_TIMESTAMP_NUM_MAX);
                    if (currentCmd.m_timestampStartFrame)
                    {
                        void* cpuCopyBuffer = GPUTimestamps::GetRawCPUSideTimestampBuffer();
                        const uint32 numTimestampsRecorded = GPUTimestamps::GetMostRecentRecordedTimestampCount();
                        ResolveMostRecentAvailableTimestamps(cpuCopyBuffer, numTimestampsRecorded, immediateSubmit);
                        GPUTimestamps::ProcessTimestamps();
                    }

                    RecordCommandGPUTimestamp(GPUTimestamps::GetMostRecentRecordedTimestampCount(), immediateSubmit);
                    GPUTimestamps::RecordName(currentCmd.m_timestampNameStr);

                    break;
                }

                case GraphicsCommand::eDebugMarkerStart:
                {
                    RecordCommandDebugMarkerStart(immediateSubmit, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eDebugMarkerEnd:
                {
                    RecordCommandDebugMarkerEnd(immediateSubmit);
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
    BeginVulkanCommandRecordingImmediate();
    ProcessGraphicsCommandStream(graphicsCommandStream, true);
    EndVulkanCommandRecordingImmediate();
    #endif
}

void CreateAllDefaultTextures(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    DefaultTexture& tex = DefaultTextures[DefaultTextureID::eBlack2x2];
    ResourceDesc desc = {};
    desc.resourceType = ResourceType::eImage2D;
    desc.debugLabel = "Default Texture Black2x2";
    desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB;
    desc.dims = v3ui(2, 2, 1);
    desc.imageUsageFlags = Tk::Graphics::ImageUsageFlags::Sampled | Tk::Graphics::ImageUsageFlags::TransferDst;
    desc.arrayEles = 1;
    tex.res = CreateResource(desc);
    tex.clearValue = v4f(0.0, 0.0, 0.0, 0.0);
    
    for (uint32 i = 0; i < DefaultTextureID::eMax; ++i)
    {
        DefaultTexture& currTex = DefaultTextures[i];
        
        graphicsCommandStream->CmdTransitionLayout(currTex.res, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eTransferDst, "Transition default texture to clear optimal");
        graphicsCommandStream->CmdClear(currTex.res, currTex.clearValue, "Clear default texture");
        graphicsCommandStream->CmdTransitionLayout(currTex.res, Graphics::ImageLayout::eTransferDst, Graphics::ImageLayout::eShaderRead, "Transition default texture to shader read");
    }

    Graphics::SubmitCmdsImmediate(graphicsCommandStream);
    graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream
}

void DestroyDefaultTextures()
{
    for (uint32 i = 0; i < DefaultTextureID::eMax; ++i)
    {
        DefaultTexture& currTex = DefaultTextures[i];
        Graphics::DestroyResource(currTex.res);
        currTex.res = Graphics::DefaultResHandle_Invalid;
    }
}

DefaultTexture GetDefaultTexture(uint32 defaultTexID)
{
    TINKER_ASSERT(defaultTexID < DefaultTextureID::eMax);
    return DefaultTextures[defaultTexID];
}

}
}
