#include <stdlib.h>

#include "Core/FileIO/FileLoading.h"

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

            bool LineStartsWith(uint8* buffer, uint32 currentIndex, const char* twoCharPrefix)
            {
                return buffer[currentIndex] == twoCharPrefix[0] && buffer[currentIndex + 1] == twoCharPrefix[1];
            }

            uint32 CountLinesWithPrefix(uint8* entireFileBuffer, uint32 bufferSizeInBytes, const char* twoCharPrefix)
            {
                // Counter gets advanced as we scan lines of the files
                uint32 currentIndex = 0;

                uint32 numLines = 0;
                while (1)
                {
                    scanLine(entireFileBuffer, &currentIndex);

                    if (currentIndex >= bufferSizeInBytes)
                    {
                        break;
                    }

                    if (LineStartsWith(entireFileBuffer, currentIndex, twoCharPrefix))
                    {
                        ++numLines;
                    }
                }
                return numLines;
            }

            uint32 GetOBJVertCount(uint8* entireFileBuffer, uint32 bufferSizeInBytes)
            {
                const char facePrefix[2] = { 'f', ' ' };
                return CountLinesWithPrefix(entireFileBuffer, bufferSizeInBytes, facePrefix) * 3;
            }

            void scanWord(uint8* buffer, uint32* currentIndex)
            {
                while (buffer[*currentIndex] != ' '  &&
                       buffer[*currentIndex] != '/'  &&
                       buffer[*currentIndex] != '\n' &&
                       buffer[*currentIndex] != '\r')
                {
                    ++*currentIndex;
                }
            }

            void scanWhiteSpace(uint8* buffer, uint32* currentIndex)
            {
                while (buffer[*currentIndex] == ' ' ||
                       buffer[*currentIndex] == '/')
                {
                    ++*currentIndex;
                }
            }

            void scanWordIntoBuffer(uint8* buffer, uint32* currentIndex, char* nextWord)
            {
                // Scan to the next word separated by white space
                scanWhiteSpace(buffer, currentIndex);

                // We are now on the first character of the word
                uint32 wordStartIndex = *currentIndex;

                // Scan to the end of the word
                scanWord(buffer, currentIndex);

                for (uint8 i = 0; i < 32; ++i)
                {
                    nextWord[i] = '\0';
                }
                for (uint32 uiChar = 0; wordStartIndex + uiChar < *currentIndex; ++uiChar)
                {
                    nextWord[uiChar] = buffer[wordStartIndex + uiChar];
                }
            }

            void ParseOBJ(v4f* dstPositionBuffer, v2f* dstUVBuffer, v3f* dstNormalBuffer, uint32* dstIndexBuffer,
                uint8* entireFileBuffer, uint32 bufferSizeInBytes)
            {
                TINKER_ASSERT(dstPositionBuffer);
                TINKER_ASSERT(dstNormalBuffer);
                TINKER_ASSERT(dstUVBuffer);
                TINKER_ASSERT(dstIndexBuffer);
                TINKER_ASSERT(entireFileBuffer);

                // Count the number of each attribute and allocate buffers to contain the data as we read it
                // TODO: count multiple prefixes at once rather than scanning the entire file for each
                // TODO: custom/faster allocation
                char prefix[2] = { 'v', ' ' };
                uint32 numPositionsInFile = CountLinesWithPrefix(entireFileBuffer, bufferSizeInBytes, prefix);
                prefix[0] = 'v';
                prefix[1] = 't';
                uint32 numUVsInFile = CountLinesWithPrefix(entireFileBuffer, bufferSizeInBytes, prefix);
                prefix[0] = 'v';
                prefix[1] = 'n';
                uint32 numNormalsInFile = CountLinesWithPrefix(entireFileBuffer, bufferSizeInBytes, prefix);
                uint8* attrReadBuffer = new uint8[sizeof(v4f) * numPositionsInFile + sizeof(v3f) * numNormalsInFile + sizeof(v2f) * numUVsInFile];
                v4f* posReadBuffer = (v4f*)attrReadBuffer;
                v2f* uvReadBuffer = (v2f*)((uint8*)posReadBuffer + sizeof(v4f) * numPositionsInFile);
                v3f* normReadBuffer = (v3f*)((uint8*)uvReadBuffer + sizeof(v2f) * numUVsInFile);

                // Count as we write values to each buffer
                uint32 posBufCtr = 0;
                uint32 normBufCtr = 0;
                uint32 uvBufCtr = 0;

                // Need global counter for indices
                uint32 indicesCounter = 0;

                // Counter gets advanced as we scan lines of the files
                uint32 currentIndex = 0;

                // Scan until we hit the null terminator which is used here to mark EOF
                while (entireFileBuffer[currentIndex] != '\0')
                {
                    if (entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == ' ')
                    {
                        // Vertex positions

                        // skip the 'v'
                        scanWord(entireFileBuffer, &currentIndex);

                        v4f newVertPos = v4f(0.0f, 0.0f, 0.0f, 1.0f);
                        const uint8 numWordsPerVert = 3;
                        for (uint8 uiWord = 0; uiWord < numWordsPerVert; ++uiWord)
                        {
                            char nextWord[32];
                            scanWordIntoBuffer(entireFileBuffer, &currentIndex, nextWord);
                            newVertPos[uiWord] = (float)atof(nextWord);
                        }

                        posReadBuffer[posBufCtr++] = newVertPos;
                    }
                    else if (entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == 't')
                    {
                        // Vertex texture coordinates

                        // skip the 'vt'
                        scanWord(entireFileBuffer, &currentIndex);

                        v2f newVertUV = v2f(0.0f, 0.0f);
                        const uint8 numWordsPerVert = 2;
                        for (uint8 uiWord = 0; uiWord < numWordsPerVert; ++uiWord)
                        {
                            char nextWord[32];
                            scanWordIntoBuffer(entireFileBuffer, &currentIndex, nextWord);
                            newVertUV[uiWord] = (float)atof(nextWord);
                        }

                        uvReadBuffer[uvBufCtr++] = newVertUV;
                    }
                    else if (entireFileBuffer[currentIndex] == 'v' && entireFileBuffer[currentIndex + 1] == 'n')
                    {
                        // Vertex normals

                        // skip the 'vn'
                        scanWord(entireFileBuffer, &currentIndex);

                        v3f newVertNormal = v3f(0.0f, 0.0f, 0.0f);
                        const uint8 numWordsPerVert = 3;
                        for (uint8 uiWord = 0; uiWord < numWordsPerVert; ++uiWord)
                        {
                            char nextWord[32];
                            scanWordIntoBuffer(entireFileBuffer, &currentIndex, nextWord);
                            newVertNormal[uiWord] = (float)atof(nextWord);
                        }

                        normReadBuffer[normBufCtr++] = newVertNormal;
                    }
                    else if (entireFileBuffer[currentIndex] == 'f' && entireFileBuffer[currentIndex + 1] == ' ')
                    {
                        // Indices

                        // skip the 'f'
                        scanWord(entireFileBuffer, &currentIndex);

                        const uint8 numWordsPerFace = 3;
                        for (uint8 uiWord = 0; uiWord < numWordsPerFace; ++uiWord)
                        {
                            const uint8 numIndicesPerFace = 3;
                            uint32 newIndices[numIndicesPerFace];

                            // NOTE: '/' characters are treated as white space when scanning chars
                            char nextWord[32];

                            scanWordIntoBuffer(entireFileBuffer, &currentIndex, nextWord);
                            newIndices[0] = (uint32)atoi(nextWord);

                            scanWordIntoBuffer(entireFileBuffer, &currentIndex, nextWord);
                            newIndices[1] = (uint32)atoi(nextWord);

                            scanWordIntoBuffer(entireFileBuffer, &currentIndex, nextWord);
                            newIndices[2] = (uint32)atoi(nextWord);

                            // Normalize indices to start from 0, OBJ convention is to start from 1
                            for (uint8 i = 0; i < numIndicesPerFace; ++i)
                            {
                                --newIndices[i];
                            }

                            // reach into the given attribute buffers and write them to the dst buffers
                            dstPositionBuffer[indicesCounter] = posReadBuffer[newIndices[0]];
                            dstUVBuffer[indicesCounter] = uvReadBuffer[newIndices[1]];
                            dstNormalBuffer[indicesCounter] = normReadBuffer[newIndices[2]];
                            dstIndexBuffer[indicesCounter] = indicesCounter;

                            ++indicesCounter;
                        }
                    }
                    else
                    {
                        // Proceed to next line
                        scanLine(entireFileBuffer, &currentIndex);
                    }
                }

                delete attrReadBuffer;
            }

            BMPInfo GetBMPInfo(uint8* entireFileBuffer)
            {
                // NOTE: assumes the buffer is a well-formed bmp file
                // Skips the file header, which is immediately followed by the bmp info
                return *(BMPInfo*)(entireFileBuffer + sizeof(BMPHeader));
            }
        }
    }
}
