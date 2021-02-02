#include "TinkerBenchmark.h"

#include "MathBenchmarks/VectorTypeBenchmarks.h"
#include "../Include/Core/Utilities/ScopedTimer.h"

#include <timeapi.h>

int main()
{
    timeBeginPeriod(1);
    TINKER_BENCHMARK_HEADER;
    
    // Vector benchmarks
    //TINKER_BENCHMARK("VectorType - V2 Add, Scalar", BM_v2Add_Scalar);
    //TINKER_BENCHMARK("VectorType - V2 Add, Vectorized", BM_v2Add_Vectorized);
    //TINKER_BENCHMARK("VectorType - M2I * V2I, Scalar", BM_m2MulV2_iScalar);
    //TINKER_BENCHMARK("VectorType - M2I * V2I, Vectorized", BM_m2MulV2_iVectorized);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Scalar", BM_v2_Startup, BM_m2MulV2_fScalar, BM_v2_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Scalar, Multithreaded", BM_m2MulV2_fScalar_MT_Startup, BM_m2MulV2_fScalar_MT, BM_m2MulV2_fScalar_MT_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Vectorized", BM_v2_Startup, BM_m2MulV2_fVectorized, BM_v2_Shutdown);

    // V$ mul M4
    TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Scalar", BM_v4_Startup, BM_m4MulV4_fScalar, BM_v4_Shutdown);
    for (uint32 i = 0; i < 10; ++i)
    {
        {
            TIMED_SCOPED_BLOCK("all the benchmark1");

            BM_v4_Startup();
            BM_m4MulV4_fScalar();
            BM_v4_Shutdown();
        }
    }

    TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Vectorized", BM_v4_Startup, BM_m4MulV4_fVectorized, BM_v4_Shutdown);
    for (uint32 i = 0; i < 10; ++i)
    {
        {
            TIMED_SCOPED_BLOCK("all the benchmark2");

            BM_v4_Startup();
            BM_m4MulV4_fVectorized();
            BM_v4_Shutdown();
        }
    }

    for (uint32 i = 0; i < 10; ++i)
    {
        {
            TIMED_SCOPED_BLOCK("all the benchmark3");

            BM_v4_MT_Startup();
            BM_m4MulV4_fScalar_MT();
            BM_v4_MT_Shutdown();
        }
    }

    for (uint32 i = 0; i < 10; ++i)
    {
        {
            TIMED_SCOPED_BLOCK("all the benchmark4");

            BM_v4_MT_Startup();
            BM_m4MulV4_fVectorized_MT();
            BM_v4_MT_Shutdown();
        }
    }

    TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Scalar, Multithreaded", BM_v4_MT_Startup, BM_m4MulV4_fScalar_MT, BM_v4_MT_Shutdown);
    TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Vectorized, Multithreaded", BM_v4_MT_Startup, BM_m4MulV4_fVectorized_MT, BM_v4_MT_Shutdown);

    exit(EXIT_SUCCESS);
}
