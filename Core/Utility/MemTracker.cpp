#include "Core/Utility/MemTracker.h"
#include "PlatformGameAPI.h"

#include <string.h>

namespace Tk
{
namespace Core
{
namespace Utility
{

MemTracker g_MemTracker;

void RecordMemAlloc(uint32 sizeInBytes, void* memPtr)
{
    MemRecord m;
    m.sizeInBytes = sizeInBytes;
    m.memPtr = (size_t)memPtr;
    m.bWasDeallocated = false;
    memcpy(&g_MemTracker.m_records[g_MemTracker.m_numRecords++], &m, sizeof(MemRecord));
}

uint32 FindRecord(const MemRecord& m)
{
    for (uint32 i = 0; i < g_MemTracker.m_numRecords; ++i)
    {
        if (g_MemTracker.m_records[i] == m)
        {
            return i;
        }
    }

    return MAX_RECORDS;
}

void RecordMemDealloc(void* memPtr)
{
    MemRecord m;
    memset(&m, 0, sizeof(MemRecord));
    m.memPtr = (size_t)memPtr;
    uint32 index = FindRecord(m);

    if (index == MAX_RECORDS)
    {
        // Memory not allocated yet, or double free
    }
    else
    {
        g_MemTracker.m_records[index].bWasDeallocated = true;
    }
}

void DebugOutputAllMemAllocs()
{
    if (g_MemTracker.m_numRecords == 0)
    {
        return;
    }

    Platform::PrintDebugString("***** Dumping all alloc records that were not deallocated *****\n");
    char buffer[512];
    for (uint32 i = 0; i < g_MemTracker.m_numRecords; ++i)
    {
        const MemRecord& record = g_MemTracker.m_records[i];
        if (record.bWasDeallocated) continue;

        memset(buffer, 0, ARRAYCOUNT(buffer));
        
        // TODO: alloc file/line

        // Alloc size
        _itoa_s(record.sizeInBytes, buffer, 10, 10);
        Platform::PrintDebugString("Alloc size: ");
        Platform::PrintDebugString(buffer);

        //-----
        Platform::PrintDebugString("\n");
    }
    Platform::PrintDebugString("********************\n");
}

}
}
}

