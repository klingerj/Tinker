@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

echo ***** Building Tinker Core Engine *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerCore.pdb > NUL 2> NUL

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
rem TinkerCore - static library
set SourceListCore=../Core/Math/VectorTypes.cpp ../Core/FileIO/FileLoading.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsCore=/FdTinkerCore.pdb
    ) else (
    set DebugCompileFlagsCore=
    )

set OBJDir=%cd%\obj_core_engine\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%
set OBJListCore= %OBJDir%VectorTypes.obj %OBJDir%FileLoading.obj

echo.
echo Building TinkerCore.lib...
cl /c %CommonCompileFlags% %DebugCompileFlagsCore% %SourceListCore%
lib /machine:x64 /Wx /out:TinkerCore.lib /nologo %OBJListCore%

:DoneBuild
popd
popd
