#pragma once

#include "Core/CoreDefines.h"

#define MAX_RECORDS MAX_UINT16

namespace Tinker
{
namespace Core
{
namespace Utility
{

// TODO: make this stuff thread-safe
// TODO: remove alloc records when they are dealloc'd
void RecordMemAlloc(uint32 sizeInBytes, void* memPtr);
void RecordMemDealloc(void* memPtr);
void DebugOutputAllMemAllocs();

struct MemRecord
{
    uint32 sizeInBytes;
    size_t memPtr;
    uint8  bWasDeallocated;

    bool operator==(const MemRecord& other) const
    {
        // If this record was deallocated, it cannot be equal to another, aka not found by Vector::Find()
        // THIS IS TEMPORARY
        return (memPtr == other.memPtr) && (!bWasDeallocated);
    }
};

// TODO: replace with map structure 
struct MemTracker
{
    MemRecord* m_records;
    uint32 m_numRecords;

    MemTracker()
    {
        m_records = (MemRecord*)malloc(sizeof(MemRecord) * MAX_RECORDS);
        m_numRecords = 0;
    }

    ~MemTracker()
    {
        //TODO: move this
        DebugOutputAllMemAllocs();
    }
};

extern MemTracker g_MemTracker;

}
}
}
