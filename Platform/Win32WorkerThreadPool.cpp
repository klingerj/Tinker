#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Containers/RingBuffer.h"

#include <process.h>
#include <windows.h>
//#include <string.h>

#include <emmintrin.h>

#define NUM_JOBS_PER_WORKER 64
#define WORKER_THREAD_STACK_SIZE 1024 * 1024 * 2

namespace Tinker
{
    namespace Platform
    {
        void WaitOnJob(WorkerJob* job)
        {
            while (!job->m_done);
        }

        typedef struct thread_info
        {
            BYTE_ALIGN(64) volatile bool terminate = false;
            volatile bool didTerminate = true;
            uint32 threadId = 0;
            BYTE_ALIGN(64) HANDLE semaphoreHandle = INVALID_HANDLE_VALUE;
            BYTE_ALIGN(64) Containers::RingBuffer<WorkerJob*> jobs;
        } ThreadInfo;

        void __cdecl WorkerThreadFunction(void* arg)
        {
            ThreadInfo* info = (ThreadInfo*)(arg);

            /*char buffer[50] = {};
                    strcpy_s(buffer, "From thread Id: ");
                    _itoa_s(info->threadId, buffer + 16, 10, 10);
                    buffer[17] = '\n';
                    buffer[18] = '\0';
                    PrintDebugString(buffer);*/

            /* old:
            while (!info->terminate)
            {
                if (info->jobs.Size() > 0)
                {
                    WorkerJob* job;
                    info->jobs.Dequeue(&job);
                    (*job)();
                    
                    job->m_done = true;
                }

                WaitForSingleObjectEx(info->semaphoreHandle, INFINITE, FALSE);
            }
            */

        outer_loop:
            while (!info->terminate)
            {
                const uint32 limit = 4096;
                uint32 count = 0;

                while (count++ < limit)
                {
                    uint32 head = info->jobs.m_head.load(std::memory_order_acquire);
                    uint32 tail = info->jobs.m_tail.load(std::memory_order_acquire);

                    if (head - tail > 0)
                    {
                        WorkerJob* job;
                        info->jobs.Dequeue(&job);
                        (*job)();
                        job->m_done = true;
                        goto outer_loop; // reset counter until sema
                    }
                    _mm_pause();
                }

                WaitForSingleObjectEx(info->semaphoreHandle, INFINITE, FALSE);
            }

            info->didTerminate = true;
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
                    m_threads[i].jobs.Init(NUM_JOBS_PER_WORKER);
                    m_threads[i].terminate = false;
                    m_threads[i].didTerminate = false;
                    m_threads[i].threadId = i;
                    m_threads[i].semaphoreHandle = CreateSemaphoreEx(0, 0, NUM_JOBS_PER_WORKER, 0, 0, SEMAPHORE_ALL_ACCESS);
                    _beginthread(WorkerThreadFunction, WORKER_THREAD_STACK_SIZE, &m_threads[i]);
                }
            }

            void Shutdown()
            {
                // Tell the threads to wake up and terminate ASAP
                for (uint8 i = 0; i < m_numThreads; ++i)
                {
                    m_threads[i].terminate = true;
                    ReleaseSemaphore(m_threads[i].semaphoreHandle, 1, 0);
                }
                // Wait for the threads to finish their current tasks, then terminate
                for (uint8 i = 0; i < m_numThreads; ++i)
                {
                    while(!m_threads[i].didTerminate);
                }

                // Free the job buffers
                for (uint8 i = 0; i < m_numThreads; ++i)
                {
                    m_threads[i].jobs.ExplicitFree();
                }
            }

            void EnqueueNewThreadJob(WorkerJob* newJob)
            {
                // Amazing scheduler
                m_threads[m_schedulerCounter].jobs.Enqueue(newJob);
                ReleaseSemaphore(m_threads[m_schedulerCounter].semaphoreHandle, 1, 0);
                m_schedulerCounter = (m_schedulerCounter + 1) % m_numThreads;
            }
        };
    }
}
