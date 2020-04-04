#include "PlatformAPI.h"

#ifdef _WIN32
#include <windows.h>

uint32 AtomicIncrement32(uint32 volatile* ptr)
{
    return InterlockedIncrement(ptr);
}
#endif
