@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified. Must specify 'Release' or 'Debug'
        pause
        goto DoneBuild
        )
    )

echo ***** Building Tinker Game *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerGame*.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi
set CommonLinkFlags=/incremental:no /opt:ref /DEBUG

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Zi /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )

set CompileIncludePaths=/I ../Core 

rem *********************************************************************************************************
rem TinkerGame - shared library
set SourceListGame=../Game/GameMain.cpp ../Game/GraphicsTypes.cpp ../Game/AssetManager.cpp ../Game/RenderPass.cpp ../Game/Raytracing.cpp ../Game/View.cpp ../Game/Camera.cpp ../Game/InputManager.cpp
set CompileDefines=/DTINKER_GAME /D_ASSETS_DIR=..\\Assets\\ 

if "%TIME:~0,1%" == " " (
    set BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__0%TIME:~1,1%_%TIME:~3,2%_%TIME:~6,2%
    ) else (
        set BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
        )
set GameDllPdbName=TinkerGame_%BuildTimestamp%.pdb

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsGame=/Fd%GameDllPdbName%
    set DebugLinkFlagsGame=/pdb:%GameDllPdbName% /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    ) else (
    set DebugCompileFlagsGame=/Fd%GameDllPdbName%
    set DebugLinkFlagsGame=/pdb:%GameDllPdbName% /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    )

set OBJDir=%cd%\obj_game\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerGame.dll...
cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsGame% %SourceListGame% /link %CommonLinkFlags% TinkerApp.lib /DLL /export:GameUpdate /export:GameDestroy /export:GameWindowResize %DebugLinkFlagsGame% /out:TinkerGame.dll

rem Delete unnecessary files
echo.
if EXIST TinkerGame.lib (
    echo Deleting unnecessary file TinkerGame.lib
    del TinkerGame.lib
    )
if EXIST TinkerGame.exp (
    echo Deleting unnecessary file TinkerGame.exp
    echo.
    del TinkerGame.exp
    )

:DoneBuild
popd
popd
