#pragma once

#include "Core/Math/VectorTypes.h"

namespace Tinker
{
    namespace Core
    {
        namespace FileLoading
        {
            using namespace Math;

            // This implementation of OBJ loading is only set up to properly load OBJ files exported from Autodesk Maya.
            // It only loads positions, normals, UVs, and indices.
            // It also assumes the mesh has been triangulated already!
            // It is generally free to crash or misbehave when loading a nonconforming OBJ file.

            // Returns the number of vertices in the obj file
            // Counts the number of rows that start with f, times 3 (assumes triangulated mesh)
            uint32 GetOBJVertCount(uint8* entireFileBuffer, uint32 bufferSizeInBytes);

            // Parse the OBJ file and populate existing vertex attribute buffers
            void ParseOBJ(v4f* dstPositionBuffer, v2f* dstUVBuffer, v3f* dstNormalBuffer, uint32* dstIndexBuffer,
                uint8* entireFileBuffer, uint32 bufferSizeInBytes);

            // Loading of various texture types
            #pragma pack(push, 1)
            // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader
            typedef struct bmp_header
            {
                uint16 type;
                uint32 fileSizeInBytes;
                int16 reserved1;
                int16 reserved2;
                uint32 offsetToBMPBytes;
            } BMPHeader;

            typedef struct bmp_info
            {
                uint32 structSize;
                int32 width;
                int32 height;
                uint16 planes;
                uint16 bitsPerPixel;
                uint32 compression;
                uint32 sizeInBytes;
                int32 xPPM;
                int32 yPPM;
                uint32 clrUsed;
                uint32 clrImp;
                struct rgb_quad
                {
                    uint8 b;
                    uint8 g;
                    uint8 r;
                    uint8 reserved;
                };
            } BMPInfo;
            #pragma pack(pop)

            BMPInfo GetBMPInfo(uint8* entireFileBuffer);
        }
    }
}

