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
echo ***** Building Tinker Tests *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerTest.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi
set CommonLinkFlags=/incremental:no /opt:ref /DEBUG

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )

set CompileIncludePaths=/I ../ 

rem *********************************************************************************************************
rem TinkerTest - unit testing
set SourceListTest=../Platform/Win32PlatformGameAPI.cpp ../Test/TestMain.cpp
set CompileDefines= 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsTest=/FdTinkerTest.pdb
    set DebugLinkFlagsTest=/pdb:TinkerTest.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    ) else (
    set DebugCompileFlagsTest=/FdTinkerTest.pdb
    set DebugLinkFlagsTest=/pdb:TinkerTest.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    )

set CompileIncludePaths=%CompileIncludePaths% /I ../Test 

set OBJDir=%cd%\obj_test\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerTest.exe...
cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsTest% %SourceListTest% /link %CommonLinkFlags% TinkerApp.lib %DebugLinkFlagsTest% /out:TinkerTest.exe

:DoneBuild
popd
popd
