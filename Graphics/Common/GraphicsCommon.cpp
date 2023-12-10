#include "GraphicsCommon.h"
#include "GPUTimestamps.h"
#include "DataStructures/HashMap.h"
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

static Tk::Core::HashMap<uint64, SwapChainData, MapHashFn64> SwapChainDataMap;
void CreateSwapChain(const Tk::Platform::WindowHandles* windowHandles, uint32 width, uint32 height)
{
    uint32 dataIndex = SwapChainDataMap.Insert(windowHandles->hWindow, {}); // TODO: make this api better 
    SwapChainData& newSwapChainData = SwapChainDataMap.DataAtIndex(dataIndex);
    newSwapChainData.windowWidth = width;
    newSwapChainData.windowHeight = height;
    CreateSwapChainAPIObjects(&newSwapChainData, windowHandles);
    newSwapChainData.isSwapChainValid = true;
}

void DestroySwapChain(const Tk::Platform::WindowHandles* windowHandles)
{
    // Acquire frame in order to ensure that fences and semaphores are reset 
    //Tk::Graphics::AcquireFrame(windowHandles);

    uint32 dataIndex = SwapChainDataMap.FindIndex(windowHandles->hWindow);
    SwapChainData& swapChainData = SwapChainDataMap.DataAtIndex(dataIndex);
    swapChainData.isSwapChainValid = false;

    DestroySwapChainAPIObjects(&swapChainData);
    SwapChainDataMap.Remove(windowHandles->hWindow);
}

void CreateContext(const Tk::Platform::WindowHandles* windowHandles)
{
    int result = 0;

    #ifdef VULKAN
    result = InitVulkan(windowHandles);
    #endif

    if (result)
    {
        Core::Utility::LogMsg("Platform", "Failed to init graphics backend!", Core::Utility::LogSeverity::eCritical);
    }

    SwapChainDataMap.Reserve(NUM_SWAP_CHAINS_STARTING_ALLOC_SIZE); // TODO the hash map data structure needs resize still so...
}

void DestroyContext()
{
    #ifdef VULKAN
    DestroyVulkan();
    #endif
}

void RecreateContext(const Tk::Platform::WindowHandles* windowHandles)
{
    DestroyContext();

    //TODO: just call CreateContext()?
    #ifdef VULKAN
    InitVulkan(windowHandles);
    #endif
}

void WindowResize(const Tk::Platform::WindowHandles* windowHandles, uint32 newWindowWidth, uint32 newWindowHeight)
{
    DestroySwapChain(windowHandles);
    CreateSwapChain(windowHandles, newWindowWidth, newWindowHeight);
}

void WindowMinimized(const Tk::Platform::WindowHandles* windowHandles)
{
    uint32 dataIndex = SwapChainDataMap.FindIndex(windowHandles->hWindow);
    SwapChainData& swapChainData = SwapChainDataMap.DataAtIndex(dataIndex);
    swapChainData.isSwapChainValid = false;
}

ResourceHandle GetCurrentSwapChainImage(const Tk::Platform::WindowHandles* windowHandles)
{
    uint32 dataIndex = SwapChainDataMap.FindIndex(windowHandles->hWindow);
    SwapChainData& swapChainData = SwapChainDataMap.DataAtIndex(dataIndex);
    return swapChainData.swapChainResourceHandles[swapChainData.currentSwapChainImage];
}

bool AcquireFrame(const Tk::Platform::WindowHandles* windowHandles)
{
    uint32 dataIndex = SwapChainDataMap.FindIndex(windowHandles->hWindow);
    SwapChainData& swapChainData = SwapChainDataMap.DataAtIndex(dataIndex);
    if (swapChainData.isSwapChainValid)
    {
        #ifdef VULKAN
        return VulkanAcquireFrame(&swapChainData);
        #else
        return false;
        #endif
    }
    else
    {
        return false;
    }
}

