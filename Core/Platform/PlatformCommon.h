#pragma once

#ifdef _WIN32
#include <windows.h>
#else
// TODO: other platform headers
#endif

namespace Tk
{
namespace Platform
{

struct PlatformWindowHandles
{
    #ifdef _WIN32
    HINSTANCE instance;
    HWND windowHandle;
    #endif
    // TODO: other platform window handles

};

}
}
