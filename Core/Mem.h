#pragma once

#include "Core/CoreDefines.h"

namespace Tk
{
namespace Core
{

#define DISABLE_MEM_TRACKING

//#if !defined(DISABLE_MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
//#define MEM_TRACKING
//#endif

//#if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
//#endif

TINKER_API void* CoreMalloc(size_t size);
TINKER_API void CoreFree(void* ptr);

}
}
