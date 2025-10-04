#include "PlatformGameAPI.h"
#include <windows.h>

namespace Tk
{
  namespace Platform
  {
    // I/O
    PRINT_DEBUG_STRING(PrintDebugString)
    {
      OutputDebugString(str);
    }

    ALLOC_ALIGNED_RAW(AllocAlignedRaw)
    {
      return _aligned_malloc(size, alignment);
    }

    FREE_ALIGNED_RAW(FreeAlignedRaw)
    {
      _aligned_free(ptr);
    }

#include <dbghelp.h>

    WALK_STACK_TRACE(WalkStackTrace)
    {
#ifndef ENABLE_MEM_TRACKING
      return 1;
#else

      SymSetOptions(SYMOPT_LOAD_LINES);

      DWORD machine = IMAGE_FILE_MACHINE_AMD64; // TODO check architecture?

      HANDLE process = GetCurrentProcess();
      HANDLE thread = GetCurrentThread();
      CONTEXT context = {};
      context.ContextFlags = CONTEXT_FULL;
      RtlCaptureContext(&context);

      STACKFRAME frame = {};
  #if _WIN32
      frame.AddrPC.Offset = context.Rip;
      frame.AddrPC.Mode = AddrModeFlat;
      frame.AddrFrame.Offset = context.Rbp;
      frame.AddrFrame.Mode = AddrModeFlat;
      frame.AddrStack.Offset = context.Rsp;
      frame.AddrStack.Mode = AddrModeFlat;
  #else
      frame.AddrPC.Offset = context.Eip;
      frame.AddrPC.Mode = AddrModeFlat;
      frame.AddrFrame.Offset = context.Ebp;
      frame.AddrFrame.Mode = AddrModeFlat;
      frame.AddrStack.Offset = context.Esp;
      frame.AddrStack.Mode = AddrModeFlat;
  #endif

      auto ProcessStackWalk = [&](StackTraceEntry* stackTraceEntry)
      {
        DWORD64 functionAddress = frame.AddrPC.Offset;

        DWORD64 moduleBase = SymGetModuleBase(process, frame.AddrPC.Offset);
        if (moduleBase)
        {
          char moduleScratchBuf[StackTraceEntry::MaxNameBufferSize];
          GetModuleFileNameA((HINSTANCE)moduleBase, moduleScratchBuf,
                             stackTraceEntry->MaxNameBufferSize);
          stackTraceEntry->moduleName.Append(&moduleScratchBuf[0]);
        }
        else
        {
          stackTraceEntry->moduleName.Append("InvalidModule");
        }

        char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + stackTraceEntry->MaxNameBufferSize];
        PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
        symbol->SizeOfStruct =
          sizeof(IMAGEHLP_SYMBOL) + stackTraceEntry->MaxNameBufferSize;
        symbol->MaxNameLength = stackTraceEntry->MaxNameBufferSize - 1;

        if (SymGetSymFromAddr(process, frame.AddrPC.Offset, NULL, symbol))
        {
          stackTraceEntry->functionName.Append(symbol->Name,
                                               uint32(strlen(symbol->Name)));
          stackTraceEntry->functionName.NullTerminate();
        }
        else
        {
          stackTraceEntry->functionName.Append("InvalidFunction");
        }

        DWORD offset = 0;
        IMAGEHLP_LINE line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

        if (SymGetLineFromAddr(process, frame.AddrPC.Offset, &offset, &line))
        {
          stackTraceEntry->fileName.Append(line.FileName, uint32(strlen(line.FileName)));
          stackTraceEntry->fileName.NullTerminate();
          stackTraceEntry->lineNum = line.LineNumber;
        }
        else
        {
          stackTraceEntry->fileName.Append("InvalidFile");
          stackTraceEntry->lineNum = 0;
        }
      };

      StackTraceEntry* currentStackFrame = (StackTraceEntry*)stackEntryAllocator.Alloc(
        sizeof(StackTraceEntry), alignof(StackTraceEntry));
      currentStackFrame->Init();
      if (StackWalk(machine, process, thread, &frame, &context, NULL,
                    SymFunctionTableAccess, SymGetModuleBase, NULL))
      {
        *topOfStack = currentStackFrame;
        ProcessStackWalk(currentStackFrame);
      }
      else
      {
        return 1;
      }

      while (StackWalk(machine, process, thread, &frame, &context, NULL,
                       SymFunctionTableAccess, SymGetModuleBase, NULL))
      {
        StackTraceEntry* newStackFrame = (StackTraceEntry*)stackEntryAllocator.Alloc(
          sizeof(StackTraceEntry), alignof(StackTraceEntry));
        newStackFrame->Init();
        currentStackFrame->next = newStackFrame;
        currentStackFrame = currentStackFrame->next;

        ProcessStackWalk(currentStackFrame);
      }

      return 0;

#endif
    }
  } //namespace Platform
} //namespace Tk
