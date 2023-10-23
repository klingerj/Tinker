@echo off
setlocal
setlocal enabledelayedexpansion

if "%1" == "-h" (goto PrintHelp)
if "%1" == "-help" (goto PrintHelp)
if "%1" == "help" (goto PrintHelp)
goto StartScript

:PrintHelp
echo Usage: build_engine.bat ^<build_mode^> ^<graphics_api^>
echo.
echo build_mode:
echo   Release
echo   Debug
echo.
echo graphics_api
echo   VK (uses VULKAN_SDK environment variable)
echo   D3D12
echo.
echo For example:
echo build_game.bat Release VK
echo.
goto EndScript

:StartScript
set BuildConfig=%1
set GraphicsAPI=%2

if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified. Must specify 'Release' or 'Debug'
        pause
        goto DoneBuild
        )
    )

if "%GraphicsAPI%" == "VK" (
    rem Vulkan
    echo Using Vulkan SDK: %VULKAN_SDK%
    echo.
    ) else (
        if "%GraphicsAPI%" == "D3D12" (
        echo D3D12 not yet supported. Build canceled.
        echo.
        goto DoneBuild
        ) else (
	    set GraphicsAPI="None"
	    echo Unsupported graphics API specified.
        goto DoneBuild
        )
    )

echo ***** Building Tinker Game *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
set AbsolutePathPrefix=%cd%/
set "AbsolutePathPrefix=%AbsolutePathPrefix:\=/%" 
pushd .\Build
del TinkerGame*.pdb > NUL 2> NUL

:: Features
set "EnableMemTracking=1"

rem *********************************************************************************************************
set CommonCompileFlags=/nologo /std:c++20 /GL /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi /FS
set CommonLinkFlags=/incremental:no /opt:ref /DEBUG

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Zi /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )
echo.

set CompileIncludePaths=/I ../ 
set CompileIncludePaths=%CompileIncludePaths% /I ../Core 
set CompileIncludePaths=%CompileIncludePaths% /I ../Tools 
set CompileIncludePaths=%CompileIncludePaths% /I ../DebugUI 
set CompileIncludePaths=%CompileIncludePaths% /I ../ThirdParty/dxc_2022_07_18 
set CompileIncludePaths=%CompileIncludePaths% /I ../ThirdParty/xxHash-0.8.2 
set CompileIncludePaths=%CompileIncludePaths% /I ../ThirdParty/imgui-docking 

rem *********************************************************************************************************
rem TinkerGame - shared library

set SourceListGame= 
rem Glob all files in desired directories
for /r %AbsolutePathPrefix%Game/ %%i in (*.cpp) do set SourceListGame=!SourceListGame! %%i 
for /r %AbsolutePathPrefix%DebugUI/ %%i in (*.cpp) do set SourceListGame=!SourceListGame! %%i 
for /r %AbsolutePathPrefix%Graphics/Common/ %%i in (*.cpp) do set SourceListGame=!SourceListGame! %%i 
for /r %AbsolutePathPrefix%Graphics/CPURaytracing/ %%i in (*.cpp) do set SourceListGame=!SourceListGame! %%i 

if "%GraphicsAPI%" == "VK" (
    for /r %AbsolutePathPrefix%Graphics/Vulkan/ %%i in (*.cpp) do set SourceListGame=!SourceListGame! %%i 
)

set SourceListGame=%SourceListGame% %AbsolutePathPrefix%Tools/ShaderCompiler/ShaderCompiler.cpp 

rem Don't glob third party folders right now
set SourceListGame=%SourceListGame% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui.cpp 
set SourceListGame=%SourceListGame% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_demo.cpp 
set SourceListGame=%SourceListGame% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_draw.cpp 
set SourceListGame=%SourceListGame% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_tables.cpp 
set SourceListGame=%SourceListGame% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_widgets.cpp 
if "%GraphicsAPI%" == "D3D12" ( echo No source files available for D3D12. )

rem Create unity build file will all cpp files included
set "SourceListGame=%SourceListGame:\=/%" rem convert backslashes to forward slashes
set UnityBuildCppFile=GameUnityBuildFile.cpp
set INCLUDE_PREFIX=#include

echo Deleting old unity build cpp source file %UnityBuildCppFile%.
if exist %UnityBuildCppFile% del %UnityBuildCppFile%
echo ...Done.
echo.

echo Source files included in build:
for %%i in (%SourceListGame%) do (
    echo %%i
    echo %INCLUDE_PREFIX% "%%i" >> %UnityBuildCppFile%
)
echo Generated: %UnityBuildCppFile%
rem 

rem Defines 
set CompileDefines=/DTINKER_GAME 
set CompileDefines=%CompileDefines% /DASSERTS_ENABLE=1 
if "%EnableMemTracking%" == "1" (
    set CompileDefines=%CompileDefines% /DENABLE_MEM_TRACKING 
)
set CompileDefines=%CompileDefines% /D_ASSETS_DIR=..\\Assets\\ 
set CompileDefines=%CompileDefines% /D_COOKED_ASSETS_DIR=..\\CookedAssets\\ 
set CompileDefines=%CompileDefines% /D_SHADERS_SPV_DIR=..\\Shaders\\spv\\ 
set CompileDefines=%CompileDefines% /D_SHADERS_SRC_DIR=..\\Shaders\\hlsl\\ 
if "%GraphicsAPI%" == "VK" (
    set CompileDefines=!CompileDefines! /DVULKAN 
)

if "%TIME:~0,1%" == " " (
    set BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__0%TIME:~1,1%_%TIME:~3,2%_%TIME:~6,2%
    ) else (
        set BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
        )
set GameDllPdbName=TinkerGame_%BuildTimestamp%.pdb

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsGame=/Fd%GameDllPdbName%
    set DebugLinkFlagsGame=/pdb:%GameDllPdbName% /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=!CompileDefines! 
    ) else (
    set DebugCompileFlagsGame=/Fd%GameDllPdbName%
    set DebugLinkFlagsGame=/pdb:%GameDllPdbName% /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=!CompileDefines! 
    )

set LibsToLink=TinkerApp.lib 
set LibsToLink=%LibsToLink% ../ThirdParty/dxc_2022_07_18/lib/x64/dxcompiler.lib 
if "%GraphicsAPI%" == "VK" (
    set CompileIncludePaths=!CompileIncludePaths! /I %VULKAN_SDK%/Include 
    set LibsToLink=!LibsToLink! %VULKAN_SDK%/Lib/vulkan-1.lib
)

set OBJDir=%cd%\obj_game\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerGame.dll...
cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsGame% %UnityBuildCppFile% /link %CommonLinkFlags% %LibsToLink% /DLL /export:GameUpdate /export:GameDestroy /export:GameWindowResize %DebugLinkFlagsGame% /out:TinkerGame.dll

rem Copy needed dependency DLLs to exe directory
echo.
echo Copying required dlls dxcompiler.dll and dxil.dll to exe dir...
copy ..\ThirdParty\dxc_2022_07_18\bin\x64\dxcompiler.dll
copy ..\ThirdParty\dxc_2022_07_18\bin\x64\dxil.dll
echo Done.

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
