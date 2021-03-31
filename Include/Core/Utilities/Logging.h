#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
namespace Core
{
namespace Utility
{

namespace LogSeverity
{
    enum : uint32
    {
        eInfo = 0,
        eWarning,
        eCritical
    };
}

void LogMsg(const char* prefix, const char* msg, uint32 severity);

}
}
}
