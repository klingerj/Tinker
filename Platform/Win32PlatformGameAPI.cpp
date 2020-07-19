#include "../Include/PlatformGameAPI.h"

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
        void* AllocAligned(size_t size, size_t alignment)
        {
            return _aligned_malloc(size, alignment);
        }

        void FreeAligned(void* ptr)
        {
            _aligned_free(ptr);
        }
    }
}
