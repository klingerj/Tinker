@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

rem Run game build script
rem call build_game.bat %BuildConfig%
echo.

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
rem TinkerPlatform - primary exe
set SourceListPlatform=../Platform/Win32Layer.cpp ../Platform/Win32PlatformGameAPI.cpp ../Platform/Graphics/GraphicsCommon.cpp ../Platform/ShaderManager.cpp ../Platform/Graphics/Vulkan.cpp ../Platform/Graphics/VulkanCmds.cpp ../Platform/Graphics/VulkanTypes.cpp ../Platform/Graphics/VulkanGPUMemAllocator.cpp ../Platform/Win32Logging.cpp ../Platform/Win32Client.cpp
set CompileDefines=/D_SHADERS_SPV_DIR=..\\Shaders\\spv\\ /D_SCRIPTS_DIR=..\\Scripts\\ 

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsPlatform=/FdTinkerPlatform.pdb
    set DebugLinkFlagsPlatform=/pdb:TinkerPlatform.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1 /D_VULKAN 
    ) else (
    set DebugCompileFlagsPlatform=/FdTinkerPlatform.pdb
    set DebugLinkFlagsPlatform=/pdb:TinkerPlatform.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1 /D_VULKAN 
    )

set CompileIncludePaths=/I ../ 
set LibsToLink=user32.lib ws2_32.lib 

echo.
echo Building TinkerPlatform.exe...

rem Vulkan
set VulkanVersion=1.2.141.2
echo Using Vulkan v%VulkanVersion%
echo.
set VulkanPath=C:\VulkanSDK\%VulkanVersion%

set CompileIncludePaths=%CompileIncludePaths% /I %VulkanPath%/Include
set LibsToLink=%LibsToLink% %VulkanPath%\Lib\vulkan-1.lib

set OBJDir=%cd%\obj_platform\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsPlatform% %SourceListPlatform% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsPlatform% /out:TinkerApp.exe

if EXIST TinkerApp.exp (
    echo Deleting unnecessary file TinkerApp.exp
    del TinkerApp.exp
    )

:DoneBuild
popd
popd
