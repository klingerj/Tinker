#pragma once

#include "../Core/Math/VectorTypes.h"

namespace Tinker
{
    namespace Platform
    {
        namespace Graphics
        {
            enum BufferType
            {
                eVertexBuffer = 0,
                eIndexBuffer,
                eBufferTypeMax
            };

            typedef struct staging_buffer_data
            {
                uint32 handle;
                void* memory;
            } StagingBufferData;
        }
    }
}
