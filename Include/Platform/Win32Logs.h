#pragma once

#include <windows.h>

enum
{
    eLogSeverityInfo,
    eLogSeverityWarning,
    eLogSeverityCritical
};

inline void LogMsg(const char* msg, uint32 severity)
{
    OutputDebugString("[Platform] ");

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

