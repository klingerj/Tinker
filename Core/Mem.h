#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Core
{

TINKER_API void* CoreMalloc(size_t size);
TINKER_API void CoreFree(void* ptr);

}
}
