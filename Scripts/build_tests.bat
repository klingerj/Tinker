@echo off

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerTest.pdb > NUL 2> NUL

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

rem *********************************************************************************************************
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4201 /wd4324 /wd4100 /wd4189 /EHsc /GR- /Gm-
set CommonLinkFlags=/incremental:no /opt:ref

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Zi /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )

rem *********************************************************************************************************
rem TinkerCore - static library
set SourceListCore=..\Core\Math\VectorTypes.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsCore=/FdTinkerCore.pdb
    ) else (
    set DebugCompileFlagsCore=
    )
echo.
echo Building TinkerCore.lib...
cl /c %CommonCompileFlags% %DebugCompileFlagsCore% %SourceListCore% /Fo:TinkerCore.obj
lib /verbose /machine:x64 /Wx /out:TinkerCore.lib /nologo TinkerCore.obj

rem *********************************************************************************************************
rem TinkerTest - unit testing
set SourceListTest=../Platform/Win32PlatformGameAPI.cpp ../Test/TestMain.cpp ../Core/Math/VectorTypes.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsTest=/FdTinkerTest.pdb
    set DebugLinkFlagsTest=/pdb:TinkerTest.pdb
    ) else (
    set DebugCompileFlagsTest=
    set DebugLinkFlagsTest=
    )
echo.
echo Building TinkerTest.exe...
cl %CommonCompileFlags% %DebugCompileFlagsTest% %SourceListTest% /link %CommonLinkFlags% TinkerCore.lib %DebugLinkFlagsTest% /out:TinkerTest.exe

:DoneBuild
popd
popd
