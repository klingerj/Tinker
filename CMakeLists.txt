cmake_minimum_required (VERSION 3.15.1)
project(Tinker)

if(WIN32)
    set(CMAKE_CXX_FLAGS "/std:c++17 /O2 /W3 /WX /EHsc")
endif()

# Engine project
set(SOURCE_LIST
    "Source/Main.cpp"
    "Source/Math/VectorTypes.h"
    "Source/Math/VectorTypes.cpp"
    "Source/System/CoreDefines.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST})
add_executable(Tinker ${SOURCE_LIST})
install (TARGETS Tinker DESTINATION bin)

# Benchmark project
set(SOURCE_LIST_BENCHMARK
    "Benchmark/BenchmarkMain.cpp"
    "Benchmark/TinkerBenchmark.h"
    "Benchmark/MathBenchmarks/VectorTypeBenchmarks.h"
    "Benchmark/MathBenchmarks/VectorTypeBenchmarks.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_BENCHMARK})
add_executable(TinkerBenchmark ${SOURCE_LIST_BENCHMARK})
install (TARGETS TinkerBenchmark DESTINATION bin)

# Unit Testing project
set(SOURCE_LIST_TEST
    "Test/TestMain.cpp"
    "Test/TinkerTest.h"
    "Test/MathTests/VectorTypeTests.cpp"
    "Test/MathTests/VectorTypeTests.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_TEST})
add_executable(TinkerTest ${SOURCE_LIST_TEST})
install (TARGETS TinkerTest DESTINATION bin)
