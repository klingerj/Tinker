#pragma once

#include "Math/VectorTypes.h"

namespace Tk
{

namespace Platform
{
struct WindowHandles;
}

struct GraphicsCommandStream;

namespace Graphics
{

// Enums
namespace DescriptorType
{
    enum : uint32
    {
        eBuffer = 0,
        eDynamicBuffer,
        eSampledImage,
        eSSBO,
        eStorageImage,
        eArrayOfTextures,
        eMax
    };
}

namespace BufferUsage
{
    enum : uint32
    {
        eVertex = 0,
        eIndex,
        eTransientVertex,
        eTransientIndex,
        eStaging,
        eUniform,
        eMax
    };
}

namespace ImageUsageFlags
{
    enum : uint32
    {
        RenderTarget = 0x1,
        UAV = 0x2,
        TransferDst = 0x4,
        Sampled = 0x8,
        DepthStencil = 0x10,
    };
}

namespace ResourceType
{
    enum : uint32
    {
        eBuffer1D = 0,
        eImage2D,
        eMax
    };
}

namespace BlendState
{
    enum : uint32
    {
        eAlphaBlend = 0,
        eReplace,
        eNoColorAttachment,
        eMax
    };
}

namespace DepthState
{
     enum : uint32
     {
         eOff_CCW = 0,
         eOff_NoCull,
         eTestOnWriteOn_CCW,
         eTestOnWriteOff_CCW,
         eMax
     };   
}

namespace ImageFormat
{
    enum : uint32
    {
        Invalid = 0,
        BGRA8_SRGB,
        RGBA8_SRGB,
        RGBA16_Float,
        Depth_32F,
        TheSwapChainFormat,
        eMax
    };
}

namespace ImageLayout
{
    enum : uint32
    {
        eUndefined = 0,
        eShaderRead,
        eTransferDst,
        eRenderOptimal,
        eDepthOptimal,
        eGeneral,
        ePresent,
        eMax
    };
}

namespace DepthCompareOp
{
    enum : uint32
    {
        eLeOrEqual = 0,
        eGeOrEqual,
        // TODO: support strictly less than for normal rendering?
        eMax
    };
}

namespace BindPoint
{
    enum : uint32
    {
        eGraphics = 0,
        eCompute,
        eMax
    };
}

extern uint32 MultiBufferedStatusFromBufferUsage[BufferUsage::eMax];
inline uint32 IsBufferUsageMultiBuffered(uint32 bufferUsage)
{
    TINKER_ASSERT(bufferUsage < BufferUsage::eMax);
    return MultiBufferedStatusFromBufferUsage[bufferUsage];
}

// IDs must be uniquely named and have their id ascend monotonically from 0
enum
{
    DESCLAYOUT_ID_CB_GLOBAL = 0,
    //DESCLAYOUT_ID_CB_PER_VIEW,
    //DESCLAYOUT_ID_CB_PER_MATERIAL,
    DESCLAYOUT_ID_CB_PER_INSTANCE,

    DESCLAYOUT_ID_BINDLESS_SAMPLED_TEXTURES,
    //DESCLAYOUT_ID_TEXTURES_UINT,

