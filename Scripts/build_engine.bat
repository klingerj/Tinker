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
echo build_engine.bat Release VK
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

echo ***** Building Tinker App *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerApp.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
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
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Raytracing/RayIntersection.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Tools/ShaderCompiler/ShaderCompiler.cpp 

set SourceListGraphics=
set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Common/GraphicsCommon.cpp 
set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Common/ShaderManager.cpp 
set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Common/VirtualTexture.cpp 
if "%GraphicsAPI%" == "VK" (
    set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/Vulkan.cpp 
    set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanCmds.cpp 
    set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanTypes.cpp 
    set SourceListGraphics=!SourceListGraphics! %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanCreation.cpp 
)
set SourceListApp=%SourceListApp% %SourceListGraphics%

if "%GraphicsAPI%" == "D3D12" ( echo No source files available for D3D12. )

rem Calculate absolute path prefix for application path parameters here
set AbsolutePathPrefix=%AbsolutePathPrefix:\=\\%
set CompileDefines=/DTINKER_APP 
set CompileDefines=%CompileDefines% /DASSERTS_ENABLE=1 
set CompileDefines=%CompileDefines% /DTINKER_EXPORTING 
set CompileDefines=%CompileDefines% /DENABLE_MEM_TRACKING 
set CompileDefines=%CompileDefines% /D_GAME_DLL_PATH=%AbsolutePathPrefix%\\TinkerGame.dll 
set CompileDefines=%CompileDefines% /D_GAME_DLL_HOTLOADCOPY_PATH=%AbsolutePathPrefix%\\TinkerGame_hotload.dll 
set CompileDefines=%CompileDefines% /D_SHADERS_SPV_DIR=%AbsolutePathPrefix%\\..\\Shaders\\spv\\ 
set CompileDefines=%CompileDefines% /D_SHADERS_SRC_DIR=%AbsolutePathPrefix%\\..\\Shaders\\hlsl\\ 
set CompileDefines=%CompileDefines% /D_SCRIPTS_DIR=%AbsolutePathPrefix%\\..\\Scripts\\ 

if "%GraphicsAPI%" == "VK" (
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

set CompileIncludePaths=/I ../Core /I ../Tools /I ../ThirdParty/dxc_2022_07_18 
set LibsToLink=user32.lib ws2_32.lib 
set LibsToLink=%LibsToLink% ../ThirdParty/dxc_2022_07_18/lib/x64/dxcompiler.lib 

echo.
echo Building TinkerApp.exe...


if "%GraphicsAPI%" == "VK" (
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

rem Copy needed DLLs to exe directory
echo.
echo Copying required dlls dxcompiler.dll and dxil.dll to exe dir...
copy ..\ThirdParty\dxc_2022_07_18\bin\x64\dxcompiler.dll 
copy ..\ThirdParty\dxc_2022_07_18\bin\x64\dxil.dll 
echo Done.

:DoneBuild
echo.
popd
popd

:EndScript
