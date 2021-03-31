#include "PlatformGameAPI.h"

#include <windows.h>

namespace Tinker
{
namespace Platform
{

// I/O
void PrintDebugString(const char* str)
{
    OutputDebugString(str);
}

// Memory
void* AllocAligned(size_t size, size_t alignment, const char* filename, int lineNum)
{
    #ifdef MEM_TRACKING
    return _aligned_malloc_dbg(size, alignment, filename, lineNum);
    #else
    return _aligned_malloc(size, alignment);
    #endif
}

void FreeAligned(void* ptr)
{
    _aligned_free(ptr);
}

}
}
