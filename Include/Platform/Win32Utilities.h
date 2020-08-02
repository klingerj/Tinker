#pragma once

#include "../Core/CoreDefines.h"
#include "../Include/Platform/Win32Logs.h"
#include <windows.h>

namespace Tinker
{
    namespace Platform
    {
        inline uint32 GetFileSize(const char* filename)
        {
            HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

            uint32 fileSize = 0;
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER fileSizeInBytes = {};
                if (GetFileSizeEx(fileHandle, &fileSizeInBytes))
                {
                    fileSize = SafeTruncateUint64(fileSizeInBytes.QuadPart);
                }
                else
                {
                    fileSize = 0;
                }
                CloseHandle(fileHandle);
                return fileSize;
            }
            else
            {
                LogMsg("Unable to create file handle!", eLogSeverityWarning);
                return 0;
            }
        }

        inline uint8* ReadEntireFile(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
        {
            // User must specify a file size and the dest buffer, or neither.
            TINKER_ASSERT((!fileSizeInBytes && !buffer) || (fileSizeInBytes && buffer));

            HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            uint8* fileDataBuffer = nullptr;

            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                uint32 fileSize = 0;
                if (fileSizeInBytes)
                {
                    fileSize = fileSizeInBytes;
                }
                else
                {
                    fileSize = GetFileSize(filename);
                }

                fileDataBuffer = buffer ? buffer : new uint8[fileSize];
                if (fileDataBuffer)
                {
                    DWORD numBytesRead = {};
                    ReadFile(fileHandle, fileDataBuffer, fileSize, &numBytesRead, 0);
                    TINKER_ASSERT(numBytesRead == fileSize);
                }
                else
                {
                    LogMsg("Unable to allocate buffer for reading file!", eLogSeverityCritical);
                }
                CloseHandle(fileHandle);
            }
            else
            {
                LogMsg("Unable to create file handle!", eLogSeverityWarning);
                return nullptr;
            }

            return fileDataBuffer;
        }
    }
}
