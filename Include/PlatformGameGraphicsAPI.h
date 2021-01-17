#include "Core/Math/VectorTypes.h"

namespace Tinker
{
    namespace Platform
    {
        // Enums
        enum DescriptorType : uint32
        {
            eDescriptorTypeBuffer = 0,
            eDescriptorTypeSampledImage,
            eDescriptorTypeMax
        };

        enum BufferUsage : uint32
        {
            eBufferUsageVertex = 0,
            eBufferUsageIndex,
            eBufferUsageStaging,
            eBufferUsageUniform
        };

        enum ResourceType : uint32
        {
            eResourceTypeBuffer1D = 0,
            eResourceTypeImage2D,
            eResourceTypeMax
        };

        enum BlendState : uint32
        {
            eBlendStateAlphaBlend = 0,
            eBlendStateMax
        };

        enum DepthState : uint32
        {
            eDepthStateOff = 0,
            eDepthStateTestOnWriteOn,
            eDepthStateMax
        };

        enum ImageFormat : uint32
        {
            eImageFormat_BGRA8_Unorm = 0,
            eImageFormat_RGBA8_Unorm,
            eImageFormat_Depth_32F,
            eImageFormat_Invalid,
            eImageFormatMax
        };

        enum ImageLayout : uint32
        {
            eImageLayoutUndefined = 0,
            eImageLayoutShaderRead,
            eImageLayoutColorAttachment,
            eImageLayoutSwapChainPresent,
            eImageLayoutMax
        };

        enum GraphicsCmdType : uint32
        {
            eGraphicsCmdDrawCall = 0,
            eGraphicsCmdMemTransfer,
            eGraphicsCmdRenderPassBegin,
            eGraphicsCmdRenderPassEnd,
            eGraphicsCmdImageCopy,
            eGraphicsCmdMax
        };

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
            Core::Math::v3ui dims;
            uint32 resourceType;

            union
            {
                // Buffers
                uint32 bufferUsage;

                // Images
                struct
                {
                    uint32 imageFormat;
                };
            };
        } ResourceDesc;

        struct ShaderHandle
        {
            uint32 m_hShader;

            ShaderHandle()
            {
                m_hShader = TINKER_INVALID_HANDLE;
            }

            // Warning: probably don't pass around handles as uint32 willy-nilly
            explicit ShaderHandle(uint32 h)
            {
                m_hShader = h;
            }

            inline bool operator==(const ShaderHandle& other) const
            {
                return m_hShader == other.m_hShader;
            }

            inline bool operator!=(const ShaderHandle& other) const
            {
                return m_hShader != other.m_hShader;
            }
        };
        #define DefaultShaderHandle_Invalid ShaderHandle()

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

        #define MAX_DESCRIPTOR_SETS_PER_SHADER 1
        #define MAX_DESCRIPTORS_PER_SET 1

        typedef struct descriptor_layout_params
        {
            uint32 type;
            uint32 amount;
        } DescriptorLayoutParams;

        typedef struct descriptor_layout
        {
            DescriptorLayoutParams descriptorLayoutParams[MAX_DESCRIPTOR_SETS_PER_SHADER][MAX_DESCRIPTORS_PER_SET];
        } DescriptorLayout;

        // list of resource handles in a descriptor set
        typedef struct descriptor_set_data_handles
        {
            ResourceHandle handles[MAX_DESCRIPTORS_PER_SET];
        } DescriptorSetDataHandles;

        // list of descriptor handles to bind
        typedef struct descriptor_set_desc_handles
        {
            DescriptorHandle handles[MAX_DESCRIPTORS_PER_SET];
        } DescriptorSetDescHandles;

