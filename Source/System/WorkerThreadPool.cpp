#include "WorkerThreadPool.h"

WorkerThreadPool g_WorkerThreadPool;

void PushNewJob(WorkerThread* thread, WorkerJob* job)
{
    thread->m_jobs.Push(job);
}
