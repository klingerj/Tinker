#include "Utility/MemTracker.h"
#include "Mem.h"

namespace Tk
{
namespace Core
{

#define ENABLE_MEM_TRACKING

#if defined(ENABLE_MEM_TRACKING) && defined(_WIN32)
#define MEM_TRACKING
#endif

#if defined(MEM_TRACKING) && defined(_WIN32)
// TODO: overload operator new to record memory allocations
#endif

void* CoreMalloc(size_t size)
{
    void* ptr = malloc(size);
    #if defined(MEM_TRACKING)
    Utility::RecordMemAlloc((uint64)size, ptr);
    #endif
    return ptr;
}

void CoreFree(void* ptr)
{
    free(ptr);
    #if defined(MEM_TRACKING)
    Utility::RecordMemDealloc(ptr);
    #endif
}

}
}

