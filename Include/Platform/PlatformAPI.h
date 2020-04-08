#pragma once

#include "../System/SystemDefines.h"

namespace Platform
{
    // Atomic ops
    uint32 AtomicGet(uint32* p);
    uint32 AtomicIncrement32(uint32 volatile *ptr);

    // Intrinsics
    void PauseCPU();

#define WORKER_THREAD_STACK_SIZE SIZE_2MB

#ifdef _WIN32
#define THREAD_FUNC_TYPE void __cdecl
#define THREAD_FUNC(f) void (__cdecl* f)(void*)
#endif

    uint64 LaunchThread(THREAD_FUNC(func), uint32 stackSize, void* argList);
    void EndThread();
}
