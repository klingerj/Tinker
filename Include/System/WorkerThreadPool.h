#pragma once

#include "../Containers/RingBuffer.h"

#define NUM_JOBS_PER_WORKER 64

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
    BYTE_ALIGN(64) volatile bool terminate;
    volatile uint8 m_threadId;
    BYTE_ALIGN(64) RingBuffer<WorkerJob*, NUM_JOBS_PER_WORKER> jobs;
} ThreadInfo;
THREAD_FUNC_TYPE WorkerThreadFunction(void* arg);


class WorkerThreadPool
{
private:
    ThreadInfo m_threads[16];

public:
    WorkerThreadPool() : WorkerThreadPool(16) {}

    WorkerThreadPool(uint32 NumThreads)
    {
        for (uint32 i = 0; i < MIN(NumThreads, 16); ++i)
        {
            m_threads[i].terminate = false;
            m_threads[i].m_threadId = i;
            Platform::LaunchThread(WorkerThreadFunction, WORKER_THREAD_STACK_SIZE, m_threads + i);
        }
    }
    
    ~WorkerThreadPool()
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
        for (uint8 i = 0; i < 16; ++i)
        {
            if (m_threads[i].jobs.Size() < 64)
            {
                m_threads[i].jobs.Push(newJob);
                break;
            }
        }

        return newJob;
    }
};