    // TODO: All of these should be able to get deleted after going bindless 
    DESCLAYOUT_ID_QUAD_BLIT_TEX,
    DESCLAYOUT_ID_QUAD_BLIT_VBS,
    DESCLAYOUT_ID_ASSET_INSTANCE,
    DESCLAYOUT_ID_ASSET_VBS,
    DESCLAYOUT_ID_POSONLY_VBS,
    DESCLAYOUT_ID_IMGUI_VBS,
    DESCLAYOUT_ID_IMGUI_TEX,
    DESCLAYOUT_ID_COMPUTE_COPY,
    DESCLAYOUT_ID_MAX
};

enum
{
    SHADER_ID_QUAD_BLIT_TONEMAP = 0,
    SHADER_ID_QUAD_BLIT_RGBA8,
    SHADER_ID_QUAD_BLIT_CLEAR,
    SHADER_ID_IMGUI_DEBUGUI,
    SHADER_ID_IMGUI_DEBUGUI_RGBA8,
    SHADER_ID_BASIC_ZPrepass,
    SHADER_ID_BASIC_MainView,
    SHADER_ID_ANIMATEDPOLY_MainView,
    SHADER_ID_MAX
};

enum
{
    SHADER_ID_COMPUTE_COPY = 0,
    SHADER_ID_COMPUTE_MAX
};
//-----

#define MAX_DESCRIPTOR_SETS_PER_SHADER 4
#define MAX_BINDINGS_PER_SET 3
#define DESCRIPTOR_BINDLESS_ARRAY_LIMIT 1024

// Important graphics defines
#define MAX_FRAMES_IN_FLIGHT 2
#define DESIRED_NUM_SWAP_CHAIN_IMAGES 2
#define MAX_MULTIPLE_RENDERTARGETS 8u

//#define DEPTH_REVERSED
#ifndef DEPTH_REVERSED
#define DEPTH_MIN 0.0f
#define DEPTH_MAX 1.0f
#define DEPTH_OP DepthCompareOp::eLeOrEqual
#else
// TODO: untested
#define DEPTH_MIN 1.0f
#define DEPTH_MAX 0.0f
#define DEPTH_OP DepthCompareOp::eGeOrEqual
#endif

#define GPU_TIMESTAMP_NUM_MAX 1024

#define MIN_PUSH_CONSTANTS_SIZE 128 // bytes

#define THREADGROUP_ROUND(x, tg) ((x + tg - 1) / tg)

// Concrete type for resource handle to catch errors at compile time, e.g.
// Try to free a descriptor set with a resource handle, which can happen if all handles
// are just plain uint32.
struct ResourceHandle
{
    uint32 m_hRes;

    ResourceHandle()
    {
        m_hRes = TINKER_INVALID_HANDLE;
    }

    // Warning: probably don't pass around handles as uint32 willy-nilly
    explicit ResourceHandle(uint32 h)
    {
        m_hRes = h;
    }

    inline bool operator==(const ResourceHandle& other) const
    {
        return m_hRes == other.m_hRes;
    }

    inline bool operator!=(const ResourceHandle& other) const
    {
        return m_hRes != other.m_hRes;
    }
};

typedef struct graphics_resource_description
{
    v3ui dims = {};
    uint32 resourceType = ResourceType::eMax;

    union
    {
        // Buffers
        uint32 bufferUsage;

        // Images
        struct
        {
            uint32 imageFormat;
            uint32 arrayEles;
            uint32 imageUsageFlags;
        };
    };

    const char* debugLabel = "";
} ResourceDesc;

struct FramebufferHandle
{
    uint32 m_hFramebuffer;

    FramebufferHandle()
    {
        m_hFramebuffer = TINKER_INVALID_HANDLE;
    }

    // Warning: probably don't pass around handles as uint32 willy-nilly
    explicit FramebufferHandle(uint32 h)
    {
        m_hFramebuffer = h;
    }

    inline bool operator==(const FramebufferHandle& other) const
    {
        return m_hFramebuffer == other.m_hFramebuffer;
    }

    inline bool operator!=(const FramebufferHandle& other) const
    {
        return m_hFramebuffer != other.m_hFramebuffer;
    }
};

struct DescriptorHandle
{
    uint32 m_hDesc;
    uint32 m_layoutID;

    DescriptorHandle()
    {
        m_hDesc = TINKER_INVALID_HANDLE;
        m_layoutID = DESCLAYOUT_ID_MAX;
    }

    // Warning: probably don't pass around handles as uint32 willy-nilly
    explicit DescriptorHandle(uint32 h, uint32 lID)
    {
        m_hDesc = h;
        m_layoutID = lID;
    }

    inline bool operator==(const DescriptorHandle& other) const
    {
        return m_hDesc == other.m_hDesc &&
            m_layoutID == other.m_layoutID;
    }

