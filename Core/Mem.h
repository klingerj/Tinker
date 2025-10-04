#pragma once

#include "CoreDefines.h"
#include "Utility/MemTracker.h"

namespace Tk
{
  namespace Core
  {
    TINKER_API void* CoreMalloc(size_t size);
    TINKER_API void CoreFree(void* ptr);
    TINKER_API void* CoreMallocAligned(size_t size, size_t alignment);
    TINKER_API void CoreFreeAligned(void* ptr);
  } //namespace Core
} //namespace Tk
