#include "VectorTypeBenchmarks.h"
#include "Platform/PlatformCommon.h"

#include "Platform/Win32WorkerThreadPool.h"
#include <emmintrin.h>

using namespace Tk;
using namespace Platform;
using namespace Core;

const uint32 threadCount = 6;

const uint32 numJobs = threadCount;
WorkerJob* jobs[numJobs];

v2f* g_v2s = nullptr;
v4f* g_v4s = nullptr;
v4f* g_v4s_dst = nullptr;
const uint32 jobSize = 16 * 2;
const uint32 numVectors = numJobs * jobSize;
const uint32 numIters = 10000000;

void BM_v2_Startup()
{
    g_v2s = (v2f*)Tk::Platform::AllocAlignedRaw(numVectors * sizeof(v2f), CACHE_LINE);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v2s[i] = v2f((float)i, (float)(i + 1));
    }
}

void BM_v2_Shutdown()
{
    Tk::Platform::FreeAlignedRaw(g_v2s);
    g_v2s = nullptr;
}

void BM_v2Add_Scalar()
{
    v2f a = { 0.0f, 0.0f };
    v2f b = { 1.0f, 1.0f };

    for (uint32 i = 0; i < 10000000; ++i)
    {
        b = a + b;
        a += b;
    }
}

void BM_v2Add_Vectorized()
{
    // TODO: SSE/AVX version of adding vector types
}

void BM_m2MulV2_iScalar()
{
    v2i v = { 4, 5 };
    for (uint32 i = 0; i < 10000000; ++i)
    {
        m2i m = { (int32)i, (int32)i + 1, (int32)i + 2, (int32)i + 3 };
        v = m * v;
    }
}

void BM_m2MulV2_iVectorized()
{
    alignas(16) v2i v = { 4, 5 };
    for (uint32 i = 0; i < 10000000; ++i)
    {
        m2i m = { (int32)i, (int32)i + 1, (int32)i + 2, (int32)i + 3 };
        VectorOps::Mul_SIMD(&v, &m, &v);
    }
}

void BM_m2MulV2_fScalar()
{
    v2f* const vectors = g_v2s;
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

    for (uint32 iter = 0; iter < numIters; ++iter)
    {
        for (uint32 i = 0; i < numVectors; ++i)
        {
            vectors[i] = m * vectors[i];
        }
    }
}

void BM_m2MulV2_fScalar_MT_Startup()
{
    ThreadPool::Startup(threadCount);
    BM_v2_Startup();
}

void BM_m2MulV2_fScalar_MT()
{
    v2f* const vectors = g_v2s;
    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i] = CreateNewThreadJob([=]() {
            const m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

            for (uint32 iter = 0; iter < numIters; ++iter)
            {
                for (uint32 j = 0; j < jobSize; ++j)
                {
                    uint32 index = j + i * jobSize;
                    vectors[index] = m * vectors[index];
                }
            }
        });
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        ThreadPool::EnqueueSingleJob(jobs[i]);
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }
}

void BM_m2MulV2_fScalar_MT_Shutdown()
{
    ThreadPool::Shutdown();
    BM_v2_Shutdown();
    for (uint32 i = 0; i < numJobs; ++i)
    {
        Tk::Platform::FreeAlignedRaw(jobs[i]);
    }
}

void BM_m2MulV2_fVectorized()
{
    v2f* const vectors = g_v2s;
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

    for (uint32 i = 0; i < numVectors; ++i)
    {
        // NOTE: bad, shouldn't write to the original matrix with simd
        //VectorOps::Mul_SIMD(&vectors[i], &m, &vectors[i]);
    }
}

