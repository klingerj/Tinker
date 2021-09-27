#include "TinkerBenchmark.h"

#include "MathBenchmarks/VectorTypeBenchmarks.h"
#include "ContainerBenchmarks/HashMapBenchmarks.h"
#include "Utility/ScopedTimer.h"

#include <timeapi.h>
#include <chrono>
#include <stdio.h>

int main()
{
    //SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    //timeBeginPeriod(1);
    //TINKER_BENCHMARK_HEADER;
    
    // Vector benchmarks
    //TINKER_BENCHMARK("VectorType - V2 Add, Scalar", BM_v2Add_Scalar);
    //TINKER_BENCHMARK("VectorType - V2 Add, Vectorized", BM_v2Add_Vectorized);
    //TINKER_BENCHMARK("VectorType - M2I * V2I, Scalar", BM_m2MulV2_iScalar);
    //TINKER_BENCHMARK("VectorType - M2I * V2I, Vectorized", BM_m2MulV2_iVectorized);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Scalar", BM_v2_Startup, BM_m2MulV2_fScalar, BM_v2_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Scalar, Multithreaded", BM_m2MulV2_fScalar_MT_Startup, BM_m2MulV2_fScalar_MT, BM_m2MulV2_fScalar_MT_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M2F * V2F, Vectorized", BM_v2_Startup, BM_m2MulV2_fVectorized, BM_v2_Shutdown);

    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("Hashmap insert 1m uint32", BM_hm_Startup, BM_HMIns1M_fScalar, BM_hm_Shutdown);
    /*
    BM_hm_Startup();
    auto start_ = std::chrono::high_resolution_clock::now();
    BM_HMIns1M_fScalar();
    auto elapsed_ = std::chrono::high_resolution_clock::now() - start_;
    BM_hm_Shutdown();
    long long microseconds_ = std::chrono::duration_cast<std::chrono::microseconds>(elapsed_).count();
    std::cout << "Elapsed time: " << microseconds_ << " us" << std::endl;
    */

    /*
    for (uint32 i = 0; i < 10; ++i)
    {
        BM_hm_Startup();
        {
            TIMED_SCOPED_BLOCK("Hashmap benchmark");

            BM_HMIns1M_fScalar();
        }
        BM_hm_Shutdown();
    }

    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("std unordered map insert 1m uint32", BM_hm_Startup, BM_StdHMIns1M_fScalar, BM_hm_Shutdown);
    for (uint32 i = 0; i < 10; ++i)
    {
        BM_hm_Startup();
        {
            TIMED_SCOPED_BLOCK("Std unordered map benchmark");
            
            BM_StdHMIns1M_fScalar();
        }
        BM_hm_Shutdown();
    }
    */

    // V4 mul M4
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Scalar", BM_v4_Startup, BM_m4MulV4_fScalar, BM_v4_Shutdown);
    for (uint32 i = 0; i < 10; ++i)
    {
        BM_v4_Startup();
        {
            TIMED_SCOPED_BLOCK("VectorType - M4F * V4F, Scalar");

            BM_m4MulV4_fScalar();
        }
        BM_v4_Shutdown();
    }

    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Vectorized", BM_v4_Startup, BM_m4MulV4_fVectorized, BM_v4_Shutdown);
    for (uint32 i = 0; i < 10; ++i)
    {
        BM_v4_Startup();
        {
            TIMED_SCOPED_BLOCK("VectorType - M4F * V4F, Vectorized");

            BM_m4MulV4_fVectorized();
        }
        BM_v4_Shutdown();
    }

    for (uint32 i = 0; i < 10; ++i)
    {
        BM_v4_MT_Startup();
        {
            TIMED_SCOPED_BLOCK("VectorType - M4F * V4F, Scalar, Multithreaded");

            BM_m4MulV4_fScalar_MT();
        }
        BM_v4_MT_Shutdown();
    }

    for (uint32 i = 0; i < 10; ++i)
    {
        BM_v4_MT_Startup();
        {
            TIMED_SCOPED_BLOCK("VectorType - M4F * V4F, Vectorized, Multithreaded");
            BM_m4MulV4_fVectorized_MT();
        }
        BM_v4_MT_Shutdown();
    }

    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Scalar, Multithreaded", BM_v4_MT_Startup, BM_m4MulV4_fScalar_MT, BM_v4_MT_Shutdown);
    //TINKER_BENCHMARK_STARTUP_SHUTDOWN("VectorType - M4F * V4F, Vectorized, Multithreaded", BM_v4_MT_Startup, BM_m4MulV4_fVectorized_MT, BM_v4_MT_Shutdown);

    exit(EXIT_SUCCESS);
}
