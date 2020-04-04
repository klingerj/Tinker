#include "VectorTypeBenchmarks.h"

void BM_v2Add_Scalar()
{
    v2f a = { 0.0f, 0.0f };
    v2f b = { 1.0f, 1.0f };

    for (uint32 i = 0; i < 1000000; ++i)
    {
        b = a + b;
        a += b;
    }
}

void BM_v2Add_Vectorized()
{
    // TODO: SSE/AVX version of adding vector types
}
