#pragma once

#include "Core/Math/VectorTypes.h"

namespace Tk
{
namespace Platform
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
        BGRA8_SRGB = 0,
        RGBA8_SRGB,
        Depth_32F,
        Invalid,
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
    v3ui dims;
    uint32 resourceType;

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
            FramebufferHandle m_framebufferHandle;
            uint32 m_renderPassID;
            uint32 m_renderWidth;
            uint32 m_renderHeight;
        };

        // End render pass
        // NOTE: For now, no data
        /*struct
        {
        };*/

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
    DESCLAYOUT_ID_VIRTUAL_TEXTURE,
    DESCLAYOUT_ID_TERRAIN_DATA,
    DESCLAYOUT_ID_MAX,
};

enum
{
    RENDERPASS_ID_SWAP_CHAIN_BLIT = 0,
    RENDERPASS_ID_ZPrepass,
    RENDERPASS_ID_MainView,
    RENDERPASS_ID_MAX
};

enum
{
    SHADER_ID_SWAP_CHAIN_BLIT = 0,
    SHADER_ID_BASIC_ZPrepass,
    SHADER_ID_BASIC_MainView,
    SHADER_ID_ANIMATEDPOLY_MainView,
    SHADER_ID_BASIC_VirtualTexture,
    SHADER_ID_MAX,
};
//-----

#define CREATE_RESOURCE(name) ResourceHandle name(const ResourceDesc& resDesc)
typedef CREATE_RESOURCE(create_resource);

#define DESTROY_RESOURCE(name) void name(ResourceHandle handle)
typedef DESTROY_RESOURCE(destroy_resource);

#define MAP_RESOURCE(name) void* name(ResourceHandle handle)
typedef MAP_RESOURCE(map_resource);

#define UNMAP_RESOURCE(name) void name(ResourceHandle handle)
typedef UNMAP_RESOURCE(unmap_resource);

#define CREATE_FRAMEBUFFER(name) FramebufferHandle name(ResourceHandle* rtColorHandles, uint32 numRTColorHandles, ResourceHandle rtDepthHandle, uint32 width, uint32 height, uint32 renderPassID)
typedef CREATE_FRAMEBUFFER(create_framebuffer);

#define DESTROY_FRAMEBUFFER(name) void name(FramebufferHandle handle)
typedef DESTROY_FRAMEBUFFER(destroy_framebuffer);

#define CREATE_GRAPHICS_PIPELINE(name) bool name(void* vertexShaderCode, uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID, uint32* descriptorHandles, uint32 numDescriptorHandles)
typedef CREATE_GRAPHICS_PIPELINE(create_graphics_pipeline);

#define CREATE_DESCRIPTOR(name) DescriptorHandle name(uint32 descLayoutID)
typedef CREATE_DESCRIPTOR(create_descriptor);

#define DESTROY_DESCRIPTOR(name) void name(DescriptorHandle handle)
typedef DESTROY_DESCRIPTOR(destroy_descriptor);

#define DESTROY_ALL_DESCRIPTORS(name) void name()
typedef DESTROY_ALL_DESCRIPTORS(destroy_all_descriptors);

#define WRITE_DESCRIPTOR(name) void name(uint32 descLayoutID, DescriptorHandle* descSetHandles, uint32 descSetCount, DescriptorSetDataHandles* descSetDataHandles, uint32 descSetDataCount)
typedef WRITE_DESCRIPTOR(write_descriptor);

#define SUBMIT_CMDS_IMMEDIATE(name) void name(Tk::Platform::GraphicsCommandStream* graphicsCommandStream)
typedef SUBMIT_CMDS_IMMEDIATE(submit_cmds_immediate);

// Not meant for the user
#define CREATE_DESCRIPTOR_LAYOUT(name) bool name(uint32 descLayoutID, const Platform::DescriptorLayout* descLayout)
typedef CREATE_DESCRIPTOR_LAYOUT(create_descriptor_layout);

#define CREATE_RENDERPASS(name) bool name(uint32 renderPassID, uint32 numColorRTs, uint32 colorFormat, uint32 startLayout, uint32 endLayout, uint32 depthFormat)
typedef CREATE_RENDERPASS(create_renderpass);

#define DESTROY_GRAPHICS_PIPELINE(name) void name(uint32 shaderID)
typedef DESTROY_GRAPHICS_PIPELINE(destroy_graphics_pipeline);

}
}
