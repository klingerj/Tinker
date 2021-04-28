#pragma once

namespace Tinker
{
namespace Core
{

#define DISABLE_MEM_TRACKING

#if !defined(DISABLE_MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
#define MEM_TRACKING
#endif

#if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
#endif
void* CoreMalloc(size_t size);
void CoreFree(void* ptr);

}
}

