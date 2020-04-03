#pragma once

#include "../Source/Utils/Timer.h"

#include <iostream>

#define MAX(a, b) (a > b) ? a : b;
#define MIN(a, b) (a < b) ? a : b;

class Benchmark
{
public:
    virtual void operator()() = 0;
};

template <typename T>
class BenchmarkFuncContainer : Benchmark
{
private:
    T t;

public:
    void operator()() override { t(); }
};

Timer g_Timer;

#define NUM_SAMPLES 25

#define TINKER_BENCHMARK_HEADER() \
            std::cout << "Tinker Engine Benchmarks\n"; \
            std::cout << "Number of times each benchmark will be run: " << NUM_SAMPLES << "\n\n";

#define TINKER_PRINT_STATS(timeSamples) \
            float bestTime = FLT_MAX, avgTime = 0.0f, worstTime = FLT_MIN; \
            float timeSum = 0.0f; \
            for (uint32_t i = 0; i < NUM_SAMPLES; ++i) \
            { \
                timeSum += timeSamples[i]; \
                bestTime = MIN(bestTime, timeSamples[i]); \
                worstTime = MAX(worstTime, timeSamples[i]); \
            } \
            avgTime = timeSum / NUM_SAMPLES; \
            std::cout << "Best Time: "  << bestTime << " ms\n"; \
            std::cout << "Avg Time: "   << avgTime <<  " ms\n"; \
            std::cout << "Worst Time: " << worstTime <<  " ms\n\n";

#define TINKER_BENCHMARK(n, f) { \
                std::cout << n << ":\n"; \
                \
                float timeSamples[NUM_SAMPLES] = {}; \
                for (uint32_t i = 0; i < NUM_SAMPLES; ++i) \
                { \
                    g_Timer.Start(); \
                    f(); \
                    g_Timer.Stop(); \
                    timeSamples[i] = g_Timer.ElapsedTime(); \
                } \
                TINKER_PRINT_STATS(timeSamples); \
            }
