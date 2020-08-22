#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
    namespace Platform
    {
        // I/O
        void PrintDebugString(const char* str);

        // Memory
        void* AllocAligned(size_t size, size_t alignment);
        void FreeAligned(void* ptr);

        class WorkerJob
        {
        public:
            BYTE_ALIGN(64) volatile bool m_done = false;
            virtual ~WorkerJob() {}

            virtual void operator()() = 0;
        };

        template <typename T>
        class JobFunc : public WorkerJob
        {
        public:
            T m_t;

            JobFunc(T t) : m_t(t) {}
            virtual void operator()() override { m_t(); };
        };

        template <typename T>
        WorkerJob* CreateNewThreadJob(T t)
        {
            JobFunc<T>* jobMem = (JobFunc<T>*)AllocAligned(sizeof(JobFunc<T>), 64);
            return new (jobMem) JobFunc(t);
        }
        #define WAIT_ON_THREAD_JOB(name) void name(WorkerJob* job)
        typedef WAIT_ON_THREAD_JOB(wait_on_thread_job);

        #define ENQUEUE_WORKER_THREAD_JOB(name) void name(WorkerJob* newJob)
        typedef ENQUEUE_WORKER_THREAD_JOB(enqueue_worker_thread_job);

        #define READ_ENTIRE_FILE(name) uint8* name(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
        typedef READ_ENTIRE_FILE(read_entire_file);

        #define GET_FILE_SIZE(name) uint32 name(const char* filename)
        typedef GET_FILE_SIZE(get_file_size);

        #define INIT_NETWORK_CONNECTION(name) int name()
        typedef INIT_NETWORK_CONNECTION(init_network_connection);

        #define END_NETWORK_CONNECTION(name) int name()
        typedef END_NETWORK_CONNECTION(end_network_connection);

        #define SEND_MESSAGE_TO_SERVER(name) int name()
        typedef SEND_MESSAGE_TO_SERVER(send_message_to_server);
        
        #define SYSTEM_COMMAND(name) int name(const char* command)
        typedef SYSTEM_COMMAND(system_command);

        // Input

        enum
        {
            eKeyA = 0,
            eKeyB,
            eKeyC,
            eKeyD,
            eKeyE,
            eKeyF,
            eKeyG,
            eKeyH,
            eKeyI,
            eKeyJ,
            eKeyK,
            eKeyL,
            eKeyM,
            eKeyN,
            eKeyO,
            eKeyP,
            eKeyQ,
            eKeyR,
            eKeyS,
            eKeyT,
            eKeyU,
            eKeyV,
            eKeyW,
            eKeyX,
            eKeyY,
            eKeyZ,
            eKey0,
            eKey1,
            eKey2,
            eKey3,
            eKey4,
            eKey5,
            eKey6,
            eKey7,
            eKey8,
            eKey9,
            eKeyF1,
            eKeyF2,
            eKeyF3,
            eKeyF4,
            eKeyF5,
            eKeyF6,
            eKeyF7,
            eKeyF8,
            eKeyF9,
            eKeyF10,
            eKeyF11,
            eKeyF12,
            eMaxKeycodes
        };

        typedef struct keycode_state
        {
            uint8 isDown; // is the key currently down
            uint8 numStateChanges; // number of times the key went up/down
        } KeycodeState;

        typedef struct input_state_delta
        {
            // TODO: gamepad input

            // Keyboard input
            KeycodeState keyCodes[eMaxKeycodes];
        } InputStateDeltas;

        // Graphics

        enum
        {
            eBufferUsageVertex = 0,
            eBufferUsageIndex,
            eBufferUsageStaging,
            eBufferUsageUniform
        };

        #define MAX_DESCRIPTOR_SETS_PER_SHADER 1
        #define MAX_DESCRIPTORS_PER_SET 1

        enum
        {
            eDescriptorTypeBuffer = 0,
            eDescriptorTypeSampledImage,
            eDescriptorTypeMax
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

        typedef struct descriptor_set_handles
        {
            uint32 handles[MAX_DESCRIPTORS_PER_SET];
        } DescriptorSetDataHandles;

        inline void InitDescLayout(DescriptorLayout* layout)
        {
            for (uint32 uiDescSet = 0; uiDescSet < MAX_DESCRIPTOR_SETS_PER_SHADER; ++uiDescSet)
            {
                for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTORS_PER_SET; ++uiDesc)
                {
                    layout->descriptorTypes[uiDescSet][uiDesc].type = TINKER_INVALID_HANDLE;
                    layout->descriptorTypes[uiDescSet][uiDesc].amount = 0;
                }
            }
        }

        inline void InitDescSetDataHandles(DescriptorSetDataHandles* descSetDataHandles)
        {
            for (uint32 uiDesc = 0; uiDesc < MAX_DESCRIPTORS_PER_SET; ++uiDesc)
            {
                descSetDataHandles->handles[uiDesc] = TINKER_INVALID_HANDLE;
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
            union
            {
                // Draw call
                struct
                {
                    uint32 m_numIndices;
                    uint32 m_numVertices;
                    uint32 m_numUVs;
                    uint32 m_indexBufferHandle;
                    uint32 m_vertexBufferHandle;
                    uint32 m_uvBufferHandle;
                    uint32 m_shaderHandle;
                    DescriptorSetDataHandles m_descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
                };

                // Memory transfer
                struct
                {
                    uint32 m_sizeInBytes;
                    uint32 m_srcBufferHandle; // src
                    uint32 m_dstBufferHandle; // dst
                };

                // Begin render pass
                struct
                {
                    uint32 m_renderPassHandle;
                    uint32 m_framebufferHandle;
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
                    uint32 m_srcImgHandle;
                    uint32 m_dstImgHandle;
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
            uint32 renderPassHandle;
            uint32 descriptorHandle;
        } GraphicsPipelineParams;

        typedef struct buffer_data
        {
            uint32 handle;
            void* memory;
        } BufferData;

        #define CREATE_BUFFER(name) Tinker::Platform::BufferData name(uint32 sizeInBytes, uint32 bufferUsage)
        typedef CREATE_BUFFER(create_buffer);

        #define DESTROY_BUFFER(name) void name(uint32 handle, uint32 bufferUsage)
        typedef DESTROY_BUFFER(destroy_buffer);

        #define CREATE_FRAMEBUFFER(name) uint32 name(uint32* imageViewResourceHandles, uint32 numImageViewResourceHandles, uint32 width, uint32 height, uint32 renderPassHandle)
        typedef CREATE_FRAMEBUFFER(create_framebuffer);

        #define DESTROY_FRAMEBUFFER(name) void name(uint32 handle)
        typedef DESTROY_FRAMEBUFFER(destroy_framebuffer);

        #define CREATE_IMAGE_RESOURCE(name) uint32 name(uint32 width, uint32 height)
        typedef CREATE_IMAGE_RESOURCE(create_image_resource);

        #define DESTROY_IMAGE_RESOURCE(name) void name(uint32 handle)
        typedef DESTROY_IMAGE_RESOURCE(destroy_image_resource);

        #define CREATE_IMAGE_VIEW_RESOURCE(name) uint32 name(uint32 imageResourceHandle)
        typedef CREATE_IMAGE_VIEW_RESOURCE(create_image_view_resource);

        #define DESTROY_IMAGE_VIEW_RESOURCE(name) void name(uint32 handle)
        typedef DESTROY_IMAGE_VIEW_RESOURCE(destroy_image_view_resource);

        #define CREATE_GRAPHICS_PIPELINE(name) uint32 name(void* vertexShaderCode, uint32 numVertexShaderBytes, void* fragmentShaderCode, uint32 numFragmentShaderBytes, uint32 blendState, uint32 depthState, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassHandle, uint32 descriptorHandle)
        typedef CREATE_GRAPHICS_PIPELINE(create_graphics_pipeline);

        #define DESTROY_GRAPHICS_PIPELINE(name) void name(uint32 handle)
        typedef DESTROY_GRAPHICS_PIPELINE(destroy_graphics_pipeline);

        #define CREATE_RENDER_PASS(name) uint32 name(uint32 startLayout, uint32 endLayout)
        typedef CREATE_RENDER_PASS(create_render_pass);

        #define DESTROY_RENDER_PASS(name) void name(uint32 handle)
        typedef DESTROY_RENDER_PASS(destroy_render_pass);
        
        #define CREATE_DESCRIPTOR(name) uint32 name(DescriptorLayout* descLayout)
        typedef CREATE_DESCRIPTOR(create_descriptor);

        #define DESTROY_DESCRIPTOR(name) void name(uint32 handle)
        typedef DESTROY_DESCRIPTOR(destroy_descriptor);

        #define DESTROY_ALL_DESCRIPTORS(name) void name()
        typedef DESTROY_ALL_DESCRIPTORS(destroy_all_descriptors);

        #define WRITE_DESCRIPTOR(name) void name(DescriptorLayout* descLayout, uint32 descSetHandle, DescriptorSetDataHandles* descSetDataHandles)
        typedef WRITE_DESCRIPTOR(write_descriptor);

        // Platform api functions passed from platform layer to game
        typedef struct platform_api_functions
        {
            enqueue_worker_thread_job* EnqueueWorkerThreadJob;
            wait_on_thread_job* WaitOnThreadJob;
            read_entire_file* ReadEntireFile;
            get_file_size* GetFileSize;
            init_network_connection* InitNetworkConnection;
            end_network_connection* EndNetworkConnection;
            send_message_to_server* SendMessageToServer;
            system_command* SystemCommand;
            create_buffer* CreateBuffer;
            destroy_buffer* DestroyBuffer;
            create_framebuffer* CreateFramebuffer;
            destroy_framebuffer* DestroyFramebuffer;
            create_image_resource* CreateImageResource;
            destroy_image_resource* DestroyImageResource;
            create_image_view_resource* CreateImageViewResource;
            destroy_image_view_resource* DestroyImageViewResource;
            create_graphics_pipeline* CreateGraphicsPipeline;
            destroy_graphics_pipeline* DestroyGraphicsPipeline;
            create_render_pass* CreateRenderPass;
            destroy_render_pass* DestroyRenderPass;
            create_descriptor* CreateDescriptor;
            destroy_descriptor* DestroyDescriptor;
            destroy_all_descriptors* DestroyAllDescriptors;
            write_descriptor* WriteDescriptor;
        } PlatformAPIFuncs;

        // Game side
        #define GAME_UPDATE(name) uint32 name(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight, const Tinker::Platform::InputStateDeltas* inputStateDeltas)
        typedef GAME_UPDATE(game_update);

        #define GAME_DESTROY(name) void name(Tinker::Platform::PlatformAPIFuncs* platformFuncs)
        typedef GAME_DESTROY(game_destroy);

        #define GAME_WINDOW_RESIZE(name) void name(Tinker::Platform::PlatformAPIFuncs* platformFuncs, uint32 newWindowWidth, uint32 newWindowHeight)
        typedef GAME_WINDOW_RESIZE(game_window_resize);
    }
}
