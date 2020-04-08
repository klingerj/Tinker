#include "../../Include/System/WorkerThreadPool.h"

THREAD_FUNC_TYPE WorkerThreadFunction(void* arg)
{
    ThreadInfo* info = (ThreadInfo*)(arg);
    
    while (!info->terminate)
    {
        if (info->jobs.Size() > 0)
        {
            WorkerJob* job;
            info->jobs.Dequeue(&job);
            (*job)();
            job->m_done = true;
        }
        Platform::PauseCPU();
    }
    
    info->didTerminate = true;
    Platform::EndThread();
}