        inline void InitDescLayout(DescriptorLayout* layout)
        {
            for (uint32 uiDescSet = 0; uiDescSet < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDescSet)
            {
                for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTORS_PER_SET; ++uiDesc)
                {
                    layout->descriptorLayoutParams[uiDescSet][uiDesc].type = eDescriptorTypeMax;
                    layout->descriptorLayoutParams[uiDescSet][uiDesc].amount = 0;
                }
            }
        }

        inline void InitDescSetDescHandles(DescriptorSetDescHandles* descSetDescHandles)
        {
            for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTORS_PER_SET; ++uiDesc)
            {
                descSetDescHandles->handles[uiDesc] = DefaultDescHandle_Invalid;
            }
        }

        typedef struct graphics_command
        {
            uint32 m_commandType;
            const char* debugLabel = "Default Label";
            union
            {
                // Draw call
                struct
                {
                    uint32 m_numIndices;
                    ResourceHandle m_indexBufferHandle;
                    ResourceHandle m_positionBufferHandle;
                    ResourceHandle m_uvBufferHandle;
                    ResourceHandle m_normalBufferHandle;
                    ShaderHandle m_shaderHandle;
                    DescriptorSetDescHandles m_descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
                };

                // Memory transfer
                struct
                {
                    uint32 m_sizeInBytes;
                    ResourceHandle m_srcBufferHandle; // src
                    ResourceHandle m_dstBufferHandle; // dst
                };

                // Begin render pass
                struct
                {
                    FramebufferHandle m_framebufferHandle;
                    uint32 m_renderWidth;
                    uint32 m_renderHeight;
                };

                // End render pass
                // NOTE: For now, no data
                /*struct
                {
                };*/

                // Image copy
                struct
                {
                    uint32 m_width;
                    uint32 m_height;
                    ResourceHandle m_srcImgHandle;
                    ResourceHandle m_dstImgHandle;
                };

                // TODO: other commands
            };
        } GraphicsCommand;

        typedef struct graphics_command_stream
        {
            GraphicsCommand* m_graphicsCommands;
            uint32 m_numCommands;
            uint32 m_maxCommands;
        } GraphicsCommandStream;

        typedef struct graphics_pipeline_params
        {
            uint32 blendState;
            uint32 depthState;
            uint32 viewportWidth;
            uint32 viewportHeight;
            FramebufferHandle framebufferHandle;
            DescriptorHandle descriptorHandle;
        } GraphicsPipelineParams;

        #define CREATE_RESOURCE(name) ResourceHandle name(const ResourceDesc& resDesc)
        typedef CREATE_RESOURCE(create_resource);

        #define DESTROY_RESOURCE(name) void name(ResourceHandle handle)
        typedef DESTROY_RESOURCE(destroy_resource);
        
        #define MAP_RESOURCE(name) void* name(ResourceHandle handle)
        typedef MAP_RESOURCE(map_resource);

        #define UNMAP_RESOURCE(name) void name(ResourceHandle handle)
        typedef UNMAP_RESOURCE(unmap_resource);

        #define CREATE_FRAMEBUFFER(name) FramebufferHandle name(ResourceHandle* rtColorHandles, uint32 numRTColorHandles, ResourceHandle rtDepthHandle, uint32 colorEndLayout, uint32 width, uint32 height)
        typedef CREATE_FRAMEBUFFER(create_framebuffer);

        #define DESTROY_FRAMEBUFFER(name) void name(FramebufferHandle handle)
        typedef DESTROY_FRAMEBUFFER(destroy_framebuffer);

        #define CREATE_GRAPHICS_PIPELINE(name) ShaderHandle name(void* vertexShaderCode, uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 blendState, uint32 depthState, uint32 viewportWidth, uint32 viewportHeight, FramebufferHandle framebufferHandle, DescriptorHandle descriptorHandle)
        typedef CREATE_GRAPHICS_PIPELINE(create_graphics_pipeline);

        #define DESTROY_GRAPHICS_PIPELINE(name) void name(ShaderHandle handle)
        typedef DESTROY_GRAPHICS_PIPELINE(destroy_graphics_pipeline);
        
        #define CREATE_DESCRIPTOR(name) DescriptorHandle name(DescriptorLayout* descLayout)
        typedef CREATE_DESCRIPTOR(create_descriptor);

        #define DESTROY_DESCRIPTOR(name) void name(DescriptorHandle handle)
        typedef DESTROY_DESCRIPTOR(destroy_descriptor);

        #define DESTROY_ALL_DESCRIPTORS(name) void name()
        typedef DESTROY_ALL_DESCRIPTORS(destroy_all_descriptors);

        #define WRITE_DESCRIPTOR(name) void name(DescriptorLayout* descLayout, DescriptorHandle descSetHandle, DescriptorSetDataHandles* descSetDataHandles)
        typedef WRITE_DESCRIPTOR(write_descriptor);

        #define SUBMIT_CMDS_IMMEDIATE(name) void name(Tinker::Platform::GraphicsCommandStream* graphicsCommandStream)
        typedef SUBMIT_CMDS_IMMEDIATE(submit_cmds_immediate);
    }
}
