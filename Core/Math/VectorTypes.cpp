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
        
        return *(v2f*)(&sum);
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

        return *(v2i*)(&sum);
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

        return *(v2ui*)(&sum);
    }

    void Mul_SIMD(const v4f& v, const m4f& m, v4f& out)
    {
        __m128 vec = _mm_load_ps(v.m_data);
        __m128 vx = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vy = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vz = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 vw = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 mcol1 = _mm_load_ps(m[0].m_data);
        __m128 mcol2 = _mm_load_ps(m[1].m_data);
        __m128 mcol3 = _mm_load_ps(m[2].m_data);
        __m128 mcol4 = _mm_load_ps(m[3].m_data);

        __m128 xProd = _mm_mul_ps(mcol1, vx);
        __m128 yProd = _mm_mul_ps(mcol2, vy);
        __m128 zProd = _mm_mul_ps(mcol3, vz);
        __m128 wProd = _mm_mul_ps(mcol4, vw);

        __m128 sum1 = _mm_add_ps(xProd, yProd);
        __m128 sum2 = _mm_add_ps(zProd, wProd);
        __m128 sum  = _mm_add_ps(sum1, sum2);

        _mm_stream_ps((float*)&out, sum);
        //_mm_sfence();
    }

    v4i Mul_SIMD(const v4i& v, const m4i& m)
    {
        __m128i vx = _mm_setr_epi32(v.x, v.x, v.x, v.x);
        __m128i vy = _mm_setr_epi32(v.y, v.y, v.y, v.y);
        __m128i vz = _mm_setr_epi32(v.z, v.z, v.z, v.z);
        __m128i vw = _mm_setr_epi32(v.w, v.w, v.w, v.w);
        __m128i mcol1 = _mm_setr_epi32(m[0][0], m[0][1], m[0][2], m[0][3]);
        __m128i mcol2 = _mm_setr_epi32(m[1][0], m[1][1], m[1][2], m[1][3]);
        __m128i mcol3 = _mm_setr_epi32(m[2][0], m[2][1], m[2][2], m[2][3]);
        __m128i mcol4 = _mm_setr_epi32(m[3][0], m[3][1], m[3][2], m[3][3]);

        __m128i xProd = _mm_mullo_epi32(mcol1, vx);
        __m128i yProd = _mm_mullo_epi32(mcol2, vy);
        __m128i zProd = _mm_mullo_epi32(mcol3, vz);
        __m128i wProd = _mm_mullo_epi32(mcol4, vw);

        __m128i sum1 = _mm_add_epi32(xProd, yProd);
        __m128i sum2 = _mm_add_epi32(zProd, wProd);
        __m128i sum  = _mm_add_epi32(sum1, sum2);

        return *(v4i*)(&sum);
    }

    v4ui Mul_SIMD(const v4ui& v, const m4ui& m)
    {
        __m128i vx = _mm_setr_epi32(v.x, v.x, v.x, v.x);
        __m128i vy = _mm_setr_epi32(v.y, v.y, v.y, v.y);
        __m128i vz = _mm_setr_epi32(v.z, v.z, v.z, v.z);
        __m128i vw = _mm_setr_epi32(v.w, v.w, v.w, v.w);
        __m128i mcol1 = _mm_setr_epi32(m[0][0], m[0][1], m[0][2], m[0][3]);
        __m128i mcol2 = _mm_setr_epi32(m[1][0], m[1][1], m[1][2], m[1][3]);
        __m128i mcol3 = _mm_setr_epi32(m[2][0], m[2][1], m[2][2], m[2][3]);
        __m128i mcol4 = _mm_setr_epi32(m[3][0], m[3][1], m[3][2], m[3][3]);

        __m128i xProd = _mm_mullo_epi32(mcol1, vx);
        __m128i yProd = _mm_mullo_epi32(mcol2, vy);
        __m128i zProd = _mm_mullo_epi32(mcol3, vz);
        __m128i wProd = _mm_mullo_epi32(mcol4, vw);

        __m128i sum1 = _mm_add_epi32(xProd, yProd);
        __m128i sum2 = _mm_add_epi32(zProd, wProd);
        __m128i sum  = _mm_add_epi32(sum1, sum2);

        return *(v4ui*)(&sum);
    }
}
