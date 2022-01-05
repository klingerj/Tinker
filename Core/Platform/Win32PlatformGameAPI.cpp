#include "PlatformGameAPI.h"

#include <windows.h>

namespace Tk
{
namespace Platform
{

// I/O
PRINT_DEBUG_STRING(PrintDebugString)
{
    OutputDebugString(str);
}

}
}
