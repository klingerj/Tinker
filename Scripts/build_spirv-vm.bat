@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

echo ***** Building SPIR-V VM *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del SpirvVM.pdb > NUL 2> NUL

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
rem SpirvVM - primary exe
rem ../SPIR-V-VM/VMShader.cpp
set SourceList=../SPIR-V-VM/SpirvVM.c ../SPIR-V-VM/VMState.c ../SPIR-V-VM/VMShader.c ../SPIR-V-VM/SpirvOps.c ../SPIR-V-VM/Main.c

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlags=/FdSpirvVM.pdb
    set DebugLinkFlags=/pdb:SpirvVM.pdb
    ) else (
    set DebugCompileFlags=
    set DebugLinkFlags=
    )

echo.
echo Building SpirvVM.exe...

set CompileIncludePaths=
set LibsToLink=user32.lib

set OBJDir=%cd%\obj_SpirvVM\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% %DebugCompileFlags% %SourceList% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlags% /out:SpirvVM.exe

:DoneBuild
popd
popd
