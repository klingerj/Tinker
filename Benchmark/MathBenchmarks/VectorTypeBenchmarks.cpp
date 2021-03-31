#include "VectorTypeBenchmarks.h"

#include "Win32WorkerThreadPool.cpp"

using namespace Tinker;
using namespace Platform;
using namespace Core;
using namespace Math;

const uint32 threadCount = 8;
WorkerThreadPool g_threadpool;

const uint32 numJobs = threadCount;
WorkerJob* jobs[numJobs];

v2f* g_v2s = nullptr;
v4f* g_v4s = nullptr;
v4f* g_v4s_dst = nullptr;
const uint32 jobSize = (2 << 16);
const uint32 numVectors = numJobs * jobSize;
const uint32 numIters = 25;

void BM_v2_Startup()
{
    g_v2s = (v2f*)AllocAligned(numVectors * sizeof(v2f), 64, __FILE__, __LINE__);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v2s[i] = v2f((float)i, (float)(i + 1));
    }
}

void BM_v2_Shutdown()
{
    FreeAligned(g_v2s);
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
    g_threadpool.Startup(threadCount);
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
        jobs[i]->m_done = false;
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        g_threadpool.EnqueueNewThreadJob(jobs[i]);
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }
}

void BM_m2MulV2_fScalar_MT_Shutdown()
{
    g_threadpool.Shutdown();
    BM_v2_Shutdown();
    for (uint32 i = 0; i < numJobs; ++i)
    {
        FreeAligned(jobs[i]);
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
    g_v4s = (v4f*)AllocAligned(numVectors * sizeof(v4f), 64, __FILE__, __LINE__);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v4s[i] = v4f((float)i, (float)(i + 1),(float)(i + 2), (float)(i + 3));
    }

    g_v4s_dst = (v4f*)AllocAligned(numVectors * sizeof(v4f), 64, __FILE__, __LINE__);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v4s_dst[i] = v4f((float)i, (float)(i + 1), (float)(i + 2), (float)(i + 3));
    }
}

void BM_v4_Shutdown()
{
    FreeAligned(g_v4s);
    g_v4s = nullptr;
    FreeAligned(g_v4s_dst);
    g_v4s_dst = nullptr;
}

void BM_v4_MT_Startup()
{
    g_threadpool.Startup(threadCount);
    BM_v4_Startup();
}

void BM_m4MulV4_fScalar_MT()
{
    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i] = CreateNewThreadJob([=]() {
            alignas(16) const m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

            for (uint32 iter = 0; iter < numIters; ++iter)
            {
                for (uint32 j = 0; j < jobSize; ++j)
                {
                    uint32 index = j + i * jobSize;
                    g_v4s_dst[index] = m * g_v4s[index];
                }
            }
            });
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i]->m_done = false;
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        g_threadpool.EnqueueNewThreadJob(jobs[i]);
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }
}

void BM_m4MulV4_fVectorized_MT()
{
    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i] = CreateNewThreadJob([=]() {
            alignas(16) const m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

            for (uint32 iter = 0; iter < numIters; ++iter)
            {
                for (uint32 j = 0; j < jobSize; ++j)
                {
                    uint32 index = j + i * jobSize;
                    VectorOps::Mul_SIMD(&g_v4s[index], &m, &g_v4s_dst[index]);
                }
            }
        });
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i]->m_done = false;
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        g_threadpool.EnqueueNewThreadJob(jobs[i]);
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }
}

void BM_v4_MT_Shutdown()
{
    g_threadpool.Shutdown();
    BM_v4_Shutdown();
}

void BM_m4MulV4_fScalar()
{
    m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 iter = 0; iter < numIters; ++iter)
    {
        for (uint32 i = 0; i < numVectors; ++i)
        {
            //m.m_data[i % 16] = (float)i;
            g_v4s_dst[i] = m * g_v4s[i];
        }
    }
}

void BM_m4MulV4_fVectorized()
{
    alignas(16) m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 iter = 0; iter < numIters; ++iter)
    {
        for (uint32 i = 0; i < numVectors; ++i)
        {
            //m.m_data[i % 16] = (float)i;
            VectorOps::Mul_SIMD(&g_v4s[i], &m, &g_v4s_dst[i]);
        }
    }
}
