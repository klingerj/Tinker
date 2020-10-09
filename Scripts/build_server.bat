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

echo.
echo ***** Building Tinker Server *****

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi
set CommonLinkFlags=/incremental:no /opt:ref /DEBUG

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Zi /Od /MTd /fp:fast
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT /fp:fast
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

set OBJDir=%cd%\obj_server\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

rem /I %CompileIncludePaths%
cl %CommonCompileFlags% %DebugCompileFlagsServer% %SourceListServer% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsServer% /out:TinkerServer.exe 

:DoneBuild
popd
popd