    inline bool operator!=(const DescriptorHandle& other) const
    {
        return !(*this == other);
    }
};

typedef struct descriptor_layout_params
{
    uint32 type;
    uint32 amount;
} DescriptorLayoutParams;

typedef struct descriptor_layout
{
    DescriptorLayoutParams params[MAX_BINDINGS_PER_SET];

    void InitInvalid()
    {
        for (uint32 uiDesc = 0; uiDesc < MAX_BINDINGS_PER_SET; ++uiDesc)
        {
            params[uiDesc].type = DescriptorType::eMax;
            params[uiDesc].amount = 0;
        }
    }
} DescriptorLayout;

#define DefaultResHandle_Invalid ResourceHandle()
#define DefaultFramebufferHandle_Invalid FramebufferHandle()
#define DefaultDescHandle_Invalid DescriptorHandle()

// List of resource handles in a descriptor set, convenience struct 
typedef struct descriptor_set_data_handles
{
    ResourceHandle handles[MAX_BINDINGS_PER_SET];

    void InitInvalid()
    {
        for (uint32 uiDesc = 0; uiDesc < MAX_BINDINGS_PER_SET; ++uiDesc)
        {
            handles[uiDesc] = DefaultResHandle_Invalid;
        }
    }
} DescriptorSetDataHandles;

// Command buffer
typedef struct command_buffer
{
    uint64 apiObjectHandles[MAX_FRAMES_IN_FLIGHT];

    bool operator==(const struct command_buffer& other) const
    {
        for (uint32 i = 0; i < ARRAYCOUNT(apiObjectHandles); ++i)
        {
            if (apiObjectHandles[i] != other.apiObjectHandles[i])
            {
                return false;
            }
        }
        return true;
    }
} CommandBuffer;
CommandBuffer CreateCommandBuffer();
void BeginCommandRecording(CommandBuffer commandBuffer);
void EndCommandRecording(CommandBuffer commandBuffer);

typedef struct mem_mapped_buf_ptr
{
    void* m_ptr = nullptr;
    uint32 m_offset = 0;

    void* GetCurrPtr() 
    {
        return (uint8*)m_ptr + m_offset;
    }

    void MemcpyInto(const void* srcData, uint32 numBytes)
    {
        void* currPtr = GetCurrPtr();
        memcpy(currPtr, srcData, numBytes);
        m_offset += numBytes;
    }

} MemoryMappedBufferPtr;

typedef struct graphics_command
{
    enum : uint32
    {
        eDrawCall = 0,
        eDispatch,
        ePushConstant,
        eSetScissor,
        eSetViewport,
        eCmdBufferBegin,
        eCmdBufferEnd,
        eRenderPassBegin,
        eRenderPassEnd,
        eLayoutTransition,
        eClearImage,
        eCopy,
        eGPUTimestamp,
        eDebugMarkerStart,
        eDebugMarkerEnd,
        eMax
    };

    const char* debugLabel = "Default Label";
    uint32 m_commandType;

    union
    {   
        // Draw call
        struct
        {
            uint32 m_numIndices;
            uint32 m_numInstances;
            uint32 m_vertexOffset;
            uint32 m_indexOffset;
            uint32 m_shader;
            uint32 m_blendState;
            uint32 m_depthState;
            ResourceHandle m_indexBufferHandle;
            DescriptorHandle m_descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        };

        // Dispatch
        struct
        {
            uint32 m_threadGroupsX;
            uint32 m_threadGroupsY;
            uint32 m_threadGroupsZ;
            uint32 m_shader;
            DescriptorHandle m_descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        };

        // Push constant
        // TODO: more data here makes this struct bigger, maybe store a pointer?
        enum { ePushConstantLimitInBytes = 32u };
        struct
        {
            uint32 m_shaderForLayout;
            uint8 m_pushConstantData[ePushConstantLimitInBytes];
        };

        // Scissor
        struct
        {
            int32 m_scissorOffsetX;
            int32 m_scissorOffsetY;
            uint32 m_scissorWidth;
            uint32 m_scissorHeight;
        };

        // Viewport
        struct
        {
            float m_viewportOffsetX;
            float m_viewportOffsetY;
            float m_viewportWidth;
            float m_viewportHeight;
            float m_viewportMinDepth;
            float m_viewportMaxDepth;
        };

        // Begin/end command buffer
        struct
        {
            CommandBuffer m_commandBuffer;
        };

        // Begin render pass
        struct
        {
            uint32 m_renderWidth;
            uint32 m_renderHeight;
            uint32 m_numColorRTs;
            ResourceHandle m_colorRTs[MAX_MULTIPLE_RENDERTARGETS];
            ResourceHandle m_depthRT;
        };

        // End render pass
        // NOTE: no actual data required

        // Image Layout Transition
        struct
        {
            ResourceHandle m_imageHandle;
            uint32 m_startLayout;
            uint32 m_endLayout;
        };

        // Clear image
        struct
        {
            ResourceHandle m_imageHandle;
            v4f m_clearValue;
        };

        // Copy
        struct
        {
            uint32 m_sizeInBytes;
            ResourceHandle m_srcBufferHandle;
            ResourceHandle m_dstBufferHandle;
        };

        // GPU Timestamp
        struct
        {
            const char* m_timestampNameStr;
            bool m_timestampStartFrame;
        };
        
        // Debug marker start
        // Debug marker end
        // NOTE: no actual data required
    };

} GraphicsCommand;

struct GraphicsCommandStream
{
    GraphicsCommand* m_graphicsCommands;
    uint32 m_numCommands;
    uint32 m_maxCommands;

