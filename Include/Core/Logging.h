#pragma once

#include "../Core/CoreDefines.h"

enum
{
    eLogSeverityInfo,
    eLogSeverityWarning,
    eLogSeverityCritical
};

void LogMsg(const char* msg, uint32 severity);
