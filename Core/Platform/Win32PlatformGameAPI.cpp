#include "PlatformGameAPI.h"

#include <windows.h>

namespace Tk
{
namespace Platform
{

// I/O
PRINT_DEBUG_STRING(PrintDebugString)
{
    OutputDebugString(str);
}

// Memory
ALLOC_ALIGNED(AllocAligned)
{
    #ifdef MEM_TRACKING
    return _aligned_malloc_dbg(size, alignment, filename, lineNum);
    #else
    return _aligned_malloc(size, alignment);
    #endif
}

FREE_ALIGNED(FreeAligned)
{
    _aligned_free(ptr);
}

}
}