    void Clear()
    {
        m_numCommands = 0;
        //memset(m_graphicsCommands, 0, sizeof(GraphicsCommand) * m_maxCommands); // shouldnt' be needed for correctness but could be helpful for debug
    }

    GraphicsCommand* GetNextCommand()
    {
        TINKER_ASSERT(m_numCommands < m_maxCommands);
        return &m_graphicsCommands[m_numCommands++];
    }

    void CmdDraw(uint32 numIndices, uint32 numInstances, uint32 vertexOffset, uint32 indexOffset, uint32 shaderID, uint32 blendState, uint32 depthState,
        ResourceHandle indexBufferHandle, uint32 numDescriptors, DescriptorHandle* descriptors, const char* dbgLabel = "Draw")
    {
        TINKER_ASSERT(numDescriptors <= MAX_DESCRIPTOR_SETS_PER_SHADER);

        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = Graphics::GraphicsCommand::eDrawCall;
        command->debugLabel = dbgLabel;
        command->m_numIndices = numIndices;
        command->m_numInstances = numInstances;
        command->m_vertexOffset = vertexOffset;
        command->m_indexOffset = indexOffset;
        command->m_shader = shaderID;
        command->m_blendState = blendState;
        command->m_depthState = depthState;
        command->m_indexBufferHandle = indexBufferHandle;
        memcpy(command->m_descriptors, descriptors, sizeof(DescriptorHandle) * numDescriptors);
    }
    
    void CmdDispatch(uint32 threadGroupsX, uint32 threadGroupsY, uint32 threadGroupsZ, uint32 shaderID, uint32 numDescriptors, DescriptorHandle* descriptors, const char* dbgLabel = "Dispatch")
    {
        TINKER_ASSERT(numDescriptors <= MAX_DESCRIPTOR_SETS_PER_SHADER);

        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eDispatch;
        command->debugLabel = dbgLabel;
        command->m_threadGroupsX = threadGroupsX;
        command->m_threadGroupsY = threadGroupsY;
        command->m_threadGroupsZ = threadGroupsZ;
        command->m_shader = shaderID;
        memcpy(command->m_descriptors, descriptors, sizeof(DescriptorHandle) * numDescriptors);
    }

    void CmdPushConstant(uint32 shaderForLayout, const uint8* srcPushConstantData, uint32 numPushConstantBytes, const char* dbgLabel = "PushConstant")
    {
        TINKER_ASSERT(numPushConstantBytes <= GraphicsCommand::ePushConstantLimitInBytes);

        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::ePushConstant;
        command->debugLabel = dbgLabel;
        command->m_shaderForLayout = shaderForLayout;
        memcpy(command->m_pushConstantData, srcPushConstantData, numPushConstantBytes);
    }

