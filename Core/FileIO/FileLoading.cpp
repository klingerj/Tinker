#include "../../Include/Core/FileIO/FileLoading.h"

namespace Tinker
{
    namespace Core
    {
        namespace FileLoading
        {
            using namespace Math;

            void scanLine(uint8* buffer, uint32* currentIndex)
            {
                startScan:
                while (buffer[*currentIndex] != '\n' && buffer[*currentIndex] != '\r')
                {
                    ++*currentIndex;
                }
                ++*currentIndex; // move one past the newline

                // Consume all consecutive '\n' and '\r' characters
                if (buffer[*currentIndex] == '\n' || buffer[*currentIndex] == '\r')
                {
                    goto startScan;
                }
            }

            uint32 countVertLines(uint8* fileBuffer, uint32* currentIndex)
            {
                uint32 numVertices = 0;

                while (fileBuffer[*currentIndex] == 'v' && fileBuffer[*currentIndex + 1] == ' ')
                {
                    scanLine(fileBuffer, currentIndex);
                    ++numVertices;
                }

                return numVertices;
            }

            uint32 GetOBJVertCount(uint8* entireFileBuffer)
            {
                // Counter gets advanced as we scan lines of the files
                uint32 currentIndex = 0;

                // If first line starts with "v ", start reading lines
                if (entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == ' ')
                {
                    return countVertLines(entireFileBuffer, &currentIndex);
                }
                else
                {
                    // Skip lines until we find one that starts with "v "
                    while (!(entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == ' '))
                    {
                        scanLine(entireFileBuffer, &currentIndex);
                    }

                    // Start reading lines
                    return countVertLines(entireFileBuffer, &currentIndex);
                }
            }

            void ParseOBJ(v4f* dstPositionBuffer, v3f* dstNormalBuffer, v2f* dstUVBuffer, uint32* dstIndexBuffer, uint8* entireFileBuffer)
            {
                while (0)
                {
                    // TODO: Read the file data line by line and parse the data
                }

                if (dstPositionBuffer)
                {
                    // write positions
                }

                if (dstNormalBuffer)
                {
                    // write normals
                }

                if (dstUVBuffer)
                {
                    // write uvs
                }

                if (dstIndexBuffer)
                {
                    // write indices
                }
            }
        }
    }
}

