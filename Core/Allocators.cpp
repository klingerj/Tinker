#include "Allocators.h"
#include "Platform/PlatformGameThreadAPI.h"

namespace Tk
{
  namespace Core
  {
    Vector<StackAllocator> g_PerThreadTempAllocators;
    static const size_t THREAD_TEMP_ALLOCATOR_INIT_SIZE_BYTES = Mebibytes(1);

    void InitPerThreadAllocators(uint32 numThreadsIncludingMain)
    {
      g_PerThreadTempAllocators.Resize(numThreadsIncludingMain);

      for (uint32 i = 0; i < g_PerThreadTempAllocators.Size(); ++i)
      {
        g_PerThreadTempAllocators[i].Init(THREAD_TEMP_ALLOCATOR_INIT_SIZE_BYTES,
                                          CACHE_LINE);
      }
    }

    StackAllocator& GetThreadTempAllocator()
    {
      uint32 currentThreadID = Tk::Platform::GetCurrThreadID();
      return g_PerThreadTempAllocators[currentThreadID];
    }
  } //namespace Core
} //namespace Tk
