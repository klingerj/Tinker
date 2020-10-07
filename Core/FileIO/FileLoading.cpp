#include "../../Include/Core/FileIO/FileLoading.h"

namespace Tinker
{
    namespace Core
    {
        namespace FileLoading
        {
            using namespace Math;
            uint32 GetOBJVertCount(void* entireFileBuffer)
            {
                return 0;
            }

            void ParseOBJ(v4f* dstPositionBuffer, v3f* dstNormalBuffer, v2f* dstUVBuffer, uint32* dstIndexBuffer, void* entireFileBuffer)
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

