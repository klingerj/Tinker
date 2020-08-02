#pragma once

#include "Core/CoreDefines.h"
#include "Platform/GraphicsTypes.h"

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

        #define INIT_NETWORK_CONNECTION(name) int name()
        typedef INIT_NETWORK_CONNECTION(init_network_connection);

        #define SEND_MESSAGE_TO_SERVER(name) int name()
        typedef SEND_MESSAGE_TO_SERVER(send_message_to_server);

        // Graphics
        enum
        {
            eGraphicsCmdDrawCall = 0,
            eGraphicsCmdMemTransfer,
            eGraphicsCmdRenderPassBegin,
            eGraphicsCmdRenderPassEnd,
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
                    // TODO: resources/descriptors/uniform buffers
                    uint32 m_numIndices;
                    uint32 m_numVertices;
                    uint32 m_numUVs;
                    uint32 m_indexBufferHandle;
                    uint32 m_vertexBufferHandle;
                    uint32 m_uvBufferHandle;
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
                struct
                {
                    uint32 m_renderPassHandle;
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

        #define CREATE_VERTEX_BUFFER(name) uint32 name(uint32 sizeInBytes, Graphics::BufferType bufferType)
        typedef CREATE_VERTEX_BUFFER(create_vertex_buffer);

        #define DESTROY_VERTEX_BUFFER(name) void name(uint32 handle)
        typedef DESTROY_VERTEX_BUFFER(destroy_vertex_buffer);

        #define CREATE_STAGING_BUFFER(name) Graphics::StagingBufferData name(uint32 sizeInBytes)
        typedef CREATE_STAGING_BUFFER(create_staging_buffer);

        #define DESTROY_STAGING_BUFFER(name) void name(uint32 handle)
        typedef DESTROY_STAGING_BUFFER(destroy_staging_buffer);

        #define CREATE_FRAMEBUFFER(name) uint32 name(uint32* imageViewResourceHandles, uint32 numImageViewResourceHandles, uint32 width, uint32 height)
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
        
        // Platform api functions passed from platform layer to game
        typedef struct platform_api_functions
        {
            enqueue_worker_thread_job* EnqueueWorkerThreadJob;
            wait_on_thread_job* WaitOnThreadJob;
            read_entire_file* ReadEntireFile;
            init_network_connection* InitNetworkConnection;
            send_message_to_server* SendMessageToServer;
            create_vertex_buffer* CreateVertexBuffer;
            create_staging_buffer* CreateStagingBuffer;
            destroy_vertex_buffer* DestroyVertexBuffer;
            destroy_staging_buffer* DestroyStagingBuffer;
            create_framebuffer* CreateFramebuffer;
            destroy_framebuffer* DestroyFramebuffer;
            create_image_resource* CreateImageResource;
            destroy_image_resource* DestroyImageResource;
            create_image_view_resource* CreateImageViewResource;
            destroy_image_view_resource* DestroyImageViewResource;
        } PlatformAPIFuncs;

        // Game side
        #define GAME_UPDATE(name) uint32 name(Tinker::Platform::PlatformAPIFuncs* platformFuncs, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream)
        typedef GAME_UPDATE(game_update);

        #define GAME_DESTROY(name) void name(Tinker::Platform::PlatformAPIFuncs* platformFuncs)
        typedef GAME_DESTROY(game_destroy);
    }
}
