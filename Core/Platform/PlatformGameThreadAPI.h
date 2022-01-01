#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Platform
{

struct  WorkerJob
{
public:
    alignas(CACHE_LINE) volatile uint32 m_done = 0;
    virtual ~WorkerJob() {}

    virtual void operator()() = 0;
};

template <typename T>
struct JobFunc : public WorkerJob
{
public:
    alignas(CACHE_LINE) T m_func;

    JobFunc(T func) : m_func(func) {}
    void operator()() override { m_func(); };
};

inline void WaitOnJob(WorkerJob* job)
{
    while (!job->m_done);
}

struct WorkerJobList
{
public:
    uint32 m_numJobs;
    WorkerJob** m_jobs;

    WorkerJobList() : m_numJobs(0), m_jobs(nullptr) {}
    ~WorkerJobList() {}

    void Init(uint32 numJobs)
    {
        m_numJobs = numJobs;
        m_jobs = (WorkerJob**)new WorkerJob*[m_numJobs];
        for (uint32 i = 0; i < m_numJobs; ++i)
        {
            m_jobs[i] = nullptr;
        }
    }

    void FreeList()
    {
        if (m_jobs)
        {
            for (uint32 i = 0; i < m_numJobs; ++i)
            {
                if (m_jobs[i])
                {
                    delete(m_jobs[i]);
                }
            }
            delete m_jobs;
            m_jobs = nullptr;
        }
    }

    void WaitOnJobs()
    {
        for (uint32 i = 0; i < m_numJobs; ++i)
        {
            if (m_jobs[i])
            {
                WaitOnJob(m_jobs[i]);
            }
        }
    }
};

template <typename T>
WorkerJob* CreateNewThreadJob(T t)
{
    JobFunc<T>* NewJob = new JobFunc<T>(t);
    NewJob->m_done = 0;
    return NewJob;
}

#define ENQUEUE_WORKER_THREAD_JOB(name) TINKER_API void name(WorkerJob* Job)
ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob);

#define ENQUEUE_WORKER_THREAD_JOB_LIST(name) TINKER_API void name(WorkerJobList* JobList)
ENQUEUE_WORKER_THREAD_JOB_LIST(EnqueueWorkerThreadJobList_Unassisted);
ENQUEUE_WORKER_THREAD_JOB_LIST(EnqueueWorkerThreadJobList_Assisted);

}
}
