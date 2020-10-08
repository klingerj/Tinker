#pragma once

#include "../Math/VectorTypes.h"

// This implementation of OBJ loading is only set up to properly load OBJ files exported from Autodesk Maya.
// It only loads positions, normals, UVs, and indices.
// It also assumes the mesh has been triangulated already!
// It is generally free to crash or misbehave when loading a nonconforming OBJ file.

namespace Tinker
{
    namespace Core
    {
        namespace FileLoading
        {
            using namespace Math;

            // Returns the number of vertices in the obj file
            // Counts the number of rows that start with f, times 3 (assumes triangulated mesh)
            uint32 GetOBJVertCount(uint8* entireFileBuffer, uint32 bufferSizeInBytes);

            // Parse the OBJ file and populate existing vertex attribute buffers
            void ParseOBJ(v4f* dstPositionBuffer, v2f* dstUVBuffer, v3f* dstNormalBuffer, uint32* dstIndexBuffer,
                uint8* entireFileBuffer, uint32 bufferSizeInBytes);
        }
    }
}

