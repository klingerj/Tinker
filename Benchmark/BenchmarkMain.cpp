#include "TinkerBenchmark.h"

#include "MathBenchmarks/VectorTypeBenchmarks.h"


int main()
{
    TINKER_BENCHMARK_HEADER;
    
    // Vector benchmarks
    //TINKER_BENCHMARK("VectorType - V2 Add, Scalar", BM_v2Add_Scalar);
    //TINKER_BENCHMARK("VectorType - V2 Add, Vectorized", BM_v2Add_Vectorized);
    //TINKER_BENCHMARK("VectorType - M2I * V2I, Scalar", BM_m2MulV2_iScalar);
    //TINKER_BENCHMARK("VectorType - M2I * V2I, Vectorized", BM_m2MulV2_iVectorized);
    TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Scalar", BM_v2_Startup, BM_m2MulV2_fScalar, BM_v2_Shutdown);
    TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Scalar, Multithreaded x10 threads", BM_m2MulV2_fScalar_MT_Startup, BM_m2MulV2_fScalar_MT, BM_m2MulV2_fScalar_MT_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Vectorized", BM_v2_Startup, BM_m2MulV2_fVectorized, BM_v2_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Scalar", BM_v4_Startup, BM_m4MulV4_fScalar, BM_v4_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Vectorized", BM_v4_Startup, BM_m4MulV4_fVectorized, BM_v4_Shutdown);

    exit(EXIT_SUCCESS);
}
