@echo off
setlocal

if "%1" == "-h" (goto PrintHelp)
if "%1" == "-help" (goto PrintHelp)
if "%1" == "help" (goto PrintHelp)
goto StartScript

:PrintHelp
echo Usage: build_engine.bat ^<build_mode^>
echo.
echo build_mode:
echo   Release
echo   Debug
echo.
echo For example:
echo build_engine.bat Release 
echo.
goto EndScript

:StartScript
set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

echo ***** Building Tinker App *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerApp.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++20 /GL /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi /FS
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
rem TinkerApp - primary exe
set AbsolutePathPrefix=%cd%

set SourceListApp= 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32Layer.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32WorkerThreadPool.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32PlatformGameAPI.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32Logging.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32File.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32Client.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Math/VectorTypes.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/AssetFileParsing.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/DataStructures/Vector.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/DataStructures/HashMap.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Utility/MemTracker.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Mem.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../ThirdParty/imgui-docking/imgui.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../ThirdParty/imgui-docking/imgui_draw.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../ThirdParty/imgui-docking/imgui_tables.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../ThirdParty/imgui-docking/imgui_widgets.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../ThirdParty/imgui-docking/backends/imgui_impl_win32.cpp 

rem Create unity build file will all cpp files included
set "SourceListApp=%SourceListApp:\=/%" rem convert backslashes to forward slashes
set UnityBuildCppFile=AppUnityBuildFile.cpp
set INCLUDE_PREFIX=#include

echo Deleting old unity build cpp source file %UnityBuildCppFile%.
if exist %UnityBuildCppFile% del %UnityBuildCppFile%
echo ...Done.
echo.

echo Source files included in build:
for %%i in (%SourceListApp%) do (
    echo %%i
    echo %INCLUDE_PREFIX% "%%i" >> %UnityBuildCppFile%
)
echo Generated: %UnityBuildCppFile%
rem 

rem Defines 
set CompileDefines=/DTINKER_APP 
set CompileDefines=%CompileDefines% /DASSERTS_ENABLE=1 
set CompileDefines=%CompileDefines% /DTINKER_EXPORTING 
set CompileDefines=%CompileDefines% /DENABLE_MEM_TRACKING 
set CompileDefines=%CompileDefines% /D_GAME_DLL_PATH=TinkerGame.dll 
set CompileDefines=%CompileDefines% /D_GAME_DLL_HOTLOADCOPY_PATH=TinkerGame_hotload.dll 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsApp=/FdTinkerApp.pdb
    set DebugLinkFlagsApp=/pdb:TinkerApp.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%
    ) else (
    set DebugCompileFlagsApp=/FdTinkerApp.pdb
    set DebugLinkFlagsApp=/pdb:TinkerApp.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%
    )

set CompileIncludePaths=/I ../Core 
set CompileIncludePaths=%CompileIncludePaths% /I ../ThirdParty/imgui-docking 
set LibsToLink=user32.lib ws2_32.lib Shlwapi.lib 

echo.
echo Building TinkerApp.exe...

set OBJDir=%cd%\obj_app\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsApp% %UnityBuildCppFile% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsApp% /out:TinkerApp.exe

echo.
if EXIST TinkerApp.exp (
    echo Deleting unnecessary file TinkerApp.exp
    echo.
    del TinkerApp.exp
    )

:DoneBuild
echo.
popd
popd

:EndScript
