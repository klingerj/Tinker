@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

if "%2" == "true" (
    rem Run core engine build script
    call build_core_engine_lib.bat %BuildConfig%
    )

echo.
echo ***** Building Tinker Benchmarks *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerBenchmark.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHsc /GR- /Gm- /GS- /fp:fast /Zi
set CommonLinkFlags=/incremental:no /opt:ref /DEBUG

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )

rem *********************************************************************************************************
rem TinkerBenchmark - benchmarking
set SourceListBenchmark=../Benchmark/BenchmarkMain.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/Platform/Win32PlatformGameAPI.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/Platform/Win32Logging.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/Mem.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Benchmark/MathBenchmarks/VectorTypeBenchmarks.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Benchmark/ContainerBenchmarks/HashMapBenchmarks.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/DataStructures/HashMap.cpp 
set CompileDefines= 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsBenchmark=/FdTinkerBenchmark.pdb
    set DebugLinkFlagsBenchmark=/pdb:TinkerBenchmark.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    ) else (
    set DebugCompileFlagsBenchmark=/FdTinkerBenchmark.pdb
    set DebugLinkFlagsBenchmark=/pdb:TinkerBenchmark.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=0
    )

set CompileIncludePaths=/I ../Core 

set OBJDir=%cd%\obj_benchmark\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerBenchmark.exe...
cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsBenchmark% %SourceListBenchmark% /link %CommonLinkFlags% TinkerApp.lib Winmm.lib %DebugLinkFlagsBenchmark% /out:TinkerBenchmark.exe

:DoneBuild
popd
popd
