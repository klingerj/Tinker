cmake_minimum_required (VERSION 3.15.1 FATAL_ERROR)
project(Tinker VERSION 1.0.1 DESCRIPTION "The Tinker Engine")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

if(WIN32)
    set(CMAKE_CXX_FLAGS "/nologo /std:c++17 /MT /O2 /W4 /WX /wd4201 /wd4324 /wd4100 /wd4189 /EHsc /GR- /Gm-")
    #add_link_options("/subsystem:windows,5.1") # for running on XP

    # Remove all pbd files - needed for code hotloading
    file(GLOB PDB_FILES "${CMAKE_SOURCE_DIR}/Build/Debug/*.pdb")
    foreach(pdb ${PDB_FILES})
        file(REMOVE "${pdb}")
    endforeach()
endif()

# Engine project - builds as .lib
set(SOURCE_LIST_CORE
    "Core/Math/VectorTypes.cpp"
)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_LIST_CORE})
add_library(TinkerCore STATIC ${SOURCE_LIST_CORE})
set_target_properties(TinkerCore PROPERTIES VERSION ${PROJECT_VERSION})
include_directories(${CMAKE_SOURCE_DIR}/Include)
install(TARGETS TinkerCore DESTINATION bin)

# Platform - project
set(SOURCE_LIST_PLATFORM
    "Platform/Win32Layer.cpp"
    "Platform/Win32PlatformGameAPI.cpp"
)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_LIST_PLATFORM})
include_directories(${CMAKE_SOURCE_DIR}/Include)
add_executable(TinkerPlatform ${SOURCE_LIST_PLATFORM})
set_property(TARGET TinkerPlatform PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")
install(TARGETS TinkerPlatform DESTINATION bin)

# Game demo project - builds as dll
set(SOURCE_LIST_GAME
    "Game/GameMain.cpp"
    "Platform/Win32PlatformGameAPI.cpp"
)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_LIST_GAME})
add_library(TinkerGame SHARED ${SOURCE_LIST_GAME})
target_link_libraries(TinkerGame TinkerCore)

target_link_options(TinkerGame PRIVATE "/EXPORT:GameUpdate")

if (WIN32)
    #add_custom_command(TARGET TinkerGame PRE_BUILD COMMAND echo "%date%_%time% > ${CURR_TIME}" VERBATIM)
    #add_custom_target(Timestamp_Force_Rebuild ALL :)
    #add_dependencies(TinkerGame Timestamp_Force_Rebuild)
    string(TIMESTAMP CURR_TIME %Y_%d_%m_%H_%M_%S)
    message("${CURR_TIME}")
    target_compile_options(TinkerGame PRIVATE "/Fd${CMAKE_SOURCE_DIR}/Build/Debug/TinkerGame_${CURR_TIME}.pdb")
    target_link_options(TinkerGame PRIVATE "/PDB:${CMAKE_SOURCE_DIR}/Build/Debug/TinkerGame_${CURR_TIME}.pdb")
endif()

install(TARGETS TinkerGame DESTINATION bin)

# Benchmark project
set(SOURCE_LIST_BENCHMARK
    "Platform/Win32PlatformGameAPI.cpp"
    "Benchmark/BenchmarkMain.cpp"
    "Benchmark/TinkerBenchmark.h"
    "Benchmark/MathBenchmarks/VectorTypeBenchmarks.h"
    "Benchmark/MathBenchmarks/VectorTypeBenchmarks.cpp"
)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_LIST_BENCHMARK})
add_executable(TinkerBenchmark ${SOURCE_LIST_BENCHMARK})
target_link_libraries(TinkerBenchmark TinkerCore)
install(TARGETS TinkerBenchmark DESTINATION bin)

# Unit Testing project
set(SOURCE_LIST_TEST
    "Platform/Win32PlatformGameAPI.cpp"
    "Test/TestMain.cpp"
    "Test/TinkerTest.h"
    "Test/MathTests/VectorTypeTests.h"
    "Test/ContainerTests/RingBufferTests.h"
    "Test/MemoryTests/AllocatorTests.h"
)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_LIST_TEST})
add_executable(TinkerTest ${SOURCE_LIST_TEST})
target_link_libraries(TinkerTest TinkerCore)
install(TARGETS TinkerTest DESTINATION bin)
