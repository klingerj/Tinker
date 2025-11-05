#pragma once

#include "CoreDefines.h"

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

    } //namespace Utility
  } //namespace Core
} //namespace Tk
