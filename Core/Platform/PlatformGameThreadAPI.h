#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Platform
{

// TODO: do an aligned alloc for these jobs via PlatformGameAPI.h
// should move all this stuff into a cpp file
class /*alignas(CACHE_LINE)*/ WorkerJob
{
public:
    alignas(CACHE_LINE) volatile bool m_done = false;
    virtual ~WorkerJob() {}

    virtual void operator()() = 0;
};

template <typename T>
class JobFunc : public WorkerJob
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

class WorkerJobList
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
    return new JobFunc<T>(t);
}

#define ENQUEUE_WORKER_THREAD_JOB(name) void name(WorkerJob* newJob)
typedef ENQUEUE_WORKER_THREAD_JOB(enqueue_worker_thread_job);

}
}
