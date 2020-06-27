#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
    namespace Platform
    {
        // I/O
        void Print(const char* str, size_t len);

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
        
        // Graphics
        enum
        {
            eGraphicsCmdDrawCall = 0,
            eGraphicsCmdMax
        };

        typedef struct graphics_command
        {
            uint32 m_commandType;
            union
            {
                struct
                {
                    // TODO: resources/descriptors/uniform buffers
                    uint32 m_indexBufferHandle;
                    uint32 m_vertexBufferHandle;
                    uint32 m_uvBufferHandle;
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

        // Platform api functions passed from platform layer to game
        typedef struct platform_api_functions
        {
            // TODO: more functions, e.g. graphics resource creation
            enqueue_worker_thread_job* EnqueueWorkerThreadJob;
        } PlatformAPIFuncs;

        #define GAME_UPDATE(name) uint32 name(Tinker::Platform::PlatformAPIFuncs* platformFuncs, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream)
        typedef GAME_UPDATE(game_update);
    }
}