void BM_v4_Startup()
{
    g_v4s = (v4f*)Tk::Platform::AllocAlignedRaw(numVectors * sizeof(v4f), CACHE_LINE);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v4s[i] = v4f((float)i, (float)(i + 1),(float)(i + 2), (float)(i + 3));
    }

    g_v4s_dst = (v4f*)Tk::Platform::AllocAlignedRaw(numVectors * sizeof(v4f), CACHE_LINE);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v4s_dst[i] = v4f((float)i, (float)(i + 1), (float)(i + 2), (float)(i + 3));
    }
}

void BM_v4_Shutdown()
{
    Tk::Platform::FreeAlignedRaw(g_v4s);
    g_v4s = nullptr;
    Tk::Platform::FreeAlignedRaw(g_v4s_dst);
    g_v4s_dst = nullptr;
}

void BM_v4_MT_Startup()
{
    ThreadPool::Startup(threadCount - 1);
    BM_v4_Startup();
}

void BM_m4MulV4_fScalar_MT()
{
    for (uint32 i = 0; i < numJobs; ++i)
    {
        uint32 jobOffset = i * jobSize;

        jobs[i] = CreateNewThreadJob([=]() {
            alignas(16) const m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

            for (uint32 iter = 0; iter < numIters; ++iter)
            {
                for (uint32 j = 0; j < jobSize; j += 16)
                {
                    uint32 index = j + jobOffset;
                    g_v4s_dst[index     ] = m * g_v4s[index     ];
                    g_v4s_dst[index + 1 ] = m * g_v4s[index + 1 ];
                    g_v4s_dst[index + 2 ] = m * g_v4s[index + 2 ];
                    g_v4s_dst[index + 3 ] = m * g_v4s[index + 3 ];
                    g_v4s_dst[index + 4 ] = m * g_v4s[index + 4 ];
                    g_v4s_dst[index + 5 ] = m * g_v4s[index + 5 ];
                    g_v4s_dst[index + 6 ] = m * g_v4s[index + 6 ];
                    g_v4s_dst[index + 7 ] = m * g_v4s[index + 7 ];
                    g_v4s_dst[index + 8 ] = m * g_v4s[index + 8 ];
                    g_v4s_dst[index + 9 ] = m * g_v4s[index + 9 ];
                    g_v4s_dst[index + 10] = m * g_v4s[index + 10];
                    g_v4s_dst[index + 11] = m * g_v4s[index + 11];
                    g_v4s_dst[index + 12] = m * g_v4s[index + 12];
                    g_v4s_dst[index + 13] = m * g_v4s[index + 13];
                    g_v4s_dst[index + 14] = m * g_v4s[index + 14];
                    g_v4s_dst[index + 15] = m * g_v4s[index + 15];
                }
            }
            });
    }

    for (uint32 i = 1; i < numJobs; ++i)
    {
        ThreadPool::EnqueueSingleJob(jobs[i]);
    }

    (*jobs[0])();
    jobs[0]->m_done = 1;
    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }
}

void BM_m4MulV4_fVectorized_MT()
{
    for (uint32 i = 0; i < numJobs; ++i)
    {
        uint32 jobOffset = i * jobSize;

        jobs[i] = CreateNewThreadJob([=]() {

            alignas(16) const m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

            for (uint32 iter = 0; iter < numIters; ++iter)
            {
                for (uint32 j = 0; j < jobSize; j += 16)
                {
                    uint32 index = j + jobOffset;
                    VectorOps::Mul_SIMD(&g_v4s[index     ], &m, &g_v4s_dst[index     ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 1 ], &m, &g_v4s_dst[index + 1 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 2 ], &m, &g_v4s_dst[index + 2 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 3 ], &m, &g_v4s_dst[index + 3 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 4 ], &m, &g_v4s_dst[index + 4 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 5 ], &m, &g_v4s_dst[index + 5 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 6 ], &m, &g_v4s_dst[index + 6 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 7 ], &m, &g_v4s_dst[index + 7 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 8 ], &m, &g_v4s_dst[index + 8 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 8 ], &m, &g_v4s_dst[index + 8 ]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 10], &m, &g_v4s_dst[index + 10]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 11], &m, &g_v4s_dst[index + 11]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 12], &m, &g_v4s_dst[index + 12]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 13], &m, &g_v4s_dst[index + 13]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 14], &m, &g_v4s_dst[index + 14]);
                    VectorOps::Mul_SIMD(&g_v4s[index + 15], &m, &g_v4s_dst[index + 15]);
                }
            }
        });
    }

    for (uint32 i = 1; i < numJobs; ++i)
    {
        ThreadPool::EnqueueSingleJob(jobs[i]);
    }

    (*jobs[0])();
    jobs[0]->m_done = 1;
    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }
}