    void CmdSetScissor(int32 scissorOffsetX, int32 scissorOffsetY, uint32 scissorWidth, uint32 scissorHeight, const char* dbgLabel = "SetScissor")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eSetScissor;
        command->debugLabel = dbgLabel;
        command->m_scissorOffsetX = scissorOffsetX;
        command->m_scissorOffsetY = scissorOffsetY;
        command->m_scissorWidth = scissorWidth;
        command->m_scissorHeight = scissorHeight;
    }

    void CmdSetViewport(float viewportOffsetX, float viewportOffsetY, float viewportWidth, float viewportHeight, float viewportMinDepth, float viewportMaxDepth, const char* dbgLabel = "SetViewport")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eSetViewport;
        command->debugLabel = dbgLabel;
        command->m_viewportOffsetX = viewportOffsetX;
        command->m_viewportOffsetY = viewportOffsetY;
        command->m_viewportWidth = viewportWidth;
        command->m_viewportHeight = viewportHeight;
        command->m_viewportMinDepth = viewportMinDepth;
        command->m_viewportMaxDepth = viewportMaxDepth;
    }

    void CmdCommandBufferBegin(CommandBuffer cmdBuf, const char* dbgLabel = "CmdBufferBegin")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eCmdBufferBegin;
        command->debugLabel = dbgLabel;
        command->m_commandBuffer = cmdBuf;
    }

    void CmdCommandBufferEnd(CommandBuffer cmdBuf, const char* dbgLabel = "CmdBufferEnd")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eCmdBufferEnd;
        command->debugLabel = dbgLabel;
        command->m_commandBuffer = cmdBuf;
    }

    void CmdRenderPassBegin(uint32 renderWidth, uint32 renderHeight, uint32 numColorRTs, ResourceHandle* colorRTs, ResourceHandle depthRT, const char* dbgLabel = "RenderPassBegin")
    {
        TINKER_ASSERT(numColorRTs <= MAX_MULTIPLE_RENDERTARGETS);

        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = Tk::Graphics::GraphicsCommand::eRenderPassBegin;
        command->debugLabel = dbgLabel;
        command->m_numColorRTs = numColorRTs;
        memcpy(command->m_colorRTs, colorRTs, numColorRTs * sizeof(ResourceHandle));
        command->m_depthRT = depthRT;
        command->m_renderWidth = renderWidth;
        command->m_renderHeight = renderHeight;
    }

    void CmdRenderPassEnd(const char* dbgLabel = "RenderPassEnd")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = Tk::Graphics::GraphicsCommand::eRenderPassEnd;
        command->debugLabel = dbgLabel;
    }

    void CmdLayoutTransition(ResourceHandle imageHandle, uint32 startLayout, uint32 endLayout, const char* dbgLabel = "Transition")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eLayoutTransition;
        command->debugLabel = dbgLabel;
        command->m_imageHandle = imageHandle;
        command->m_startLayout = startLayout;
        command->m_endLayout = endLayout;
    }

    void CmdClear(ResourceHandle imageHandle, const v4f& clearValue, const char* dbgLabel = "Clear")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eClearImage;
        command->debugLabel = dbgLabel;
        command->m_imageHandle = imageHandle;
        command->m_clearValue = clearValue;
    }

    void CmdCopy(ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle, uint32 sizeInBytes, const char* dbgLabel = "Copy")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eCopy;
        command->debugLabel = dbgLabel;
        command->m_srcBufferHandle = srcBufferHandle;
        command->m_dstBufferHandle = dstBufferHandle;
        command->m_sizeInBytes = sizeInBytes;
    }

    void CmdTimestamp(const char* nameStr, const char* dbgLabel = "Timestamp", bool startFrame = false)
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eGPUTimestamp;
        command->debugLabel = dbgLabel;
        command->m_timestampNameStr = nameStr;
        command->m_timestampStartFrame = startFrame;
    }

    void CmdDebugMarkerStart(const char* dbgLabel = "DebugMarkerStart")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eDebugMarkerStart;
        command->debugLabel = dbgLabel;
    }

    void CmdDebugMarkerEnd(const char* dbgLabel = "DebugMarkerEnd")
    {
        GraphicsCommand* command = GetNextCommand();
        command->m_commandType = GraphicsCommand::eDebugMarkerEnd;
        command->debugLabel = dbgLabel;
    }
};

