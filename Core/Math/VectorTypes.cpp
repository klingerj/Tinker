#include "../../Include/Core/Math/VectorTypes.h"

#include <smmintrin.h>
#include <xmmintrin.h>

// mulps - SSE1
// mulloepi32 - SSE4.1

namespace VectorOps
{
    v2f Mul_SIMD(const v2f& v, const m2f& m)
    {
        __m128 vx = _mm_setr_ps(v.x, v.x, 0.0f, 0.0f);
        __m128 vy = _mm_setr_ps(v.y, v.y, 0.0f, 0.0f);
        __m128 mcol1 = _mm_setr_ps(m[0][0], m[0][1], 0.0f, 0.0f);
        __m128 mcol2 = _mm_setr_ps(m[1][0], m[1][1], 0.0f, 0.0f);

        __m128 xProd = _mm_mul_ps(mcol1, vx);
        __m128 yProd = _mm_mul_ps(mcol2, vy);
        
        __m128 sum = _mm_add_ps(xProd, yProd);
        
        const v2f result = *(v2f*)(&sum);
        return result;
    }

    v2i Mul_SIMD(const v2i& v, const m2i& m)
    {
        __m128i vx = _mm_setr_epi32(v.x, v.x, 0, 0);
        __m128i vy = _mm_setr_epi32(v.y, v.y, 0, 0);
        __m128i mcol1 = _mm_setr_epi32(m[0][0], m[0][1], 0, 0);
        __m128i mcol2 = _mm_setr_epi32(m[1][0], m[1][1], 0, 0);

        __m128i xProd = _mm_mullo_epi32(mcol1, vx);
        __m128i yProd = _mm_mullo_epi32(mcol2, vy);

        __m128i sum = _mm_add_epi32(xProd, yProd);

        const v2i result = *(v2i*)(&sum);
        return result;
    }

    v2ui Mul_SIMD(const v2ui& v, const m2ui& m)
    {
        __m128i vx = _mm_setr_epi32(v.x, v.x, 0, 0);
        __m128i vy = _mm_setr_epi32(v.y, v.y, 0, 0);
        __m128i mcol1 = _mm_setr_epi32(m[0][0], m[0][1], 0, 0);
        __m128i mcol2 = _mm_setr_epi32(m[1][0], m[1][1], 0, 0);

        __m128i xProd = _mm_mullo_epi32(mcol1, vx);
        __m128i yProd = _mm_mullo_epi32(mcol2, vy);

        __m128i sum = _mm_add_epi32(xProd, yProd);

        const v2ui result = *(v2ui*)(&sum);
        return result;
    }
}