void ProcessGraphicsCommandStream(const GraphicsCommandStream* graphicsCommandStream)
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
        for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
        {
            currDescriptors[uiDesc] = Graphics::DefaultDescHandle_Invalid;
        }
        
        CommandBuffer currentCmdBuf = {};

        for (uint32 uiCmd = 0; uiCmd  < graphicsCommandStream->m_numCommands; ++uiCmd)
        {
            TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[uiCmd].m_commandType < GraphicsCommand::eMax);
            const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[uiCmd];

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
                        RecordCommandBindShader(currentCmdBuf, currentShaderID, currentBlendState, currentDepthState);
                    }
                    
                    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
                    {
                        if (currDescriptors[uiDesc] != currentCmd.m_descriptors[uiDesc])
                        {
                            currDescriptors[uiDesc] = currentCmd.m_descriptors[uiDesc];

                            if (currentCmd.m_descriptors[uiDesc] != Graphics::DefaultDescHandle_Invalid)
                            {
                                RecordCommandBindDescriptor(currentCmdBuf, currentShaderID, BindPoint::eGraphics, currentCmd.m_descriptors[uiDesc], uiDesc);
                            }
                        }
                    }

                    RecordCommandDrawCall(currentCmdBuf, currentCmd.m_indexBufferHandle,
                        currentCmd.m_numIndices, currentCmd.m_numInstances, currentCmd.m_vertexOffset,
                        currentCmd.m_indexOffset, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eDispatch:
                {
                    // currently binds pso unconditionally, but it's probably fine 
                    RecordCommandBindComputeShader(currentCmdBuf, currentCmd.m_shader);

                    for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDesc)
                    {
                        if (currDescriptors[uiDesc] != currentCmd.m_descriptors[uiDesc])
                        {
                            currDescriptors[uiDesc] = currentCmd.m_descriptors[uiDesc];
                            
                            if (currentCmd.m_descriptors[uiDesc] != Graphics::DefaultDescHandle_Invalid)
                            {
                                RecordCommandBindDescriptor(currentCmdBuf, currentCmd.m_shader, BindPoint::eCompute, currentCmd.m_descriptors[uiDesc], uiDesc);
                            }
                        }
                    }

                    RecordCommandDispatch(currentCmdBuf, currentCmd.m_threadGroupsX, currentCmd.m_threadGroupsY, currentCmd.m_threadGroupsZ, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eCopy:
                {
                    RecordCommandMemoryTransfer(currentCmdBuf, currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle,
                        currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::ePushConstant:
                {
                    RecordCommandPushConstant(currentCmdBuf, &currentCmd.m_pushConstantData[0], ARRAYCOUNT(currentCmd.m_pushConstantData) * sizeof(uint8),
                        currentCmd.m_shaderForLayout);
                    break;
                }

                case GraphicsCommand::eSetViewport:
                {
                    RecordCommandSetViewport(currentCmdBuf, currentCmd.m_viewportOffsetX, currentCmd.m_viewportOffsetY, currentCmd.m_viewportWidth,
                        currentCmd.m_viewportHeight, currentCmd.m_viewportMinDepth, currentCmd.m_viewportMaxDepth);
                    break;
                }

                case GraphicsCommand::eSetScissor:
                {
                    RecordCommandSetScissor(currentCmdBuf, currentCmd.m_scissorOffsetX, currentCmd.m_scissorOffsetY, currentCmd.m_scissorWidth, currentCmd.m_scissorHeight);
                    break;
                }

                case GraphicsCommand::eCmdBufferBegin:
                {
                    TINKER_ASSERT(currentCmdBuf == CommandBuffer());
                    BeginCommandRecording(currentCmd.m_commandBuffer);
                    currentCmdBuf = currentCmd.m_commandBuffer;
                    break;
                }

                case GraphicsCommand::eCmdBufferEnd:
                {
                    TINKER_ASSERT(currentCmdBuf != CommandBuffer());
                    TINKER_ASSERT(currentCmd.m_commandBuffer == currentCmdBuf);
                    EndCommandRecording(currentCmd.m_commandBuffer);
                    currentCmdBuf = {};
                    break;
                }

                case GraphicsCommand::eRenderPassBegin:
                {
                    RecordCommandRenderPassBegin(currentCmdBuf, currentCmd.m_numColorRTs, &currentCmd.m_colorRTs[0], currentCmd.m_depthRT,
                        currentCmd.m_renderWidth, currentCmd.m_renderHeight, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eRenderPassEnd:
                {
                    RecordCommandRenderPassEnd(currentCmdBuf);
                    break;
                }

                case GraphicsCommand::eLayoutTransition:
                {
                    RecordCommandTransitionLayout(currentCmdBuf, currentCmd.m_imageHandle,
                        currentCmd.m_startLayout, currentCmd.m_endLayout, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eClearImage:
                {
                    RecordCommandClearImage(currentCmdBuf, currentCmd.m_imageHandle,
                        currentCmd.m_clearValue, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eGPUTimestamp:
                {
                    TINKER_ASSERT(GPUTimestamps::GetMostRecentRecordedTimestampCount() <= GPU_TIMESTAMP_NUM_MAX);
                    if (currentCmd.m_timestampStartFrame)
                    {
                        void* cpuCopyBuffer = GPUTimestamps::GetRawCPUSideTimestampBuffer();
                        const uint32 numTimestampsRecorded = GPUTimestamps::GetMostRecentRecordedTimestampCount();
                        ResolveMostRecentAvailableTimestamps(currentCmdBuf, cpuCopyBuffer, numTimestampsRecorded);
                        GPUTimestamps::ProcessTimestamps();
                    }

                    RecordCommandGPUTimestamp(currentCmdBuf, GPUTimestamps::GetMostRecentRecordedTimestampCount());
                    GPUTimestamps::RecordName(currentCmd.m_timestampNameStr);
                    break;
                }

                case GraphicsCommand::eDebugMarkerStart:
                {
                    RecordCommandDebugMarkerStart(currentCmdBuf, currentCmd.debugLabel);
                    break;
                }

                case GraphicsCommand::eDebugMarkerEnd:
                {
                    RecordCommandDebugMarkerEnd(currentCmdBuf);
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

void SubmitFrameToGPU(const Tk::Platform::WindowHandles* windowHandles, CommandBuffer commandBuffer)
{
    uint32 dataIndex = SwapChainDataMap.FindIndex(windowHandles->hWindow);
    SwapChainData& swapChainData = SwapChainDataMap.DataAtIndex(dataIndex);

    #ifdef VULKAN
    VulkanSubmitFrame(&swapChainData, commandBuffer);
    #endif
}

void PresentToSwapChain(const Tk::Platform::WindowHandles* windowHandles)
{
    uint32 dataIndex = SwapChainDataMap.FindIndex(windowHandles->hWindow);
    SwapChainData& swapChainData = SwapChainDataMap.DataAtIndex(dataIndex);

    #ifdef VULKAN
    VulkanPresentToSwapChain(&swapChainData);
    #endif
}

SUBMIT_CMDS_IMMEDIATE(SubmitCmdsImmediate)
{
    ProcessGraphicsCommandStream(graphicsCommandStream);

    #ifdef VULKAN
    VulkanSubmitCommandBufferAndWaitImmediate(commandBuffer);
    #endif
}

void CreateAllDefaultTextures(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    CommandBuffer cmdBuf = CreateCommandBuffer();

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
    
    graphicsCommandStream->CmdCommandBufferBegin(cmdBuf, "Begin default texture creation cmd buffer");

    for (uint32 i = 0; i < DefaultTextureID::eMax; ++i)
    {
        DefaultTexture& currTex = DefaultTextures[i];
        graphicsCommandStream->CmdLayoutTransition(currTex.res, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eTransferDst, "Transition default texture to clear optimal");
        graphicsCommandStream->CmdClear(currTex.res, currTex.clearValue, "Clear default texture");
        graphicsCommandStream->CmdLayoutTransition(currTex.res, Graphics::ImageLayout::eTransferDst, Graphics::ImageLayout::eShaderRead, "Transition default texture to shader read");
    }

    graphicsCommandStream->CmdCommandBufferEnd(cmdBuf);
    Graphics::SubmitCmdsImmediate(graphicsCommandStream, cmdBuf);
    graphicsCommandStream->Clear();
    //graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream
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
