#pragma once

#include "../Containers/RingBuffer.h"

#define NUM_JOBS_PER_WORKER 64

namespace Tinker
{
    class WorkerJob
    {
    public:
        volatile bool m_done = false;

        virtual void operator()() = 0;
    };

    // Lets us queue up any lambda or function pointer in the thread pool
    template <typename T>
    class JobFunc : public WorkerJob
    {
    public:
        T m_t;

        JobFunc(T t) : m_t(t) {}
        virtual void operator()() override { m_t(); };
    };

    typedef struct thread_info
    {
        BYTE_ALIGN(64) volatile bool terminate = false;
        volatile bool didTerminate = true;
        volatile uint8 threadId = 0;
        BYTE_ALIGN(64) Containers::RingBuffer<WorkerJob*, NUM_JOBS_PER_WORKER> jobs;
    } ThreadInfo;
    THREAD_FUNC_TYPE WorkerThreadFunction(void* arg);

    class WorkerThreadPool
    {
    private:
        ThreadInfo m_threads[16];
        uint8 m_schedulerCounter = 0;
        uint8 m_numThreads = 0;

    public:
        WorkerThreadPool() {}
        ~WorkerThreadPool() {}

        void Startup(uint32 NumThreads)
        {
            m_numThreads = MIN(NumThreads, 16);
            for (uint32 i = 0; i < m_numThreads; ++i)
            {
                while(!m_threads[i].didTerminate);
                m_threads[i].terminate = false;
                m_threads[i].didTerminate = false;
                m_threads[i].threadId = i;
                m_threads[i].jobs.Clear();
                Platform::LaunchThread(WorkerThreadFunction, WORKER_THREAD_STACK_SIZE, m_threads + i);
            }
        }

        void Shutdown()
        {
            for (uint8 i = 0; i < 16; ++i)
            {
                m_threads[i].terminate = true;
            }
        }

        template <typename T>
        WorkerJob* NewJob(T t)
        {
            WorkerJob* newJob = new JobFunc(t);

            // Amazing scheduler
            m_threads[m_schedulerCounter].jobs.Enqueue(newJob);
            m_schedulerCounter = (m_schedulerCounter + 1) % m_numThreads;

            return newJob;
        }
    };
}
