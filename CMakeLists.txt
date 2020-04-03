cmake_minimum_required (VERSION 3.15.1)
project(Tinker)

# Engine project
set(SOURCE_LIST
    "Source/Main.cpp"
    "Source/Utils/Timer.h"
)
add_executable(Tinker ${SOURCE_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST})
install (TARGETS Tinker DESTINATION bin)

# Benchmark project
set(SOURCE_LIST_BENCHMARK
    "Benchmark/BenchmarkMain.cpp"
    "Benchmark/TinkerBenchmark.h"
)
add_executable(TinkerBenchmark ${SOURCE_LIST_BENCHMARK})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_BENCHMARK})
install (TARGETS TinkerBenchmark DESTINATION bin)
