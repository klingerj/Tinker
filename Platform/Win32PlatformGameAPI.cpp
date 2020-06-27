#include "../Include/PlatformGameAPI.h"

#include <windows.h>
#include <emmintrin.h>

namespace Tinker
{
    namespace Platform
    {
        void WaitOnJob(WorkerJob* job)
        {
            while (!job->m_done);
        }

        // I/O
        void Print(const char* str, size_t len)
        {
            HANDLE stdout = CreateFileA("CONOUT$", GENERIC_WRITE,  FILE_SHARE_WRITE, FALSE, OPEN_EXISTING, 0, 0);
            if (stdout)
            {
                DWORD numBytesWritten;
                WriteFile(stdout, str, (DWORD)len, &numBytesWritten, 0);
            }
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

        // Atomic Ops
        uint32 AtomicIncrement32(uint32 volatile* ptr)
        {
            return InterlockedIncrement(ptr);
        }

        uint32 AtomicGet32(uint32* p)
        {
            // NOTE: Only correct on x86
            return *(volatile uint32*)p;
        }

        // Intrinsics
        void PauseCPU()
        {
            _mm_pause();
        }
    }
}
