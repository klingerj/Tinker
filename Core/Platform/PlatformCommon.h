#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
// TODO: other platform headers
#endif

namespace Tk
{
namespace Platform
{

struct PlatformWindowHandles
{
    #ifdef _WIN32
    HINSTANCE instance;
    HWND windowHandle;
    #endif
    // TODO: other platform window handles

};

inline void* AllocAlignedRaw(size_t size, size_t alignment)
{
    #ifdef _WIN32
    return _aligned_malloc(size, alignment);
    #endif
}

inline void FreeAlignedRaw(void* ptr)
{
    #ifdef _WIN32
    _aligned_free(ptr);
    #endif
}

}
}
