#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Containers/RingBuffer.h"

#include <process.h>

#define NUM_JOBS_PER_WORKER 64
#define WORKER_THREAD_STACK_SIZE SIZE_2MB

namespace Tinker
{
    namespace Platform
    {
        typedef struct thread_info
        {
            BYTE_ALIGN(64) volatile bool terminate = false;
            volatile bool didTerminate = true;
            volatile uint32 threadId = 0;
            BYTE_ALIGN(64) Containers::RingBuffer<WorkerJob*, NUM_JOBS_PER_WORKER> jobs;
        } ThreadInfo;

        void __cdecl WorkerThreadFunction(void* arg)
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
            _endthread();
        }

        class WorkerThreadPool
        {
        private:
            ThreadInfo m_threads[16];
            uint32 m_schedulerCounter = 0;
            uint32 m_numThreads = 0;

        public:
            void Startup(uint32 NumThreads)
            {
                m_numThreads = MIN(NumThreads, 16);
                for (uint32 i = 0; i < m_numThreads; ++i)
                {
                    m_threads[i].terminate = false;
                    m_threads[i].didTerminate = false;
                    m_threads[i].threadId = i;
                    m_threads[i].jobs.Clear();
                    _beginthread(WorkerThreadFunction, WORKER_THREAD_STACK_SIZE, m_threads + i);
                }
            }

            void Shutdown()
            {
                for (uint8 i = 0; i < m_numThreads; ++i)
                {
                    m_threads[i].terminate = true;
                }
                for (uint8 i = 0; i < m_numThreads; ++i)
                {
                    while(!m_threads[i].didTerminate);
                }
            }

            void EnqueueNewThreadJob(WorkerJob* newJob)
            {
                // Amazing scheduler
                m_threads[m_schedulerCounter].jobs.Enqueue(newJob);
                m_schedulerCounter = (m_schedulerCounter + 1) % m_numThreads;
            }
        };
    }
}