// Default/fallback texture resources
typedef struct default_tex
{
    ResourceHandle res;
    v4f clearValue;
} DefaultTexture;

namespace DefaultTextureID
{
    enum : uint32
    {
        eBlack2x2,
        eMax
    };
}

namespace DescUpdateConfigFlags
{
    enum : uint32
    {
        Transient = 0x1,
    };
}

// Graphics API layer
#define CREATE_RESOURCE(name) ResourceHandle name(const ResourceDesc& resDesc)
CREATE_RESOURCE(CreateResource);

#define DESTROY_RESOURCE(name) void name(ResourceHandle handle)
DESTROY_RESOURCE(DestroyResource);

#define MAP_RESOURCE(name) MemoryMappedBufferPtr name(ResourceHandle handle)
MAP_RESOURCE(MapResource);

#define UNMAP_RESOURCE(name) void name(ResourceHandle handle)
UNMAP_RESOURCE(UnmapResource);

#define CREATE_DESCRIPTOR(name) DescriptorHandle name(uint32 descLayoutID)
CREATE_DESCRIPTOR(CreateDescriptor);

#define DESTROY_DESCRIPTOR(name) void name(DescriptorHandle handle)
DESTROY_DESCRIPTOR(DestroyDescriptor);

#define DESTROY_ALL_DESCRIPTORS(name) void name()
DESTROY_ALL_DESCRIPTORS(DestroyAllDescriptors);

#define WRITE_DESCRIPTOR_SIMPLE(name) void name(DescriptorHandle descSetHandle, const DescriptorSetDataHandles* descSetDataHandles)
WRITE_DESCRIPTOR_SIMPLE(WriteDescriptorSimple);

#define WRITE_DESCRIPTOR_ARRAY(name) void name(DescriptorHandle descSetHandle, uint32 numEntries, ResourceHandle* entries, uint32 updateFlags)
WRITE_DESCRIPTOR_ARRAY(WriteDescriptorArray);

#define SUBMIT_CMDS_IMMEDIATE(name) void name(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream, Tk::Graphics::CommandBuffer commandBuffer)
SUBMIT_CMDS_IMMEDIATE(SubmitCmdsImmediate);

// Graphics command recording
void RecordCommandPushConstant(CommandBuffer commandBuffer, const uint8* data, uint32 sizeInBytes, uint32 shaderID);
void RecordCommandSetViewport(CommandBuffer commandBuffer, float x, float y, float width, float height, float minDepth, float maxDepth);
void RecordCommandSetScissor(CommandBuffer commandBuffer, int32 offsetX, int32 offsetY, uint32 width, uint32 height);
void RecordCommandDrawCall(CommandBuffer commandBuffer, ResourceHandle indexBufferHandle, uint32 numIndices, uint32 numInstances,
    uint32 vertOffset, uint32 indexOffset, const char* debugLabel);
void RecordCommandDispatch(CommandBuffer commandBuffer, uint32 threadGroupX, uint32 threadGroupY, uint32 threadGroupZ, const char* debugLabel);
void RecordCommandBindShader(CommandBuffer commandBuffer, uint32 shaderID, uint32 blendState, uint32 depthState);
void RecordCommandBindComputeShader(CommandBuffer commandBuffer, uint32 shaderID);
void RecordCommandBindDescriptor(CommandBuffer commandBuffer, uint32 shaderID, uint32 bindPoint, const DescriptorHandle descSetHandle, uint32 descSetIndex);
void RecordCommandMemoryTransfer(CommandBuffer commandBuffer, uint32 sizeInBytes, ResourceHandle srcBufferHandle, ResourceHandle dstBufferHandle,
    const char* debugLabel);
void RecordCommandRenderPassBegin(CommandBuffer commandBuffer, uint32 numColorRTs, const ResourceHandle* colorRTs, ResourceHandle depthRT,
    uint32 renderWidth, uint32 renderHeight, const char* debugLabel);
void RecordCommandRenderPassEnd(CommandBuffer commandBuffer);
void RecordCommandTransitionLayout(CommandBuffer commandBuffer, ResourceHandle imageHandle, uint32 startLayout, uint32 endLayout,
    const char* debugLabel);
