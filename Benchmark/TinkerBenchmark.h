#pragma once

#include "../Include/Core/CoreDefines.h"

#include <windows.h>
#include <iostream>

#define NUM_SAMPLES 10
#define SEC_2_MSEC 1000.0f

#define TINKER_BENCHMARK_HEADER \
        std::cout << "Tinker Engine Benchmarks\n"; \
        std::cout << "Number of times each benchmark will be run: " << NUM_SAMPLES << "\n\n";

#define TINKER_PRINT_STATS(timeSamples) \
        float bestTime = FLT_MAX, avgTime = 0.0f, worstTime = FLT_MIN; \
        float timeSum = 0.0f; \
        for (uint32 i = 0; i < NUM_SAMPLES; ++i) \
        { \
            timeSum += timeSamples[i]; \
            bestTime = MIN(bestTime, timeSamples[i]); \
            worstTime = MAX(worstTime, timeSamples[i]); \
        } \
        avgTime = timeSum / NUM_SAMPLES; \
        std::cout << "Best Time: "  << bestTime << " ms\n"; \
        std::cout << "Avg Time: "   << avgTime <<  " ms\n"; \
        std::cout << "Worst Time: " << worstTime <<  " ms\n\n";

#define TINKER_BENCHMARK(str, func) { \
        std::cout << str << ":\n"; \
        float timeSamples[NUM_SAMPLES] = {}; \
        LARGE_INTEGER start = {}, end = {}, freq = {}; \
        QueryPerformanceFrequency(&freq); \
        for (uint32 i = 0; i < NUM_SAMPLES; ++i) \
        { \
            QueryPerformanceCounter(&start); \
            func(); \
            QueryPerformanceCounter(&end); \
            timeSamples[i] = (float)(end.QuadPart - start.QuadPart) * SEC_2_MSEC / (float)freq.QuadPart; \
        } \
        TINKER_PRINT_STATS(timeSamples); \
        }

#define TINKER_BENCHMARK_STARTUP_SHUTDOWN(str, funcSU, func, funcSD) { \
        std::cout << str << ":\n"; \
        float timeSamples[NUM_SAMPLES] = {}; \
        LARGE_INTEGER start = {}, end = {}, freq = {}; \
        QueryPerformanceFrequency(&freq); \
        funcSU(); \
        for (uint32 i = 0; i < NUM_SAMPLES; ++i) \
        { \
            QueryPerformanceCounter(&start); \
            func(); \
            QueryPerformanceCounter(&end); \
            timeSamples[i] = (float)(end.QuadPart - start.QuadPart) * SEC_2_MSEC / (float)freq.QuadPart; \
        } \
        funcSD(); \
        TINKER_PRINT_STATS(timeSamples); \
        }
