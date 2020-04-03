#include "TinkerBenchmark.h"

#include <stdint.h>

static int SampleBenchmarkFunc()
{
    uint32_t num = 0;
    for (uint32_t i = 0; i < 10000000; ++i)
    {
    	num += i*i;
    }
    return num;
}

OPTIMIZATIONS_OFF
int main()
{
    TINKER_BENCHMARK_HEADER()
    TINKER_BENCHMARK("Benchmark 1", SampleBenchmarkFunc)
    TINKER_BENCHMARK("Benchmark 2", []() {
        int a = SampleBenchmarkFunc();
        int b = a * SampleBenchmarkFunc();
        return a + b;
    })
    
    exit(EXIT_SUCCESS);
}
OPTIMIZATIONS_ON
