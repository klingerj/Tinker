#pragma once

#include "../Math/VectorTypes.h"

namespace Tinker
{
    namespace Core
    {
        namespace FileLoading
        {
            using namespace Math;

            // Returns the number of vertices in the obj file
            // Counts the number of rows that start with v
            uint32 GetOBJVertCount(uint8* entireFileBuffer);

            // Parse the OBJ file and populate existing vertex attribute buffers
            void ParseOBJ(v4f* dstPositionBuffer, v3f* dstNormalBuffer, v2f* dstUVBuffer, uint32* dstIndexBuffer, uint8* entireFileBuffer);
        }
    }
}
