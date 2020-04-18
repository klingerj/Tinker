#include "VectorTypeBenchmarks.h"

#include "../Platform/Win32WorkerThreadPool.cpp"

using namespace Tinker;
using namespace Platform;

WorkerThreadPool g_threadpool;

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
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };
    v2f v = { 5.0f, 6.0f };

    for (uint32 i = 0; i < 10000000; ++i)
    {
        v = m * v;
    }
}

void BM_m2MulV2_fScalar_MT_Startup()
{
    g_threadpool.Startup(10);
}

void BM_m2MulV2_fScalar_MT()
{
    constexpr uint32 workSize = 10000000;
    constexpr uint32 numJobs = 10;
    WorkerJob* jobs[numJobs];
    for (uint16 i = 0; i < numJobs; ++i)
    {
        jobs[i] = CreateNewThreadJob([&]() {
            const m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };
            v2f v = { 5.0f, 6.0f };

            for (uint32 i = 0; i < workSize / numJobs; ++i)
            {
                v = m * v;
            }
        });
    }
    for (uint16 i = 0; i < numJobs; ++i)
    {
        g_threadpool.EnqueueNewThreadJob(jobs[i]);
    }

    for (uint16 i = 0; i < numJobs; ++i)
    {
        while (!jobs[i]->m_done);
    }

    for (uint16 i = 0; i < numJobs; ++i)
    {
        delete jobs[i];
    }
}

void BM_m2MulV2_fScalar_MT_Shutdown()
{
    g_threadpool.Shutdown();
}

void BM_m2MulV2_fVectorized()
{
    m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };
    v2f v = { 5.0f, 6.0f };

    for (uint32 i = 0; i < 10000000; ++i)
    {
        v = VectorOps::Mul_SIMD(v, m);
    }
}
