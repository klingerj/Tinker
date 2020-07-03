#pragma once

#include "../Core/CoreDefines.h"
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
                // File doesn't exist - fail?
                return 0;
            }
        }

        inline uint8* ReadEntireFile(const char* filename, uint32 fileSizeInBytes, uint8* buffer)
        {
            // User must specify a file size and the dest buffer, or neither.
            TINKER_ASSERT((!fileSizeInBytes && !buffer) || (fileSizeInBytes && buffer));

            HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            uint8* fileDataBuffer = NULL;

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
                    //TODO: Fail?
                }
                CloseHandle(fileHandle);
            }
            else
            {
                // File doesn't exist - fail?
                return NULL;
            }

            return fileDataBuffer;
        }
    }
}
