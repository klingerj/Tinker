#include "Utility/MemTracker.h"
#include "Platform/PlatformGameAPI.h"
#include "DataStructures/HashMap.h"
//#include "Allocators.h"
//#include "StringTypes.h"

#include <string.h>

namespace Tk
{
namespace Core
{
namespace Utility
{

Tk::Core::LinearAllocator StackTraceEntryAllocator;

struct MemRecord
{
    uint64 memPtr = 0;
    uint64 sizeInBytes = 0;
    uint8  bWasDeallocated = 0;
    
    Tk::Platform::StackTraceEntry* firstStackTraceEntry = nullptr;

    MemRecord() {}

    /*bool operator==(const MemRecord& other) const
    {
        // Only check if memptr is same
        return (memPtr == other.memPtr);
    }*/
};

#define MAX_ALLOCS_RECORDED 65536
struct MemTracker
{
    HashMap<uint64, MemRecord, MapHashFn64> m_AllocRecords;
    uint8 bEnableAllocRecording = 0;

    MemTracker()
    {
        #ifdef ENABLE_MEM_TRACKING
        m_AllocRecords.Reserve(MAX_ALLOCS_RECORDED);
        bEnableAllocRecording = 1; // prevents this first actual map allocation from being recorded

        // Have to manually allocate memory for the mem tracker since it'll call into the mem tracking code before it's fully initted :) 
        const uint32 allocatorSizeInBytes = 1024 * 1024 * 128;
        void* stackTraceAllocation = Tk::Platform::AllocAlignedRaw(allocatorSizeInBytes, CACHE_LINE);
        StackTraceEntryAllocator.Init(stackTraceAllocation, allocatorSizeInBytes); // TODO: use linked list of linear allocators ideally 
        #endif
    }

    ~MemTracker()
    {
#ifdef ENABLE_MEM_TRACKING
        bEnableAllocRecording = 0;
        DebugOutputAllMemAllocs();
        StackTraceEntryAllocator.ExplicitFree();
#endif
    }
};
static MemTracker g_MemTracker;

void RecordMemAlloc(uint64 sizeInBytes, void* memPtr)
{
    if (!g_MemTracker.bEnableAllocRecording)
    {
        return;
    }

    uint64 ptrAsU64 = (uint64)memPtr;

    // Grab stack trace of allocation
    Tk::Platform::StackTraceEntry* topOfStack = nullptr;
    uint32 error = Tk::Platform::WalkStackTrace(&topOfStack, StackTraceEntryAllocator);
    if (error != 0)
    {
        TINKER_ASSERT("Failed to get stack trace in mem tracker!");
    }

    // Finalize memory alloc record struct
    MemRecord m;
    m.sizeInBytes = sizeInBytes;
    m.memPtr = ptrAsU64;
    m.bWasDeallocated = 0;
    m.firstStackTraceEntry = topOfStack;
    g_MemTracker.m_AllocRecords.Insert(ptrAsU64, m);
}

void RecordMemDealloc(void* memPtr)
{
    if (!g_MemTracker.bEnableAllocRecording || !memPtr)
    {
        return;
    }

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

static uint32 LastIndexOf(const char pathDelimiter, const char* stringBuf, uint32 stringBufLen)
{
    uint32 finalDirIndex = 0;
    for (uint32 i = 0; i < stringBufLen; ++i)
    {
        if (stringBuf[i] == pathDelimiter)
        {
            finalDirIndex = i;
        }
    }

    return finalDirIndex;
}

#ifdef _ENGINE_ROOT_PATH
#define ENGINE_ROOT_PATH STRINGIFY(_ENGINE_ROOT_PATH)
#else
#define ENGINE_ROOT_PATH "./"
#endif
static const uint32 absolutePathPrefixLen = (uint32)strlen(ENGINE_ROOT_PATH);

static void ProcessStackTraceStrings(Tk::Platform::StackTraceEntry* stackTrace)
{
    for (uint32 i = 0; i < Tk::Platform::StackTraceEntry::MaxNameBufferSize; ++i)
    {
        if (stackTrace->moduleName[i] == '\\')
        {
            stackTrace->moduleName[i] = '/';
        } 
        if (stackTrace->functionName[i] == '\\')
        {
            stackTrace->functionName[i] = '/';
        }
        if (stackTrace->fileName[i] == '\\')
        {
            stackTrace->fileName[i] = '/';
        }
    }
}

void DebugOutputAllMemAllocs()
{
    // TODO: You can't really track deallocations perfectly due to destructor order not being guaranteed, but cool test

    Platform::PrintDebugString("\n***** MEMTRACKER: Dumping all alloc records *****\n\n"); //that were not deallocated
    for (uint32 i = 0; i < g_MemTracker.m_AllocRecords.Size(); ++i)
    {
        uint64 key = g_MemTracker.m_AllocRecords.KeyAtIndex(i);
        if (key == g_MemTracker.m_AllocRecords.GetInvalidKey())
            continue;

        const MemRecord& record = g_MemTracker.m_AllocRecords.DataAtIndex(i);
        //if (!record.bWasDeallocated)
        {
            //Platform::PrintDebugString(record.filename);
            Platform::PrintDebugString("Allocation:");

            // Allocation size 
            char buffer[256];
            memset(buffer, 0, ARRAYCOUNT(buffer));
            _itoa_s((int)record.sizeInBytes, buffer, ARRAYCOUNT(buffer), 10);
            Platform::PrintDebugString(buffer);
            Platform::PrintDebugString(" bytes");
            Platform::PrintDebugString("\n");
            
            Tk::Platform::StackTraceEntry* stackTrace = record.firstStackTraceEntry;
            Tk::Core::StrFixedBuffer<Tk::Platform::StackTraceEntry::MaxNameBufferSize> stackTracePrintBuffer;
            while (stackTrace)
            {
                stackTracePrintBuffer.Clear();
                ProcessStackTraceStrings(stackTrace);


                // Trim path off module name
                uint32 lastDirDelimiterIndex = LastIndexOf('/', stackTrace->moduleName.Data(), stackTrace->moduleName.Len());
                
                stackTracePrintBuffer.Append(stackTrace->moduleName.Data() + lastDirDelimiterIndex + 1);
                stackTracePrintBuffer.Append("!");
                stackTracePrintBuffer.Append(stackTrace->functionName.Data());
                stackTracePrintBuffer.Append(" in file: ");
                stackTracePrintBuffer.Append(stackTrace->fileName.Data() + absolutePathPrefixLen);
                stackTracePrintBuffer.Append(":");

                // Line number 
                memset(buffer, 0, ARRAYCOUNT(buffer));
                _itoa_s((int)stackTrace->lineNum, buffer, ARRAYCOUNT(buffer), 10);
                stackTracePrintBuffer.Append(buffer);


                stackTracePrintBuffer.NullTerminate();

                Platform::PrintDebugString(stackTracePrintBuffer.Data());
                Platform::PrintDebugString("\n");

                stackTrace = stackTrace->next;
            }
            Platform::PrintDebugString("\n");
        }
    }
    Platform::PrintDebugString("********************\n");
}

}
}
}
