#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Core
{
namespace Utility
{

// TODO: make this stuff thread-safe
void RecordMemAlloc(uint64 sizeInBytes, void* memPtr);
void RecordMemDealloc(void* memPtr);
void DebugOutputAllMemAllocs();

}
}
}
