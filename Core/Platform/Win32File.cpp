#include "Platform/PlatformGameAPI.h"
#include "Utility/Logging.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Tk
{
namespace Platform
{

GET_ENTIRE_FILE_SIZE(GetEntireFileSize)
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
        Tk::Core::Utility::LogMsg("Platform", "Unable to create file handle!", Core::Utility::LogSeverity::eCritical);
        return 0;
    }
}

READ_ENTIRE_FILE(ReadEntireFile)
{
    // User must specify a file size and the dest buffer.
    TINKER_ASSERT(fileSizeInBytes && buffer);

    HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        uint32 fileSize = 0;
        if (fileSizeInBytes)
        {
            fileSize = fileSizeInBytes;
        }
        else
        {
            fileSize = Tk::Platform::GetEntireFileSize(filename);
        }

        DWORD numBytesRead = 0;
        ReadFile(fileHandle, buffer, fileSize, &numBytesRead, 0);
        TINKER_ASSERT(numBytesRead == fileSize);
        CloseHandle(fileHandle);
    }
    else
    {
        Tk::Core::Utility::LogMsg("Platform", "Unable to create file handle!", Core::Utility::LogSeverity::eCritical);
    }
}

WRITE_ENTIRE_FILE(WriteEntireFile)
{
    // User must specify a file size and the dest buffer.
    TINKER_ASSERT(fileSizeInBytes && buffer);

    HANDLE fileHandle = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD numBytesWritten = 0;
        WriteFile(fileHandle, buffer, fileSizeInBytes, &numBytesWritten, 0);
        TINKER_ASSERT(numBytesWritten == fileSizeInBytes);
        CloseHandle(fileHandle);
    }
    else
    {
        DWORD dw = GetLastError();
        Tk::Core::Utility::LogMsg("Platform", "Unable to create file handle!", Tk::Core::Utility::LogSeverity::eCritical);
    }
}

}
}
