@echo off

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerCore.pdb > NUL 2> NUL
del TinkerPlatform.pdb > NUL 2> NUL
del TinkerGame*.pdb > NUL 2> NUL

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

rem *********************************************************************************************************
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4201 /wd4324 /wd4100 /wd4189 /EHa- /GR- /Gm-
set CommonLinkFlags=/incremental:no /opt:ref

if "%BuildConfig%" == "Debug" (
    echo Debug mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /Zi /Od /MTd
    set CommonLinkFlags=%CommonLinkFlags% /debug:full
    ) else (
    echo Release mode specified.
    set CommonCompileFlags=%CommonCompileFlags% /O2 /MT
    )

rem *********************************************************************************************************
rem TinkerCore - static library
set SourceListCore=../Core/Math/VectorTypes.cpp

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsCore=/FdTinkerCore.pdb
    ) else (
    set DebugCompileFlagsCore=
    )
echo.
echo Building TinkerCore.lib...
cl /c %CommonCompileFlags% %DebugCompileFlagsCore% %SourceListCore% /Fo:TinkerCore.obj
lib /verbose /machine:x64 /Wx /out:TinkerCore.lib /nologo TinkerCore.obj

rem *********************************************************************************************************
rem TinkerPlatform - primary exe
set SourceListPlatform=../Platform/Win32Layer.cpp ../Platform/Win32PlatformGameAPI.cpp ../Platform/Win32Vulkan.cpp

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
set LibsToLink=user32.lib %VulkanPath%\Lib\vulkan-1.lib

cl %CommonCompileFlags% /I %CompileIncludePaths% %DebugCompileFlagsPlatform% %SourceListPlatform% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsPlatform% /out:TinkerPlatform.exe 

rem *********************************************************************************************************
rem TinkerGame - shared library
set SourceListGame=../Game/GameMain.cpp ../Platform/Win32PlatformGameAPI.cpp

if "%TIME:~0,1%" == " " (
    set BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__0%TIME:~1,1%_%TIME:~3,2%_%TIME:~6,2%
    ) else (
        set BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
        )
set GameDllPdbName=TinkerGame_%BuildTimestamp%.pdb
if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsGame=/Fd%GameDllPdbName%
    set DebugLinkFlagsGame=/pdb:%GameDllPdbName%
    ) else (
    set DebugCompileFlagsGame=
    set DebugLinkFlagsGame=
    )
echo.
echo Building TinkerGame.dll...
cl %CommonCompileFlags% %DebugCompileFlagsGame% %SourceListGame% /link %CommonLinkFlags% TinkerCore.lib /DLL /export:GameUpdate %DebugLinkFlagsGame% /out:TinkerGame.dll

:DoneBuild
popd
popd
