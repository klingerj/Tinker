#include "TinkerBenchmark.h"

#include "MathBenchmarks/VectorTypeBenchmarks.h"

static uint32 SampleBenchmarkFunc()
{
    uint32 num = 0;
    for (uint32 i = 0; i < 10000000; ++i)
    {
    	num += i*i;
    }
    return num;
}

OPTIMIZATIONS_OFF
int main()
{
    // Samples
    TINKER_BENCHMARK_HEADER;
    TINKER_BENCHMARK("Sample 1", SampleBenchmarkFunc);
    TINKER_BENCHMARK("Sample 2", []() {
        uint32 a = SampleBenchmarkFunc();
        uint32 b = a * SampleBenchmarkFunc();
        return a + b;
    });
    
    // Vector benchmarks
    TINKER_BENCHMARK("VectorType benchmark - V2 Add, Scalar, 10M", BM_v2Add_Scalar);
    TINKER_BENCHMARK("VectorType benchmark - V2 Add, Vectorized, 10M", BM_v2Add_Vectorized);
    TINKER_BENCHMARK("VectorType benchmark - M2F * V2F, Scalar, 10M", BM_m2MulV2_fScalar);
    TINKER_BENCHMARK("VectorType benchmark - M2F * V2F, Vectorized, 10M", BM_m2MulV2_fVectorized);
    TINKER_BENCHMARK("VectorType benchmark - M2i * V2i, Scalar, 10M", BM_m2MulV2_iScalar);
    
    exit(EXIT_SUCCESS);
}
OPTIMIZATIONS_ON
