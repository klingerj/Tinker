@echo off
setlocal
setlocal enabledelayedexpansion

if "%1" == "-h" (goto PrintHelp)
if "%1" == "-help" (goto PrintHelp)
if "%1" == "help" (goto PrintHelp)
goto StartScript

:PrintHelp
echo Usage: build_sc.bat ^<build_mode^> ^<graphics_api^>
echo.
echo build_mode:
echo   Release
echo   Debug
echo.
echo graphics_api
echo   VK
echo   D3D12
echo.
echo For example:
echo build_sc.bat Release VK
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

echo ***** Building Tinker Shader Compiler *****

pushd ..
pushd .\ToolsBin
del TinkerSC.pdb > NUL 2> NUL

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
rem TinkerSC - primary exe
set AbsolutePathPrefix=%cd%

set SourceListSC= 
set SourceListSC=%SourceListSC% %AbsolutePathPrefix%/../Tools/ShaderCompiler/ShaderCompiler.cpp 
set SourceListSC=%SourceListSC% %AbsolutePathPrefix%/../Core/DataStructures/Vector.cpp 
set SourceListSC=%SourceListSC% %AbsolutePathPrefix%/../Core/Mem.cpp 
set SourceListSC=%SourceListSC% %AbsolutePathPrefix%/../Core/Platform/Win32File.cpp 
set SourceListSC=%SourceListSC% %AbsolutePathPrefix%/../Core/Platform/Win32Logging.cpp 

rem Calculate absolute path prefix for application path parameters here
set AbsolutePathPrefix=%AbsolutePathPrefix:\=\\%
set CompileDefines=/D_SHADERS_SPV_DIR=%AbsolutePathPrefix%\\..\\Shaders\\hlsl\\ /DASSERTS_ENABLE=1 

if %GraphicsAPIChosen% == "VK" (
    set CompileDefines=!CompileDefines! /DVULKAN 
)

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsSC=/FdTinkerSC.pdb
    set DebugLinkFlagsSC=/pdb:TinkerSC.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=!CompileDefines!
    ) else (
    set DebugCompileFlagsSC=/FdTinkerSC.pdb
    set DebugLinkFlagsSC=/pdb:TinkerSC.pdb /NATVIS:../Utils/Natvis/Tinker.natvis
    set CompileDefines=!CompileDefines!
    )

set CompileIncludePaths= /I ../Core /I ../ThirdParty/dxc_2022_07_18
set LibsToLink=user32.lib ws2_32.lib ../ThirdParty/dxc_2022_07_18/lib/x64/dxcompiler.lib

echo.
echo Building TinkerSC.exe...

set OBJDir=%cd%\obj_sc\
if NOT EXIST %OBJDir% mkdir %OBJDir%
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

cl %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsSC% %SourceListSC% /link %LibsToLink% %CommonLinkFlags% %DebugLinkFlagsSC% /out:TinkerSC.exe

echo.
if EXIST TinkerSC.exp (
    echo Deleting unnecessary file TinkerSC.exp
    echo.
    del TinkerSC.exp
    )

:DoneBuild
echo.
popd
popd

:EndScript
