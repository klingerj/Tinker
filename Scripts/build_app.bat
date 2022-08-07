@echo off
setlocal
setlocal enabledelayedexpansion

if "%1" == "-h" (goto PrintHelp)
if "%1" == "-help" (goto PrintHelp)
if "%1" == "help" (goto PrintHelp)
goto StartScript

:PrintHelp
echo Usage: build_app.bat ^<build_mode^> ^<graphics_api^>
echo.
echo build_mode:
echo   Release
echo   Debug
echo.
echo graphics_api
echo   VK (uses VULKAN_SDK environment variable)
echo.
echo For example:
echo build_app.bat Release VK
echo.
goto EndScript

:StartScript
set BuildConfig=%1
set GraphicsAPI=%2
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

if "%GraphicsAPI%" == "VK" (
	set GraphicsAPIChosen="VK"
    ) else (
        if "%GraphicsAPI%" == "D3D12" (
	    set GraphicsAPIChosen="D3D12"
        ) else (
	    set GraphicsAPIChosen="None"
	    echo Unsupported graphics API specified.
        goto DoneBuild
        )
    )

if %GraphicsAPIChosen% == "VK" (
    rem Vulkan
    echo Using Vulkan SDK: %VULKAN_SDK%
    echo.
) else (
    if %GraphicsAPIChosen% == "D3D12" (
        rem echo D3D12 not yet supported. Build canceled.
        rem echo.
        rem goto DoneBuild
    )
)

echo ***** Building Tinker App *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerApp.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi /FS
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
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Platform/Win32Client.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Math/VectorTypes.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/AssetFileParsing.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/DataStructures/Vector.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/DataStructures/HashMap.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Utility/MemTracker.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Mem.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Raytracing/RayIntersection.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Raytracing/AccelStructures/Octree.cpp 

set SourceListVulkan=
if %GraphicsAPIChosen% == "VK" (
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/Vulkan.cpp 
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanCmds.cpp 
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanTypes.cpp 
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanCreation.cpp 
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Common/GraphicsCommon.cpp 
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Common/ShaderManager.cpp 
    set SourceListVulkan=!SourceListVulkan! %AbsolutePathPrefix%/../Core/Graphics/Common/VirtualTexture.cpp 
)
set SourceListApp=%SourceListApp% %SourceListVulkan%

if %GraphicsAPIChosen% == "D3D12" ( echo No source files available for D3D12. )

rem Calculate absolute path prefix for application path parameters here
set AbsolutePathPrefix=%AbsolutePathPrefix:\=\\%
set CompileDefines=/DTINKER_APP /DTINKER_EXPORTING /D_GAME_DLL_PATH=%AbsolutePathPrefix%\\TinkerGame.dll /D_GAME_DLL_HOTLOADCOPY_PATH=%AbsolutePathPrefix%\\TinkerGame_hotload.dll /D_SHADERS_SPV_DIR=%AbsolutePathPrefix%\\..\\Shaders\\spv\\ /D_SCRIPTS_DIR=%AbsolutePathPrefix%\\..\\Scripts\\ /DASSERTS_ENABLE=1 

if %GraphicsAPIChosen% == "VK" (
    set CompileDefines=!CompileDefines! /DVULKAN 
)

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsApp=/FdTinkerApp.pdb
    set DebugLinkFlagsApp=/pdb:TinkerApp.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=!CompileDefines!
    ) else (
    set DebugCompileFlagsApp=/FdTinkerApp.pdb
    set DebugLinkFlagsApp=/pdb:TinkerApp.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=!CompileDefines!
    )

set CompileIncludePaths=/I ../Core 
set LibsToLink=user32.lib ws2_32.lib 

echo.
echo Building TinkerApp.exe...


if %GraphicsAPIChosen% == "VK" (
    set CompileIncludePaths=!CompileIncludePaths! /I %VULKAN_SDK%/Include 
    set LibsToLink=!LibsToLink! %VULKAN_SDK%\Lib\vulkan-1.lib
)

set OBJDir=%cd%\obj_app\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsApp% %SourceListApp% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsApp% /out:TinkerApp.exe

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

rem Run game build script
call build_game.bat %BuildConfig%
echo.

:EndScript
