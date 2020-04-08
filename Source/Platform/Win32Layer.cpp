#include "../../Include/Platform/PlatformAPI.h"

#include <emmintrin.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#endif

namespace Platform
{
    uint32 AtomicIncrement32(uint32 volatile* ptr)
    {
#ifdef _WIN32
        return InterlockedIncrement(ptr);
#endif
    }

    uint32 AtomicGet(uint32 *p)
    {
#ifdef _WIN32
        return *(volatile uint32*)p;
#endif
    }

    void PauseCPU()
    {
        _mm_pause();
    }

    uint64 LaunchThread(THREAD_FUNC(func), uint32 stackSize, void* argList)
    {
#ifdef _WIN32
        return _beginthread(func, stackSize, argList);
#endif
    }

    void EndThread()
    {
#ifdef _WIN32
        _endthread();
#endif
    }
}
