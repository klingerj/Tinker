#include "Mem.h"
#include "Platform/PlatformGameAPI.h"

namespace Tk
{
  namespace Core
  {
    void* CoreMalloc(size_t size)
    {
      void* ptr = malloc(size);
#ifdef ENABLE_MEM_TRACKING
      Utility::RecordMemAlloc((uint64)size, ptr);
#endif
      return ptr;
    }

    void CoreFree(void* ptr)
    {
      free(ptr);
#ifdef ENABLE_MEM_TRACKING
      Utility::RecordMemDealloc(ptr);
#endif
    }

    void* CoreMallocAligned(size_t size, size_t alignment)
    {
      void* ptr = Tk::Platform::AllocAlignedRaw(size, alignment);
#ifdef ENABLE_MEM_TRACKING
      Utility::RecordMemAlloc((uint64)size, ptr);
#endif
      return ptr;
    }

    void CoreFreeAligned(void* ptr)
    {
      Tk::Platform::FreeAlignedRaw(ptr);
#ifdef ENABLE_MEM_TRACKING
      Utility::RecordMemDealloc(ptr);
#endif
    }
  } //namespace Core
} //namespace Tk
