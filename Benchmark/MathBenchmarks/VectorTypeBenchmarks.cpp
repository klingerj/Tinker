#include "VectorTypeBenchmarks.h"

#include "../../Platform/Win32WorkerThreadPool.cpp"

using namespace Tinker;
using namespace Platform;

WorkerThreadPool g_threadpool;

v2f* g_v2s = nullptr;
v4f* g_v4s = nullptr;

void BM_v2_Startup()
{
    g_v2s = new v2f[10000000];
    for (uint32 i = 0; i < 10000000; ++i)
    {
        g_v2s[i] = v2f((float)i, (float)(i + 1));
    }
}

void BM_v2_Shutdown()
{
    delete g_v2s;
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

    for (uint32 i = 0; i < 10000000; ++i)
    {
        vectors[i] = m * vectors[i];
    }
}

void BM_m2MulV2_fScalar_MT_Startup()
{
    g_threadpool.Startup(10);
    BM_v2_Startup();
}

void BM_m2MulV2_fScalar_MT()
{
    constexpr uint32 workSize = 10000000;
    constexpr uint32 numJobs = 10;
    WorkerJob* jobs[numJobs];
    v2f* const vectors = g_v2s;
    for (uint16 i = 0; i < numJobs; ++i)
    {
        jobs[i] = CreateNewThreadJob([&]() {
            m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

            for (uint32 i = 0; i < workSize / numJobs; ++i)
            {
                vectors[i] = m * vectors[i];
            }
        });
        g_threadpool.EnqueueNewThreadJob(jobs[i]);
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        WaitOnJob(jobs[i]);
    }

    for (uint32 i = 0; i < numJobs; ++i)
    {
        delete jobs[i];
    }
}

void BM_m2MulV2_fScalar_MT_Shutdown()
{
    g_threadpool.Shutdown();
    BM_v2_Shutdown();
}

void BM_m2MulV2_fVectorized()
{
    v2f* const vectors = g_v2s;
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };

    for (uint32 i = 0; i < 10000000; ++i)
    {
        vectors[i] = VectorOps::Mul_SIMD(vectors[i], m);
    }
}

void BM_v4_Startup()
{
    g_v4s = (v4f*)AllocAligned(10000000 * sizeof(v4f), 64);
    for (uint32 i = 0; i < 10000000; ++i)
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

    for (uint32 i = 0; i < 10000000; ++i)
    {
        vectors[i] = m * vectors[i];
    }
}

void BM_m4MulV4_fVectorized()
{
    v4f* const vectors = g_v4s;
    m4f m = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

    for (uint32 i = 0; i < 10000000; ++i)
    {
        VectorOps::Mul_SIMD(vectors[i], m, vectors[i]);
    }
}
