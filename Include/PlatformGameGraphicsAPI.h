#include "Core/Math/VectorTypes.h"

namespace Tinker
{
    namespace Platform
    {
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

        enum
        {
            eDescriptorTypeBuffer = 0,
            eDescriptorTypeSampledImage,
            eDescriptorTypeMax
        };

        enum
        {
            eBufferUsageVertex = 0,
            eBufferUsageIndex,
            eBufferUsageStaging,
            eBufferUsageUniform
        };

        typedef struct descriptor_type
        {
            uint32 type;
            uint32 amount;
        } DescriptorType;

        typedef struct descriptor_layout
        {
            DescriptorType descriptorTypes[MAX_DESCRIPTOR_SETS_PER_SHADER][MAX_DESCRIPTORS_PER_SET];
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
                    layout->descriptorTypes[uiDescSet][uiDesc].type = eDescriptorTypeMax;
                    layout->descriptorTypes[uiDescSet][uiDesc].amount = 0;
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

        enum
        {
            eBlendStateAlphaBlend = 0,
            eBlendStateMax
        };

        enum
        {
            eDepthStateTestOnWriteOn = 0,
            eDepthStateOff,
            eDepthStateMax
        };

        enum
        {
            eImageLayoutUndefined = 0,
            eImageLayoutShaderRead,
            eImageLayoutColorAttachment,
            eImageLayoutSwapChainPresent,
            eImageLayoutMax
        };

        enum
        {
            eGraphicsCmdDrawCall = 0,
            eGraphicsCmdMemTransfer,
            eGraphicsCmdRenderPassBegin,
            eGraphicsCmdRenderPassEnd,
            eGraphicsCmdImageCopy,
            eGraphicsCmdMax
        };

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
                    ResourceHandle m_renderPassHandle;
                    ResourceHandle m_framebufferHandle;
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
            ResourceHandle renderPassHandle;
            DescriptorHandle descriptorHandle;
        } GraphicsPipelineParams;

        enum : uint32
        {
            eResourceTypeBuffer1D = 0,
            eResourceTypeImage2D,
            eResourceTypeMax
        };

        typedef struct graphics_resource_description
        {
            uint32 resourceType;
            Core::Math::v3ui dims;

            union
            {
                uint32 bufferUsage;

                /*struct
                {
                    uint32 ;
                };*/ // TODO: image params, e.g. format
            };

        } ResourceDesc;

        #define CREATE_RESOURCE(name) ResourceHandle name(const ResourceDesc& resDesc)
        typedef CREATE_RESOURCE(create_resource);

        #define DESTROY_RESOURCE(name) void name(ResourceHandle handle)
        typedef DESTROY_RESOURCE(destroy_resource);
        
        #define MAP_RESOURCE(name) void* name(ResourceHandle handle)
        typedef MAP_RESOURCE(map_resource);

        #define UNMAP_RESOURCE(name) void name(ResourceHandle handle)
        typedef UNMAP_RESOURCE(unmap_resource);

        #define CREATE_FRAMEBUFFER(name) ResourceHandle name(ResourceHandle* imageViewResourceHandles, uint32 numImageViewResourceHandles, uint32 width, uint32 height, ResourceHandle renderPassHandle)
        typedef CREATE_FRAMEBUFFER(create_framebuffer);

        #define DESTROY_FRAMEBUFFER(name) void name(ResourceHandle handle)
        typedef DESTROY_FRAMEBUFFER(destroy_framebuffer);

        #define CREATE_GRAPHICS_PIPELINE(name) ShaderHandle name(void* vertexShaderCode, uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 blendState, uint32 depthState, uint32 viewportWidth, uint32 viewportHeight, ResourceHandle renderPassHandle, DescriptorHandle descriptorHandle)
        typedef CREATE_GRAPHICS_PIPELINE(create_graphics_pipeline);

        #define DESTROY_GRAPHICS_PIPELINE(name) void name(ShaderHandle handle)
        typedef DESTROY_GRAPHICS_PIPELINE(destroy_graphics_pipeline);

        #define CREATE_RENDER_PASS(name) ResourceHandle name(uint32 startLayout, uint32 endLayout)
        typedef CREATE_RENDER_PASS(create_render_pass);

        #define DESTROY_RENDER_PASS(name) void name(ResourceHandle handle)
        typedef DESTROY_RENDER_PASS(destroy_render_pass);
        
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
