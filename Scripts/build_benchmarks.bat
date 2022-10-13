@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

echo.
echo ***** Building Tinker Benchmarks *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerBenchmark.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++20 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /wd4702 /EHsc /GR- /Gm- /GS- /fp:fast /Zi /FS
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
set SourceListBenchmark=%SourceListBenchmark% ../Core/Platform/Win32WorkerThreadPool.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/Platform/Win32Logging.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/Mem.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/Utility/MemTracker.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Benchmark/MathBenchmarks/VectorTypeBenchmarks.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Benchmark/DataStructureBenchmarks/HashMapBenchmarks.cpp 
set SourceListBenchmark=%SourceListBenchmark% ../Core/DataStructures/HashMap.cpp 
set CompileDefines=/DENABLE_MEM_TRACKING 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsBenchmark=/FdTinkerBenchmark.pdb
    set DebugLinkFlagsBenchmark=/pdb:TinkerBenchmark.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    ) else (
    set DebugCompileFlagsBenchmark=/FdTinkerBenchmark.pdb
    set DebugLinkFlagsBenchmark=/pdb:TinkerBenchmark.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=0
    )

set CompileIncludePaths=/I ../Core 

set OBJDir=%cd%\obj_benchmark\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerBenchmark.exe...
cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsBenchmark% %SourceListBenchmark% /link %CommonLinkFlags% Winmm.lib %DebugLinkFlagsBenchmark% /out:TinkerBenchmark.exe

:DoneBuild
popd
popd
