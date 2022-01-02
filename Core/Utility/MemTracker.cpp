#include "Utility/MemTracker.h"
#include "Platform/PlatformGameAPI.h"
#include "DataStructures/HashMap.h"

#include <string.h>

namespace Tk
{
namespace Core
{
namespace Utility
{

struct MemRecord
{
    uint64 memPtr = 0;
    uint64 sizeInBytes = 0;
    uint8  bWasDeallocated = 0;

    bool operator==(const MemRecord& other) const
    {
        // Only check if memptr is same
        return (memPtr == other.memPtr);
    }
};

#define MAX_ALLOCS_RECORDED 1 << 24
struct MemTracker
{
    HashMap<uint64, MemRecord, Hash64> m_AllocRecords;
    uint8 bEnableAllocRecording = 0;

    MemTracker()
    {
        m_AllocRecords.Reserve(MAX_ALLOCS_RECORDED);
        bEnableAllocRecording = 1; // prevents this first actual map allocation from being recorded
    }

    ~MemTracker()
    {
        //TODO: move this?
        bEnableAllocRecording = 0;
        DebugOutputAllMemAllocs();
    }
};
static MemTracker g_MemTracker;

void RecordMemAlloc(uint64 sizeInBytes, void* memPtr)
{
    if (!g_MemTracker.bEnableAllocRecording)
        return;

    uint64 ptrAsU64 = (uint64)memPtr;

    MemRecord m;
    m.sizeInBytes = sizeInBytes;
    m.memPtr = ptrAsU64;
    m.bWasDeallocated = 0;
    g_MemTracker.m_AllocRecords.Insert(ptrAsU64, m);
}

void RecordMemDealloc(void* memPtr)
{
    if (!g_MemTracker.bEnableAllocRecording || !memPtr)
        return;

    //MemRecord m;
    //memset(&m, 0, sizeof(MemRecord));
    //m.memPtr = (uint64)memPtr;
    uint32 index = g_MemTracker.m_AllocRecords.FindIndex((uint64)memPtr);

    if (index == g_MemTracker.m_AllocRecords.eInvalidIndex)
    {
        // Memory not allocated yet
        TINKER_ASSERT(0);
    }
    else
    {
        MemRecord& m = g_MemTracker.m_AllocRecords.DataAtIndex(index);
        if (m.bWasDeallocated == 1)
        {
            // Double free
            TINKER_ASSERT(0);
        }
        else
        {
            m.bWasDeallocated = 1;
        }
    }
}

void DebugOutputAllMemAllocs()
{
    // TODO: get rid of this because you can't really track this perfectly due to destructor order not being guaranteed, but cool test

    Platform::PrintDebugString("***** Dumping all alloc records that were not deallocated *****\n");
    char buffer[512];
    for (uint32 i = 0; i < g_MemTracker.m_AllocRecords.Size(); ++i)
    {
        uint64 key = g_MemTracker.m_AllocRecords.KeyAtIndex(i);
        if (key == g_MemTracker.m_AllocRecords.GetInvalidKey())
            continue;

        const MemRecord& record = g_MemTracker.m_AllocRecords.DataAtIndex(i);
        if (!record.bWasDeallocated)
        {
            memset(buffer, 0, ARRAYCOUNT(buffer));

            // TODO: file/line

            // Alloc size
            _itoa_s((int)record.sizeInBytes, buffer, 10, 10);
            Platform::PrintDebugString("Alloc size: ");
            Platform::PrintDebugString(buffer);
            Platform::PrintDebugString("\n");
        }
    }
    Platform::PrintDebugString("********************\n");
}

}
}
}

