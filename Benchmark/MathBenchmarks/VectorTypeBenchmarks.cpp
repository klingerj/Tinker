#include "VectorTypeBenchmarks.h"

#include "../Include/System/WorkerThreadPool.h"

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
    m2i m = { 1, 2, 3, 4 };
    v2i v = { 5, 6 };

    for (uint32 i = 0; i < 10000000; ++i)
    {
        v = m * v;
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

void BM_m2MulV2_iScalar_MT_Startup()
{
    g_threadpool.Startup(8);
}

void BM_m2MulV2_iScalar_MT()
{
    constexpr uint32 workSize = 10000000; // 10M
    constexpr uint32 numJobs = 8;
    WorkerJob* jobs[numJobs];
    for (uint16 i = 0; i < numJobs; ++i)
    {
        jobs[i] = g_threadpool.NewJob([&]() {
            m2f m = { 1.0f, 2.0f, 3.0f, 4.0f };
            v2f v = { 5.0f, 6.0f };

            for (uint32 i = 0; i < workSize / numJobs; ++i)
            {
                v = m * v;
            }
        });
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

void BM_m2MulV2_iScalar_MT_Shutdown()
{
    g_threadpool.Shutdown();
}

void BM_m2MulV2_fVectorized()
{
    // TODO: SSE/AVX version of m2 * v2 mul
}
