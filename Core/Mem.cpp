#include "Core/Utilities/MemTracker.h"
#include "Core/Mem.h"

namespace Tinker
{
namespace Core
{

#if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
// TODO: overload operator new to call CoreMalloc functions
#endif

void* CoreMalloc(size_t size)
{
    void* ptr = malloc(size);
    #if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
    Utility::RecordMemAlloc((uint32)size, ptr);
    #endif
    return ptr;
}

void CoreFree(void* ptr)
{
    Utility::RecordMemDealloc(ptr);
    #if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
    free(ptr);
    #endif
}

}
}

