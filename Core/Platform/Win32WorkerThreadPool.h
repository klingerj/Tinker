#include "PlatformGameAPI.h"
#include "DataStructures/RingBuffer.h"

namespace Tk
{
namespace Platform
{

typedef struct thread_info
{
    alignas(CACHE_LINE) volatile bool terminate = false;
    volatile bool didTerminate = true;
    uint32 threadId = 0;
    alignas(CACHE_LINE) uint64 semaphoreHandle = TINKER_INVALID_HANDLE;
    alignas(CACHE_LINE) Core::RingBuffer<WorkerJob*> jobs;
} ThreadInfo;

struct WorkerThreadPool
{
private:
    ThreadInfo m_threads[16];
    uint32 m_schedulerCounter = 0;
    uint32 m_numThreads = 0;

public:
    void Startup(uint32 NumThreads);
    void Shutdown();
    void EnqueueNewThreadJob(WorkerJob* newJob);
};

extern WorkerThreadPool g_ThreadPool;

}
}
