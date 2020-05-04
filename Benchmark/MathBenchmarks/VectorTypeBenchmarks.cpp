#include "VectorTypeBenchmarks.h"

#include "../../Platform/Win32WorkerThreadPool.cpp"

using namespace Tinker;
using namespace Platform;

WorkerThreadPool g_threadpool;

v2f* g_v2s = nullptr;
v4f* g_v4s = nullptr;
const uint32 numVectors = 1 << 24;

const uint32 numJobs = 128;
const uint32 jobSize = numVectors / numJobs;
WorkerJob* jobs[numJobs];

void BM_v2_Startup()
{
    g_v2s = (v2f*)AllocAligned(numVectors * sizeof(v2f), 64);
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
    v2i v = { 4, 5 };
    for (uint32 i = 0; i < 10000000; ++i)
    {
        m2i m = { (int32)i, (int32)i + 1, (int32)i + 2, (int32)i + 3 };
        v = VectorOps::Mul_SIMD(v, m);
    }
}

void BM_m2MulV2_fScalar()
{
    v2f* const vectors = g_v2s;
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

    for (uint32 i = 0; i < numVectors; ++i)
    {
        vectors[i] = m * vectors[i];
    }
}

void BM_m2MulV2_fScalar_MT_Startup()
{
    g_threadpool.Startup(10);
    BM_v2_Startup();

    v2f* const vectors = g_v2s;
    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i] = CreateNewThreadJob([=]() {
            const m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

            for (uint32 j = 0; j < jobSize; ++j)
            {
                uint32 index = j + i * jobSize;
                vectors[index] = m * vectors[index];
            }
        });
    }
}

void BM_m2MulV2_fScalar_MT()
{
    for (uint32 i = 0; i < numJobs; ++i)
    {
        jobs[i]->m_done = false;
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
        delete jobs[i];
    }
}

void BM_m2MulV2_fVectorized()
{
    v2f* const vectors = g_v2s;
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

    for (uint32 i = 0; i < numVectors; ++i)
    {
        vectors[i] = VectorOps::Mul_SIMD(vectors[i], m);
    }
}

void BM_v4_Startup()
{
    g_v4s = (v4f*)AllocAligned(numVectors * sizeof(v4f), 64);
    for (uint32 i = 0; i < numVectors; ++i)
    {
        g_v4s[i] = v4f((float)i, (float)(i + 1),(float)(i + 2), (float)(i + 3));
    }
}

void BM_v4_Shutdown()
{
    FreeAligned(g_v4s);
    g_v4s = nullptr;
}

void BM_m4MulV4_fScalar()
{
    v4f* const vectors = g_v4s;
    m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 i = 0; i < numVectors; ++i)
    {
        vectors[i] = m * vectors[i];
    }
}

void BM_m4MulV4_fVectorized()
{
    v4f* const vectors = g_v4s;
    m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 i = 0; i < numVectors; ++i)
    {
        VectorOps::Mul_SIMD(vectors[i], m, vectors[i]);
    }
}
