#pragma once

#include "Math/VectorTypes.h"

namespace Tk
{

namespace Platform
{
struct PlatformWindowHandles;
}

namespace Core
{
struct GraphicsCommandStream;

namespace Graphics
{

// Enums
namespace DescriptorType
{
    enum : uint32
    {
        eBuffer = 0,
        eSampledImage,
        eSSBO,
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
         eOff = 0,
         eTestOnWriteOn,
         eTestOnWriteOff,
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
        ePresent,
        eMax
    };
}

namespace GraphicsCmd
{
    enum : uint32
    {
        eDrawCall = 0,
        eMemTransfer,
        eRenderPassBegin,
        eRenderPassEnd,
        eLayoutTransition,
        eClearImage,
        //eImageCopy,
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

extern uint32 MultiBufferedStatusFromBufferUsage[BufferUsage::eMax];
inline uint32 IsBufferUsageMultiBuffered(uint32 bufferUsage)
{
    TINKER_ASSERT(bufferUsage < BufferUsage::eMax);
    return MultiBufferedStatusFromBufferUsage[bufferUsage];
}

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
#define DefaultResHandle_Invalid ResourceHandle()

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
#define DefaultFramebufferHandle_Invalid FramebufferHandle()

struct DescriptorHandle
{
    uint32 m_hDesc;

    DescriptorHandle()
    {
        m_hDesc = TINKER_INVALID_HANDLE;
    }

    // Warning: probably don't pass around handles as uint32 willy-nilly
    explicit DescriptorHandle(uint32 h)
    {
        m_hDesc = h;
    }

    inline bool operator==(const DescriptorHandle& other) const
    {
        return m_hDesc == other.m_hDesc;
    }

    inline bool operator!=(const DescriptorHandle& other) const
    {
        return m_hDesc != other.m_hDesc;
    }
};
#define DefaultDescHandle_Invalid DescriptorHandle()

#define MAX_DESCRIPTOR_SETS_PER_SHADER 4
#define MAX_BINDINGS_PER_SET 3

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

// list of resource handles in a descriptor set
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

#define MAX_MULTIPLE_RENDERTARGETS 8u

#define IMAGE_HANDLE_SWAP_CHAIN ResourceHandle(0xFFFFFFFE) // INVALID_HANDLE - 1 reserved to refer to the swap chain image 

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

typedef struct graphics_command
{
    const char* debugLabel = "Default Label";
    uint32 m_commandType;

    union
    {
        // Draw call
        struct
        {
            uint32 m_numIndices;
            uint32 m_numInstances;
            ResourceHandle m_indexBufferHandle;
            uint32 m_shader;
            uint32 m_blendState;
            uint32 m_depthState;
            DescriptorHandle m_descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        };

        // Memory transfer
        struct
        {
            uint32 m_sizeInBytes;
            ResourceHandle m_srcBufferHandle;
            ResourceHandle m_dstBufferHandle;
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
        /* struct
        {
        }; */

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

        // Image copy
        /*struct
        {
            uint32 m_width;
            uint32 m_height;
            ResourceHandle m_srcImgHandle;
            ResourceHandle m_dstImgHandle;
        };*/

        // TODO: other commands
    };
} GraphicsCommand;

struct GraphicsCommandStream
{
    GraphicsCommand* m_graphicsCommands;
    uint32 m_numCommands;
    uint32 m_maxCommands;
};


// TODO: move all this, and also autogenerate this eventually?
// IDs must be uniquely named and have their id ascend monotonically from 0
enum
{
    DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX = 0,
    DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS,
    DESCLAYOUT_ID_VIEW_GLOBAL,
    DESCLAYOUT_ID_ASSET_INSTANCE,
    DESCLAYOUT_ID_ASSET_VBS,
    DESCLAYOUT_ID_POSONLY_VBS,
    DESCLAYOUT_ID_MAX,
};

enum
{
    SHADER_ID_SWAP_CHAIN_BLIT = 0,
    SHADER_ID_BASIC_ZPrepass,
    SHADER_ID_BASIC_MainView,
    SHADER_ID_ANIMATEDPOLY_MainView,
    SHADER_ID_MAX,
};
//-----

#define CREATE_RESOURCE(name) TINKER_API ResourceHandle name(const ResourceDesc& resDesc)
CREATE_RESOURCE(CreateResource);

#define DESTROY_RESOURCE(name) TINKER_API void name(ResourceHandle handle)
DESTROY_RESOURCE(DestroyResource);

#define MAP_RESOURCE(name) TINKER_API void* name(ResourceHandle handle)
MAP_RESOURCE(MapResource);

#define UNMAP_RESOURCE(name) TINKER_API void name(ResourceHandle handle)
UNMAP_RESOURCE(UnmapResource);

#define CREATE_GRAPHICS_PIPELINE(name) TINKER_API bool name(void* vertexShaderCode, uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 numColorRTs, const uint32* colorRTFormats, uint32 depthFormat, uint32* descriptorHandles, uint32 numDescriptorHandles)
CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline);

#define CREATE_DESCRIPTOR(name) TINKER_API DescriptorHandle name(uint32 descLayoutID)
CREATE_DESCRIPTOR(CreateDescriptor);

#define DESTROY_DESCRIPTOR(name) TINKER_API void name(DescriptorHandle handle)
DESTROY_DESCRIPTOR(DestroyDescriptor);

#define DESTROY_ALL_DESCRIPTORS(name) TINKER_API void name()
DESTROY_ALL_DESCRIPTORS(DestroyAllDescriptors);

#define WRITE_DESCRIPTOR(name) TINKER_API void name(uint32 descLayoutID, DescriptorHandle descSetHandle, const DescriptorSetDataHandles* descSetDataHandles, uint32 descSetDataCount)
WRITE_DESCRIPTOR(WriteDescriptor);

#define SUBMIT_CMDS_IMMEDIATE(name) TINKER_API void name(Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream)
SUBMIT_CMDS_IMMEDIATE(SubmitCmdsImmediate);

// Not meant for the user
#define CREATE_DESCRIPTOR_LAYOUT(name) TINKER_API bool name(uint32 descLayoutID, const DescriptorLayout* descLayout)
CREATE_DESCRIPTOR_LAYOUT(CreateDescriptorLayout);

#define DESTROY_GRAPHICS_PIPELINE(name) TINKER_API void name(uint32 shaderID)
DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline);

void CreateContext(const Tk::Platform::PlatformWindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight);
void RecreateContext(const Tk::Platform::PlatformWindowHandles* windowHandles, uint32 windowWidth, uint32 windowHeight);
void WindowResize();
void WindowMinimized();
void DestroyContext();
void DestroyAllPSOPerms();

bool AcquireFrame();
void ProcessGraphicsCommandStream(const Tk::Core::Graphics::GraphicsCommandStream* graphicsCommandStream, bool immediateSubmit);
void BeginFrameRecording();
void EndFrameRecording();
void SubmitFrameToGPU();

}
}
}
