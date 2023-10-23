#include "Mem.h"
#include "Platform/PlatformGameAPI.h"

namespace Tk
{
namespace Core
{

#ifndef ENABLE_MEM_TRACKING
void* CoreMalloc(size_t size)
{
    return malloc(size);
}
#endif

void* CoreMallocDbg(size_t size)
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

#ifndef ENABLE_MEM_TRACKING
void* CoreMallocAligned(size_t size, size_t alignment)
{
    return Tk::Platform::AllocAlignedRaw(size, alignment);
}
#endif

void* CoreMallocAlignedDbg(size_t size, size_t alignment)
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

}
}

