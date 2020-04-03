#include "TinkerBenchmark.h"

#include <stdint.h>

static int SampleBenchmarkFunc()
{
    volatile uint32_t num = 0;
    for (uint32_t i = 0; i < 100000000; ++i)
    {
    	num += i;
    }
    return num;
}

int main()
{
    TINKER_BENCHMARK_HEADER()
    TINKER_BENCHMARK("Benchmark 1", SampleBenchmarkFunc)
    TINKER_BENCHMARK("Benchmark 2", []() {
        volatile int a = SampleBenchmarkFunc();
        volatile int b = a * SampleBenchmarkFunc();
        return a + b;
    })
    
    exit(EXIT_SUCCESS);
}
