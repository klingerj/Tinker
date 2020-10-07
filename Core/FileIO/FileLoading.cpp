#include <stdlib.h>

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
                    if (buffer[*currentIndex] == '\0') return; // marks EOF

                    ++*currentIndex;
                }

                ++*currentIndex; // move one past the newline

                // Consume all consecutive '\n' and '\r' characters
                if (buffer[*currentIndex] == '\n' || buffer[*currentIndex] == '\r')
                {
                    goto startScan;
                }
            }

            uint32 GetOBJVertCount(uint8* entireFileBuffer, uint32 bufferSizeInBytes)
            {
                // Counter gets advanced as we scan lines of the files
                uint32 currentIndex = 0;

                uint32 numFaces = 0;

                while (1)
                {
                    scanLine(entireFileBuffer, &currentIndex);

                    if (currentIndex >= bufferSizeInBytes)
                    {
                        break;
                    }

                    if (entireFileBuffer[currentIndex] == 'f' && entireFileBuffer[currentIndex + 1] == ' ')
                    {
                        ++numFaces;
                    }
                }

                return numFaces * 3;
            }

            void scanWord(uint8* buffer, uint32* currentIndex)
            {
                while (buffer[*currentIndex] != ' ' && buffer[*currentIndex] != '\n' && buffer[*currentIndex]!= '\r')
                {
                    ++*currentIndex;
                }
            }

            void scanWhiteSpace(uint8* buffer, uint32* currentIndex)
            {
                while (buffer[*currentIndex] == ' ')
                {
                    ++*currentIndex;
                }
            }

            void ParseOBJ(v4f* dstPositionBuffer, v3f* dstNormalBuffer, v2f* dstUVBuffer, uint32* dstIndexBuffer, uint8* entireFileBuffer)
            {
                TINKER_ASSERT(dstPositionBuffer);
                TINKER_ASSERT(dstNormalBuffer);
                //TINKER_ASSERT(dstUVBuffer);
                TINKER_ASSERT(dstIndexBuffer);
                TINKER_ASSERT(entireFileBuffer);

                uint32 currentIndex = 0;

                // Scan for vertex positions
                if (entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == ' ')
                {
                    v4f vertPosToWrite = v4f(0.0f, 0.0f, 0.0f, 1.0f);
                    scanWord(entireFileBuffer, &currentIndex);

                    for (uint8 uiWord = 0; uiWord < 3; ++uiWord)
                    {
                        // Scan to the next word separated by white space
                        scanWhiteSpace(entireFileBuffer, &currentIndex);

                        // We are now on the first character of the word
                        uint32 wordStartIndex = currentIndex;

                        // Scan to the end of the word
                        scanWord(entireFileBuffer, &currentIndex);

                        // Init buffer for word
                        char wordBuffer[32]; // buffer to be converted from characters to a float
                        for (uint8 i = 0; i < 32; ++i)
                        {
                            wordBuffer[i] = '\0';
                        }
                        
                        // Copy word into buffer to get parsed to a float
                        for (uint32 uiChar = 0; wordStartIndex + uiChar < currentIndex; ++uiChar)
                        {
                            wordBuffer[uiChar] = entireFileBuffer[wordStartIndex + uiChar];
                        }
                        vertPosToWrite[uiWord] = (float)atof(wordBuffer);
                    }

                    // Scan to beginning of next line
                    scanLine(entireFileBuffer, &currentIndex);
                }
                else
                {
                    // Skip lines until we find one that starts with "v "
                    while (!(entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == ' '))
                    {
                        scanLine(entireFileBuffer, &currentIndex);
                    }

                    // Start reading lines
                }
            }
        }
    }
}

