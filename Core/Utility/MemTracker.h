#pragma once

#include "CoreDefines.h"
#include "DataStructures/HashMap.h"

namespace Tk { namespace Platform { struct StackTraceEntry; } } //namespace Tk

namespace Tk
{
  namespace Core
  {
    namespace Utility
    {
      // TODO: make this stuff thread-safe
      void RecordMemAlloc(size_t sizeInBytes, void* memPtr);
      void RecordMemDealloc(void* memPtr);
      void DebugOutputAllMemAllocs();

      struct MemTrackerStaticInitializer
      {
        MemTrackerStaticInitializer();
        ~MemTrackerStaticInitializer();
      };

// TODO: working this out, only initialize the mem tracker if
// building app. If/when they build together as single exe, this
// should still be safe because both TINKER_GAME and TINKER_APP
// will be defined.
// Probably also check if building game as dll or not.
#if defined(TINKER_APP)
      static MemTrackerStaticInitializer g_memTrackerStaticInitializer;
#endif

      // TODO: put this behind whatever #define will enable imgui/debug views.
      struct MemRecord
      {
        Tk::Platform::StackTraceEntry* firstStackTraceEntry = nullptr;
        uint64 memPtr = 0;
        uint64 sizeInBytes = 0;
        uint8 bWasDeallocated = 0;
      };

      using AllocRecordMap = HashMap<uint64, MemRecord, MapHashFn64>;
      TINKER_API const AllocRecordMap& GetAllAllocRecords();
    } //namespace Utility
  } //namespace Core
} //namespace Tk
