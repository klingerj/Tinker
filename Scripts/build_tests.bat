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
echo ***** Building Tinker Tests *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerTest.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++20 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /wd4702 /EHa- /GR- /Gm- /GS- /fp:fast /Zi /FS
set CommonLinkFlags=/incremental:no /opt:ref /DEBUG

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )

set CompileIncludePaths=/I ../Core 

rem *********************************************************************************************************
rem TinkerTest - unit testing
set SourceListTest=../Test/TestMain.cpp 
set SourceListTest=%SourceListTest% ../Core/Platform/Win32PlatformGameAPI.cpp 
set SourceListTest=%SourceListTest% ../Core/Utility/MemTracker.cpp 
set SourceListTest=%SourceListTest% ../Core/DataStructures/Vector.cpp 
set SourceListTest=%SourceListTest% ../Core/DataStructures/HashMap.cpp 
set SourceListTest=%SourceListTest% ../Core/Mem.cpp 
rem set CompileDefines=/DENABLE_MEM_TRACKING 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsTest=/FdTinkerTest.pdb
    set DebugLinkFlagsTest=/pdb:TinkerTest.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    ) else (
    set DebugCompileFlagsTest=/FdTinkerTest.pdb
    set DebugLinkFlagsTest=/pdb:TinkerTest.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    )

set CompileIncludePaths=%CompileIncludePaths% /I ../Test 

set OBJDir=%cd%\obj_test\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerTest.exe...
cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsTest% %SourceListTest% /link dbghelp.lib %CommonLinkFlags% %DebugLinkFlagsTest% /out:TinkerTest.exe

:DoneBuild
popd
popd
