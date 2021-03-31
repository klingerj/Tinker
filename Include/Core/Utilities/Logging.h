#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
    namespace Core
    {
        namespace Utility
        {
            enum
            {
                eLogSeverityInfo,
                eLogSeverityWarning,
                eLogSeverityCritical
            };

            void LogMsg(const char* prefix, const char* msg, uint32 severity);
        }
    }
}
