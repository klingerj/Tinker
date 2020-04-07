#include "WorkerThreadPool.h"
#include <emmintrin.h>

THREAD_FUNC_TYPE WorkerThreadFunction(void* arg)
{
    ThreadInfo* info = (ThreadInfo*)(arg);
    
    while (!info->terminate)
    {
        if (info->jobs.Size() > 0)
        {
            WorkerJob* job = info->jobs.Pop();
            (*job)();
            job->m_done = true;
        }
        _mm_pause();
    }
    
    Platform::EndThread();
}
