@echo off

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerServer.pdb > NUL 2> NUL

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

rem *********************************************************************************************************
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm-
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
rem TinkerServer
set SourceListServer=../Platform/Win32Server.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsServer=/FdTinkerServer.pdb
    set DebugLinkFlagsServer=/pdb:TinkerServer.pdb
    ) else (
    set DebugCompileFlagsServer=
    set DebugLinkFlagsServer=
    )

echo.
echo Building TinkerServer.exe...

rem set CompileIncludePaths=""
set LibsToLink=user32.lib ws2_32.lib

echo %SourceListServer%
rem /I %CompileIncludePaths%
cl %CommonCompileFlags% %DebugCompileFlagsServer% %SourceListServer% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsServer% /out:TinkerServer.exe 

:DoneBuild
popd
popd
