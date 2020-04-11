#include "../../Include/Platform/PlatformAPI.h"

#include <windows.h>
#include <process.h>

#include <emmintrin.h>

namespace Tinker
{
    namespace Platform
    {
        uint32 AtomicIncrement32(uint32 volatile* ptr)
        {
            return InterlockedIncrement(ptr);
        }

        uint32 AtomicGet(uint32 *p)
        {
            return *(volatile uint32*)p;
        }

        void PauseCPU()
        {
            _mm_pause();
        }

        uint64 LaunchThread(THREAD_FUNC(func), uint32 stackSize, void* argList)
        {
            return _beginthread(func, stackSize, argList);
        }

        void EndThread()
        {
            _endthread();
        }

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
