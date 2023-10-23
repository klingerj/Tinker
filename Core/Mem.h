#pragma once

#include "CoreDefines.h"
#include "Utility/MemTracker.h"

namespace Tk
{
namespace Core
{

// TODO can we delete the "dbg" variants? probably 
#ifndef ENABLE_MEM_TRACKING
TINKER_API void* CoreMalloc(size_t size);
#else
TINKER_API void* CoreMallocDbg(size_t size);
#define CoreMalloc(size) CoreMallocDbg(size)
#endif

TINKER_API void CoreFree(void* ptr);

#ifndef ENABLE_MEM_TRACKING
TINKER_API void* CoreMallocAligned(size_t size, size_t alignment);
#else
TINKER_API void* CoreMallocAlignedDbg(size_t size, size_t alignment);
#define CoreMallocAligned(size, alignment) CoreMallocAlignedDbg(size, alignment)
#endif

TINKER_API void CoreFreeAligned(void* ptr);

}
}
