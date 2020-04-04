#pragma once

#include "../Containers/RingBuffer.h"

#define NUM_JOBS_PER_WORKER 64

class WorkerJob
{
public:
    virtual void operator()() = 0;
};

// Lets us queue up any lambda or function pointer in the thread pool
template <typename T>
class JobFunc : public WorkerJob
{
private:
    T m_t;
    bool m_done = false;

public:
    JobFunc(T t) : m_t(t) {}
    virtual void operator()() override { m_t(); };
};

typedef struct worker_thread
{
    RingBuffer<WorkerJob*, NUM_JOBS_PER_WORKER> m_jobs;
    uint32 m_threadId = 0;
} WorkerThread;
void PushNewJob(WorkerThread* thread, WorkerJob* job);

class WorkerThreadPool
{
private:
    WorkerThread m_threads[4];

public:
    template <typename T>
    WorkerJob* NewJob(T t)
    {
        WorkerJob* newJob = new JobFunc(t);
        PushNewJob(m_threads, newJob);
        return newJob;
    }
};

extern WorkerThreadPool g_WorkerThreadPool;
