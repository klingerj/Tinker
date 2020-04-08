cmake_minimum_required (VERSION 3.15.1)
project(Tinker VERSION 1.0.1 DESCRIPTION "The Tinker Engine")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

if(WIN32)
    set(CMAKE_CXX_FLAGS "/std:c++17 /O2 /W3 /WX /EHsc")
endif()

# Engine project - builds as .lib
set(SOURCE_LIST
    "Include/Math/VectorTypes.h"
    "Include/System/SystemDefines.h"
    "Include/System/WorkerThreadPool.h"
    "Include/Platform/PlatformAPI.h"
    "Include/Containers/RingBuffer.h"
    "Source/System/WorkerThreadPool.cpp"
    "Source/Platform/Win32Layer.cpp"
    "Source/Math/VectorTypes.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST})
add_library(Tinker STATIC ${SOURCE_LIST})
set_target_properties(Tinker PROPERTIES VERSION ${PROJECT_VERSION})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)
install(TARGETS Tinker DESTINATION bin)

# App Demo project
set(SOURCE_LIST_DEMO
    "Demo/Main.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_DEMO})
add_executable(TinkerDemo ${SOURCE_LIST_DEMO})
target_link_libraries(TinkerDemo Tinker)
install(TARGETS TinkerDemo DESTINATION bin)

# Benchmark project
set(SOURCE_LIST_BENCHMARK
    "Benchmark/BenchmarkMain.cpp"
    "Benchmark/TinkerBenchmark.h"
    "Benchmark/MathBenchmarks/VectorTypeBenchmarks.h"
    "Benchmark/MathBenchmarks/VectorTypeBenchmarks.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_BENCHMARK})
add_executable(TinkerBenchmark ${SOURCE_LIST_BENCHMARK})
target_link_libraries(TinkerBenchmark Tinker)
install(TARGETS TinkerBenchmark DESTINATION bin)

# Unit Testing project
set(SOURCE_LIST_TEST
    "Test/TestMain.cpp"
    "Test/TinkerTest.h"
    "Test/MathTests/VectorTypeTests.h"
    "Test/ContainerTests/RingBufferTests.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_TEST})
add_executable(TinkerTest ${SOURCE_LIST_TEST})
target_link_libraries(TinkerTest Tinker)
install(TARGETS TinkerTest DESTINATION bin)
