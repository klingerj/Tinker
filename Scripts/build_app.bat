@echo off
setlocal

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
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm- /GS- /fp:fast /Zi
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
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Vulkan/Vulkan.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanCmds.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanTypes.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Vulkan/VulkanGPUMemAllocator.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Common/GraphicsCommon.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Common/ShaderManager.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Graphics/Common/VirtualTexture.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Math/VectorTypes.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/FileLoading.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/DataStructures/Vector.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/DataStructures/HashMap.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Utility/MemTracker.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Mem.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Raytracing/RayIntersection.cpp 
set SourceListApp=%SourceListApp% %AbsolutePathPrefix%/../Core/Raytracing/AccelStructures/Octree.cpp 

rem Calculate absolute path prefix for application path parameters here
set AbsolutePathPrefix=%AbsolutePathPrefix:\=\\%
set CompileDefines=/DTINKER_APP /DTINKER_EXPORTING /D_GAME_DLL_PATH=%AbsolutePathPrefix%\\TinkerGame.dll /D_SHADERS_SPV_DIR=%AbsolutePathPrefix%\\..\\Shaders\\spv\\ /D_SCRIPTS_DIR=%AbsolutePathPrefix%\\..\\Scripts\\ /DASSERTS_ENABLE=1 /DVULKAN 

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
set LibsToLink=user32.lib ws2_32.lib 

echo.
echo Building TinkerApp.exe...

rem Vulkan
set VulkanVersion=1.2.141.2
echo Using Vulkan v%VulkanVersion%
echo.
set VulkanPath=C:\VulkanSDK\%VulkanVersion%

set CompileIncludePaths=%CompileIncludePaths% /I %VulkanPath%/Include
set LibsToLink=%LibsToLink% %VulkanPath%\Lib\vulkan-1.lib

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
popd
popd

rem Run game build script
call build_game.bat %BuildConfig%
echo.
