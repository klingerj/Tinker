@echo off

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

rem Run game build script
call build_game.bat %BuildConfig%
echo.

echo ***** Building Tinker Platform *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerPlatform.pdb > NUL 2> NUL

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
set SourceListPlatform=../Platform/Win32Layer.cpp ../Platform/Win32PlatformGameAPI.cpp ../Platform/Win32Vulkan.cpp ../Platform/Win32Logging.cpp ../Platform/Win32Client.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsPlatform=/FdTinkerPlatform.pdb
    set DebugLinkFlagsPlatform=/pdb:TinkerPlatform.pdb
    ) else (
    set DebugCompileFlagsPlatform=
    set DebugLinkFlagsPlatform=
    )

echo.
echo Building TinkerPlatform.exe...

rem Vulkan
set VulkanVersion=1.2.141.2
echo Using Vulkan v%VulkanVersion%
echo.
set VulkanPath="C:\VulkanSDK"\%VulkanVersion%

set CompileIncludePaths=%VulkanPath%\Include
set LibsToLink=user32.lib ws2_32.lib %VulkanPath%\Lib\vulkan-1.lib

set OBJDir=%cd%\obj_platform\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% /I %CompileIncludePaths% %DebugCompileFlagsPlatform% %SourceListPlatform% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsPlatform% /out:TinkerPlatform.exe

:DoneBuild
popd
popd
