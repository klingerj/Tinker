#include "Core/Utilities/MemTracker.h"
#include "Core/Utilities/Mem.h"

namespace Tinker
{
namespace Core
{
namespace Utility
{

#if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
// TODO: overload operator new to call CoreMalloc functions
#endif

void* CoreMalloc(size_t size)
{
    void* ptr = malloc(size);
    #if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
    RecordMemAlloc((uint32)size, ptr);
    #endif
    return ptr;
}

void CoreFree(void* ptr)
{
    RecordMemDealloc(ptr);
    #if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
    free(ptr);
    #endif
}

}
}
}

