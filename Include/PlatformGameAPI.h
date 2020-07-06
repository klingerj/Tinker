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

        // Atomic Ops
        uint32 AtomicIncrement32(uint32 volatile* ptr);
        uint32 AtomicGet32(uint32* p);

        // Intrinsics
        void PauseCPU();

        // Threading
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

        void WaitOnJob(WorkerJob* job);

        template <typename T>
        WorkerJob* CreateNewThreadJob(T t)
        {
            JobFunc<T>* jobMem = (JobFunc<T>*)AllocAligned(sizeof(JobFunc<T>), 64);
            return new (jobMem) JobFunc(t);
        }

        #define ENQUEUE_WORKER_THREAD_JOB(name) void name(WorkerJob* newJob)
        typedef ENQUEUE_WORKER_THREAD_JOB(enqueue_worker_thread_job);

        #define READ_ENTIRE_FILE(name) uint8* name(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
        typedef READ_ENTIRE_FILE(read_entire_file);

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
                    uint32 m_dstBufferType; // 0 = vertex, 1 = index. TODO: shouldn't use this
                    uint32 m_srcStagingBufferHandle; // src
                    uint32 m_dstVertexBufferHandle; // dst
                    uint32 m_dstIndexBufferHandle; // dst
                };

                // Begin render pass
                struct
                {
                    uint32 m_renderPassHandle;
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

        #define CREATE_VERTEX_BUFFER(name) uint32 name(uint32 sizeInBytes)
        typedef CREATE_VERTEX_BUFFER(create_vertex_buffer);

        #define DESTROY_VERTEX_BUFFER(name) void name(uint32 handle)
        typedef DESTROY_VERTEX_BUFFER(destroy_vertex_buffer);

        typedef struct staging_buffer_data
        {
            uint32 handle;
            void* memory;
        } StagingBufferData;

        #define CREATE_STAGING_BUFFER(name) StagingBufferData name(uint32 sizeInBytes)
        typedef CREATE_STAGING_BUFFER(create_staging_buffer);

        #define DESTROY_STAGING_BUFFER(name) void name(uint32 handle)
        typedef DESTROY_STAGING_BUFFER(destroy_staging_buffer);
        
        // Platform api functions passed from platform layer to game
        typedef struct platform_api_functions
        {
            enqueue_worker_thread_job* EnqueueWorkerThreadJob;
            read_entire_file* ReadEntireFile;
            create_vertex_buffer* CreateVertexBuffer;
            create_staging_buffer* CreateStagingBuffer;
            destroy_vertex_buffer* DestroyVertexBuffer;
            destroy_staging_buffer* DestroyStagingBuffer;
        } PlatformAPIFuncs;

        // Game side
        #define GAME_UPDATE(name) uint32 name(Tinker::Platform::PlatformAPIFuncs* platformFuncs, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream)
        typedef GAME_UPDATE(game_update);

        #define GAME_DESTROY(name) void name(Tinker::Platform::PlatformAPIFuncs* platformFuncs)
        typedef GAME_DESTROY(game_destroy);
    }
}
