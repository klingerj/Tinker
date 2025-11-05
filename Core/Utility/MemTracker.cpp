#include "Utility/MemTracker.h"
#include "DataStructures/HashMap.h"
#include "Platform/PlatformGameAPI.h"
//#include "Allocators.h"
//#include "StringTypes.h"

#include <string.h>

namespace Tk
{
  namespace Core
  {
    namespace Utility
    {

      struct MemRecord
      {
        Tk::Platform::StackTraceEntry* firstStackTraceEntry = nullptr;
        uint64 memPtr = 0;
        uint64 sizeInBytes = 0;
        uint8 bWasDeallocated = 0;
      };

#define MAX_ALLOCS_RECORDED 65'536

      struct MemTracker
      {
        HashMap<uint64, MemRecord, MapHashFn64> m_AllocRecords;
        Tk::Core::LinearAllocator m_stackTraceEntryAllocator;
        bool bEnableAllocRecording = false;

        MemTracker()
        {
          m_AllocRecords.Reserve(MAX_ALLOCS_RECORDED);

          // Have to manually allocate memory for the mem tracker since it'll call into
          // the mem tracking code before it's fully initted :)
          const uint32 allocatorSizeInBytes = 1024 * 1024 * 128;
          void* stackTraceAllocation = CoreMallocAligned(allocatorSizeInBytes, CACHE_LINE);
          m_stackTraceEntryAllocator.Init(
            stackTraceAllocation,
            allocatorSizeInBytes); // TODO: use linked list of linear allocators ideally

          // Placing this after prevents allocations that this class owns from 
          // being tracked, because it would try to record allocations into 
          // the not-yet-allocated structure.
          bEnableAllocRecording = true;
        }

        ~MemTracker()
        {
          bEnableAllocRecording = false;
          DebugOutputAllMemAllocs();
          m_stackTraceEntryAllocator.ExplicitFree();
        }
      };

      alignas(MemTracker) static std::byte mem_tracker_buffer[sizeof(MemTracker)];
      MemTracker& g_MemTracker = reinterpret_cast<MemTracker&>(mem_tracker_buffer);
      static size_t niftyCounter = 0;
      MemTrackerStaticInitializer::MemTrackerStaticInitializer()
      {
        if (niftyCounter++ == 0)
        {
          new (&g_MemTracker) MemTracker();
        }
      }

      MemTrackerStaticInitializer::~MemTrackerStaticInitializer()
      {
        if (--niftyCounter == 0)
        {
          g_MemTracker.~MemTracker();
        }
      }

      void RecordMemAlloc(size_t sizeInBytes, void* memPtr)
      {
        if (g_MemTracker.bEnableAllocRecording == false)
        {
          return;
        }

        size_t ptrAsU64 = reinterpret_cast<size_t>(memPtr);

        // Grab stack trace of allocation
        Tk::Platform::StackTraceEntry* topOfStack = nullptr;
        uint32 error =
          Tk::Platform::WalkStackTrace(&topOfStack, g_MemTracker.m_stackTraceEntryAllocator);
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
        if (g_MemTracker.bEnableAllocRecording == false || memPtr == nullptr)
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

      static uint32 LastIndexOf(const char pathDelimiter, const char* stringBuf,
                                uint32 stringBufLen)
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
        const size_t numRecords = g_MemTracker.m_AllocRecords.Size();
        if (numRecords > 0)
        {
          Platform::PrintDebugString(
            "\n***** MEMTRACKER: Dumping all alloc records *****\n\n");
        }
        else
        {
          Platform::PrintDebugString(
            "\n***** MEMTRACKER: 0 alloc records. *****\n\n");
        }

        // Currently tries hashing every index to see if it has a valid key.
        for (size_t i = 0; i < g_MemTracker.m_AllocRecords.Capacity(); ++i)
        {
          uint64 key = g_MemTracker.m_AllocRecords.KeyAtIndex(static_cast<uint32>(i));
          if (key == g_MemTracker.m_AllocRecords.GetInvalidKey())
          {
            continue;
          }

          const MemRecord& record = g_MemTracker.m_AllocRecords.DataAtIndex(static_cast<uint32>(i));
          if (!record.bWasDeallocated)
          {
            Platform::PrintDebugString("Allocation: ");

            // Allocation size
            // TODO don't use an array like this?
            char buffer[256];
            memset(buffer, 0, ARRAYCOUNT(buffer));
            _itoa_s((int)record.sizeInBytes, buffer, ARRAYCOUNT(buffer), 10);
            Platform::PrintDebugString(buffer);
            Platform::PrintDebugString(" bytes");
            Platform::PrintDebugString("\n");

            Tk::Platform::StackTraceEntry* stackTrace = record.firstStackTraceEntry;
            Tk::Core::StrFixedBuffer<Tk::Platform::StackTraceEntry::MaxNameBufferSize>
              stackPrintBuf;
            while (stackTrace)
            {
              stackPrintBuf.Clear();
              ProcessStackTraceStrings(stackTrace);

              // Trim path off module name
              uint32 lastDirDelimiterIndex = LastIndexOf(
                '/', stackTrace->moduleName.Data(), stackTrace->moduleName.Len());

              stackPrintBuf.Append(
                stackTrace->moduleName.Data() + lastDirDelimiterIndex + 1);
              stackPrintBuf.Append("!");
              stackPrintBuf.Append(stackTrace->functionName.Data());
              stackPrintBuf.Append(" in file: ");
              stackPrintBuf.Append(stackTrace->fileName.Data() + absolutePathPrefixLen);
              stackPrintBuf.Append(":");

              // Line number
              memset(buffer, 0, ARRAYCOUNT(buffer));
              _itoa_s((int)stackTrace->lineNum, buffer, ARRAYCOUNT(buffer), 10);
              stackPrintBuf.Append(buffer);

              stackPrintBuf.Append("\n");
              stackPrintBuf.NullTerminate();
              Platform::PrintDebugString(stackPrintBuf.Data());

              stackTrace = stackTrace->next;
            }
            Platform::PrintDebugString("\n");

          }
        }
        Platform::PrintDebugString("********************\n");
      }
    } //namespace Utility
  } //namespace Core
} //namespace Tk
