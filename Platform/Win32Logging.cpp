#include "../Include/Core/Utilities/Logging.h"

#include <windows.h>

namespace Tinker
{
    namespace Utility
    {
        void LogMsg(const char* prefix, const char* msg, uint32 severity)
        {
            OutputDebugString("[");
            OutputDebugString(prefix);
            OutputDebugString("]");

            const char* severityMsg;
            switch (severity)
            {
                case eLogSeverityInfo:
                {
                    severityMsg = "Info";
                    break;
                }
                case eLogSeverityWarning:
                {
                    severityMsg = "Warning";
                    break;
                }
                case eLogSeverityCritical:
                {
                    severityMsg = "Critical";
                    break;
                }
                default:
                {
                    severityMsg = "Unknown Severity";
                    break;
                }
            }
            OutputDebugString("[");
            OutputDebugString(severityMsg);
            OutputDebugString("] ");

            OutputDebugString(msg);
            OutputDebugString("\n");
        }
    }
}