void BM_v4_MT_Shutdown()
{
    ThreadPool::Shutdown();
    BM_v4_Shutdown();
}

void BM_m4MulV4_fScalar()
{
    const m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 iter = 0; iter < numIters; ++iter)
    {
        for (uint32 i = 0; i < numVectors; i += 16)
        {
            g_v4s_dst[i     ] = m * g_v4s[i     ];
            g_v4s_dst[i + 1 ] = m * g_v4s[i + 1 ];
            g_v4s_dst[i + 2 ] = m * g_v4s[i + 2 ];
            g_v4s_dst[i + 3 ] = m * g_v4s[i + 3 ];
            g_v4s_dst[i + 4 ] = m * g_v4s[i + 4 ];
            g_v4s_dst[i + 5 ] = m * g_v4s[i + 5 ];
            g_v4s_dst[i + 6 ] = m * g_v4s[i + 6 ];
            g_v4s_dst[i + 7 ] = m * g_v4s[i + 7 ];
            g_v4s_dst[i + 8 ] = m * g_v4s[i + 8 ];
            g_v4s_dst[i + 9 ] = m * g_v4s[i + 9 ];
            g_v4s_dst[i + 10] = m * g_v4s[i + 10];
            g_v4s_dst[i + 11] = m * g_v4s[i + 11];
            g_v4s_dst[i + 12] = m * g_v4s[i + 12];
            g_v4s_dst[i + 13] = m * g_v4s[i + 13];
            g_v4s_dst[i + 14] = m * g_v4s[i + 14];
            g_v4s_dst[i + 15] = m * g_v4s[i + 15];
       }
    }
}

void BM_m4MulV4_fVectorized()
{
    alignas(16) const m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 iter = 0; iter < numIters; ++iter)
    {
        for (uint32 i = 0; i < numVectors; i += 16)
        {
            VectorOps::Mul_SIMD(&g_v4s[i     ], &m, &g_v4s_dst[i     ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 1 ], &m, &g_v4s_dst[i + 1 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 2 ], &m, &g_v4s_dst[i + 2 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 3 ], &m, &g_v4s_dst[i + 3 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 4 ], &m, &g_v4s_dst[i + 4 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 5 ], &m, &g_v4s_dst[i + 5 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 6 ], &m, &g_v4s_dst[i + 6 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 7 ], &m, &g_v4s_dst[i + 7 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 8 ], &m, &g_v4s_dst[i + 8 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 8 ], &m, &g_v4s_dst[i + 8 ]);
            VectorOps::Mul_SIMD(&g_v4s[i + 10], &m, &g_v4s_dst[i + 10]);
            VectorOps::Mul_SIMD(&g_v4s[i + 11], &m, &g_v4s_dst[i + 11]);
            VectorOps::Mul_SIMD(&g_v4s[i + 12], &m, &g_v4s_dst[i + 12]);
            VectorOps::Mul_SIMD(&g_v4s[i + 13], &m, &g_v4s_dst[i + 13]);
            VectorOps::Mul_SIMD(&g_v4s[i + 14], &m, &g_v4s_dst[i + 14]);
            VectorOps::Mul_SIMD(&g_v4s[i + 15], &m, &g_v4s_dst[i + 15]);
        }
    }
}
