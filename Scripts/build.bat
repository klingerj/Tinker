@echo off

pushd ..
IF NOT EXIST .\Build mkdir .\Build
pushd .\Build
del *.pdb > NUL 2> NUL

REM *********************************************************************************************************
SET CommonCompileFlags=/nologo /Zi /O2 /std:c++17 /MT /W4 /WX /wd4201 /wd4324 /wd4100 /wd4189 /EHsc /GR- /Gm-
SET CommonLinkFlags=/incremental:no /opt:ref

REM *********************************************************************************************************
REM TinkerCore - static library
SET SourceListCore=..\Core\Math\VectorTypes.cpp
cl /c %CommonCompileFlags% /FdTinkerCore.pdb %SourceListCore%
lib user32.lib /machine:x64 /Wx /out:TinkerCore.lib /nologo

REM *********************************************************************************************************
REM TinkerPlatform - primary exe
SET SourceListPlatform=../Platform/Win32Layer.cpp ../Platform/Win32PlatformGameAPI.cpp
cl %CommonCompileFlags% /FdTinkerPlatform.pdb %SourceListPlatform% /link %CommonLinkFlags% /out:TinkerPlatform.exe

REM *********************************************************************************************************
REM TinkerGame - shared library
SET SourceListGame=../Game/GameMain.cpp ../Platform/Win32PlatformGameAPI.cpp

IF "%TIME:~0,1%" == " " (
    SET BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__0%TIME:~1,1%_%TIME:~3,2%_%TIME:~6,2%
    ) ELSE (
        SET BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
        )
SET GameDllPdbName=TinkerGame_%BuildTimestamp%.pdb
cl %CommonCompileFlags% /Fd%GameDllPdbName% %SourceListGame% /link %CommonLinkFlags% TinkerCore.lib /DLL /export:GameUpdate /PDB:%GameDllPdbName% /out:TinkerGame.dll

REM *********************************************************************************************************
REM TinkerTest - unit testing
SET SourceListTest=../Platform/Win32PlatformGameAPI.cpp ../Test/TestMain.cpp ../Core/Math/VectorTypes.cpp
cl %CommonCompileFlags% %SourceListTest% /link %CommonLinkFlags% TinkerCore.lib /out:TinkerTest.exe

REM *********************************************************************************************************
REM TinkerBenchmark - benchmarking
SET SourceListBenchmark=../Platform/Win32PlatformGameAPI.cpp ../Benchmark/BenchmarkMain.cpp ../Benchmark/MathBenchmarks/VectorTypeBenchmarks.cpp ../Core/Math/VectorTypes.cpp
cl %CommonCompileFlags% %SourceListBenchmark% /link %CommonLinkFlags% TinkerCore.lib /out:TinkerBenchmark.exe

popd
popd
