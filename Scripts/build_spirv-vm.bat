@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

rem Run core engine build script
call build_core_engine_lib.bat %BuildConfig%
echo.

echo ***** Building SPIR-V VM *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del spirv-vm.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /wd4996 /EHa- /GR- /Gm- /GS- /Zi
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
rem spirv-vm - primary exe
set SourceList=../SPIR-V-VM/spirv-vm.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlags=/Fdspirv-vm.pdb
    set DebugLinkFlags=/pdb:spirv-vm.pdb
    ) else (
    set DebugCompileFlags=
    set DebugLinkFlags=
    )

echo.
echo Building spirv-vm.exe...

set CompileIncludePaths=
set LibsToLink=user32.lib

set OBJDir=%cd%\obj_spirv-vm\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% %DebugCompileFlags% %SourceList% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlags% /out:spirv-vm.exe

:DoneBuild
popd
popd