void RecordCommandClearImage(CommandBuffer commandBuffer, ResourceHandle imageHandle,
    const v4f& clearValue, const char* debugLabel);
void RecordCommandGPUTimestamp(CommandBuffer commandBuffer, uint32 gpuTimestampID);
void RecordCommandDebugMarkerStart(CommandBuffer commandBuffer, const char* debugLabel);
void RecordCommandDebugMarkerEnd(CommandBuffer commandBuffer);
//

// Called only by ShaderManager
#define CREATE_DESCRIPTOR_LAYOUT(name) bool name(uint32 descLayoutID, const DescriptorLayout* descLayout)
CREATE_DESCRIPTOR_LAYOUT(CreateDescriptorLayout);

#define CREATE_GRAPHICS_PIPELINE(name) bool name(void* vertexShaderCode, uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 shaderID, uint32 numColorRTs, const uint32* colorRTFormats, uint32 depthFormat, uint32* descriptorLayoutHandles, uint32 numDescriptorLayoutHandles)
CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline);

#define DESTROY_GRAPHICS_PIPELINE(name) void name(uint32 shaderID)
DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline);

#define CREATE_COMPUTE_PIPELINE(name) bool name(void* computeShaderCode, uint32 numComputeShaderBytes, uint32 shaderID, uint32* descriptorLayoutHandles, uint32 numDescriptorLayoutHandles)
CREATE_COMPUTE_PIPELINE(CreateComputePipeline);

#define DESTROY_COMPUTE_PIPELINE(name) void name(uint32 shaderID)
DESTROY_COMPUTE_PIPELINE(DestroyComputePipeline);
//

void CreateContext(const Tk::Platform::WindowHandles* windowHandles);
void DestroyContext();
void RecreateContext(const Tk::Platform::WindowHandles* windowHandles);

typedef struct swap_chain_data
{
    uint32 numSwapChainImages = 0;
    uint32 currentSwapChainImage = TINKER_INVALID_HANDLE;
    uint32 windowWidth = 0;
    uint32 windowHeight = 0;
    bool isSwapChainValid = false;
    uint32 swapChainAPIObjectsHandle = TINKER_INVALID_HANDLE;
    ResourceHandle swapChainResourceHandles[DESIRED_NUM_SWAP_CHAIN_IMAGES];
} SwapChainData;
#define NUM_SWAP_CHAINS_STARTING_ALLOC_SIZE 16
void CreateSwapChain(const Tk::Platform::WindowHandles* windowHandles, uint32 width, uint32 height);
void DestroySwapChain(const Tk::Platform::WindowHandles* windowHandles);
void CreateSwapChainAPIObjects(SwapChainData* swapChainData, const Tk::Platform::WindowHandles* windowHandles);
void DestroySwapChainAPIObjects(SwapChainData* swapChainData);
void WindowResize(const Tk::Platform::WindowHandles* windowHandles, uint32 newWindowWidth, uint32 newWindowHeight);
void WindowMinimized(const Tk::Platform::WindowHandles* windowHandles);
ResourceHandle GetCurrentSwapChainImage(const Tk::Platform::WindowHandles* windowHandles);

bool AcquireFrame(const Tk::Platform::WindowHandles* windowHandles);
void ProcessGraphicsCommandStream(const Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);
void SubmitFrameToGPU(const Tk::Platform::WindowHandles* windowHandles, CommandBuffer commandBuffer);
void PresentToSwapChain(const Tk::Platform::WindowHandles* windowHandles);
void EndFrame();

void DestroyAllPSOPerms();

float GetGPUTimestampPeriod();
uint32 GetCurrentFrameInFlightIndex();
void ResolveMostRecentAvailableTimestamps(CommandBuffer commandBuffer, void* gpuTimestampCPUSideBuffer, uint32 numTimestampsInQuery);

void CreateAllDefaultTextures(Tk::Graphics::GraphicsCommandStream* graphicsCommandStream);
void DestroyDefaultTextures();
DefaultTexture GetDefaultTexture(uint32 defaultTexID);

}
}
