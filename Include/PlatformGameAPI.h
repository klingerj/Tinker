#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
    namespace Platform
    {
        // Threading
        class WorkerJob
        {
        public:
            volatile bool m_done = false;
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
        
        typedef struct game_memory
        {
            // TODO: game memory stuff
            enqueue_worker_thread_job *EnqueueWorkerThreadJob;
        } GameMemory;

        #define GAME_UPDATE(name) uint32 name(Tinker::Platform::GameMemory* memory)
        typedef GAME_UPDATE(game_update);

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
    }
}
