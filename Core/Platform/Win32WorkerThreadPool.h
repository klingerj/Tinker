#include "DataStructures/RingBuffer.h"
#include "PlatformGameAPI.h"

namespace Tk
{
  namespace Platform
  {
    namespace ThreadPool
    {
      void Startup(uint32 NumThreads);
      void Shutdown();
      void EnqueueSingleJob(WorkerJob* Job);
      void EnqueueJobList(WorkerJobList* JobList);
      void EnqueueJobSubList(WorkerJobList* JobList, uint32 NumJobs);

      uint32 NumWorkerThreads();
    } //namespace ThreadPool
  } //namespace Platform
} //namespace Tk
