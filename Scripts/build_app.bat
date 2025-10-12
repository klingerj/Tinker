@echo off
setlocal
setlocal enabledelayedexpansion

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
echo.
echo ***** Building Tinker App *****

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

:: Build settings
set "EnableUnityBuild=0"

:: Features
:: set "EnableMemTracking=1"

pushd ..
if NOT EXIST .\Build mkdir .\Build
set AbsolutePathPrefix=%cd%/
set "AbsolutePathPrefix=%AbsolutePathPrefix:\=/%" 
pushd .\Build
del TinkerApp.pdb > NUL 2> NUL

:: *********************************************************************************************************
:: /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++20 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi /FS
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
set CompileIncludePaths=%CompileIncludePaths% /I ../Core 
set CompileIncludePaths=%CompileIncludePaths% /I ../ThirdParty/imgui-docking 

:: *********************************************************************************************************
:: TinkerApp - primary exe

set SourceListApp= 
:: Glob all files in desired directories
for /r %AbsolutePathPrefix%Core/ %%i in (*.cpp) do set SourceListApp=!SourceListApp! %%i 

:: Don't glob third party folders right now
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_draw.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_tables.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%ThirdParty/imgui-docking/imgui_widgets.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%ThirdParty/imgui-docking/backends/imgui_impl_win32.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%ThirdParty/xxHash-0.8.2/xxhash.c 

:: Create unity build file will all cpp files included
set "SourceListApp=%SourceListApp:\=/%" :: convert backslashes to forward slashes
set UnityBuildCppFile=AppUnityBuildFile.cpp
set INCLUDE_PREFIX=#include
if "%EnableUnityBuild%" == "1" (
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
)

:: Defines 
set CompileDefines=/DTINKER_APP 
set CompileDefines=%CompileDefines% /DASSERTS_ENABLE=1 
set CompileDefines=%CompileDefines% /DTINKER_EXPORTING 
if "%EnableMemTracking%" == "1" (
    set CompileDefines=%CompileDefines% /DENABLE_MEM_TRACKING 
)
set CompileDefines=%CompileDefines% /D_ENGINE_ROOT_PATH=%AbsolutePathPrefix% 
set CompileDefines=%CompileDefines% /D_GAME_DLL_PATH=TinkerGame.dll 
set CompileDefines=%CompileDefines% /D_GAME_DLL_HOTLOADCOPY_PATH=TinkerGame_hotload.dll 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlags=/FdTinkerApp.pdb
    set DebugLinkFlags=/pdb:TinkerApp.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%
    ) else (
    set DebugCompileFlags=/FdTinkerApp.pdb
    set DebugLinkFlags=/pdb:TinkerApp.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=%CompileDefines%
    )

set LibsToLink=user32.lib ws2_32.lib Shlwapi.lib 
if "%EnableMemTracking%" == "1" (
    set LibsToLink=%LibsToLink% dbghelp.lib 
)

set OBJDir=%cd%\obj_app\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

if "%EnableUnityBuild%" == "1" (
    set SourceFiles=%UnityBuildCppFile% 
) else (
    set SourceFiles=%SourceListApp% 
)

echo.
echo where cl.exe: 
where cl.exe
echo where link.exe: 
where link.exe

echo.
echo Building TinkerApp.exe...
:: Kick off compile commands in parallel 
for %%i in (%SourceFiles%) do start "Compile_TinkerApp" /d %cd% /b cmd /c call ..\Scripts\compile_single_file.bat "%CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlags% %%i" 

:: Wait for each single file compile task to finish
:waitloop
:: NOTE: it is possible that this could be kept waiting by other instances of cl.exe on the machine
:: But get process info from within batch is extremely annoying/slow.
tasklist /fi "imagename eq cl.exe" | findstr /i "cl.exe" > NUL

if %errorlevel% == 0 (
    goto waitloop
) else (
    echo Done compiling.
)
:: nonzero error level means no subprocesses found -> done compiling.

:: Now gather obj files and link
set ObjList= 
for /r %OBJDir% %%i in (*.obj) do set ObjList=!ObjList! %%i 
echo Linking...
link.exe /WX /MACHINE:X64 %ObjList% %LibsToLink% %CommonLinkFlags% %DebugLinkFlags% /out:TinkerApp.exe
echo Done linking.

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
