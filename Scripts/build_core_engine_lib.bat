@echo off
setlocal

set BuildConfig=%1
if "%BuildConfig%" NEQ "Debug" (
    if "%BuildConfig%" NEQ "Release" (
        echo Invalid build config specified.
        goto DoneBuild
        )
    )

echo ***** Building Tinker Core Engine *****

pushd ..
if NOT EXIST .\Build mkdir .\Build
pushd .\Build
del TinkerCore.pdb > NUL 2> NUL

rem *********************************************************************************************************
rem /FAs for .asm file output
set CommonCompileFlags=/nologo /std:c++17 /W4 /WX /wd4127 /wd4530 /wd4201 /wd4324 /wd4100 /wd4189 /EHsc /GR- /Gm- /GS- /fp:fast /Zi
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
rem TinkerCore - static library
set OBJDir=%cd%\obj_core_engine\
if NOT EXIST %OBJDir% mkdir %OBJDir%

set SourceListCore=../Core/Math/VectorTypes.cpp ../Core/FileIO/FileLoading.cpp ../Core/Containers/Vector.cpp ../Core/Utilities/MemTracker.cpp ../Core/Utilities/Mem.cpp ../Core/Raytracing/RayIntersection.cpp ../Core/Raytracing/AccelStructures/Octree.cpp
set OBJListCore= %OBJDir%VectorTypes.obj %OBJDir%FileLoading.obj %OBJDir%Vector.obj %OBJDir%MemTracker.obj %OBJDir%Mem.obj %OBJDir%Octree.obj %OBJDir%RayIntersection.obj 
set CompileDefines=

if "%BuildConfig%" == "Debug" (
    set DebugCompileFlagsCore=/FdTinkerCore.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    ) else (
    set DebugCompileFlagsCore=/FdTinkerCore.pdb
    set CompileDefines=%CompileDefines%/DASSERTS_ENABLE=1
    )

set CompileIncludePaths=/I ../Include 
set CommonCompileFlags=%CommonCompileFlags% /Fo:%OBJDir%

echo.
echo Building TinkerCore.lib...
cl /c %CommonCompileFlags% %CompileIncludePaths% %CompileDefines% %DebugCompileFlagsCore% %SourceListCore%
lib /machine:x64 /Wx /out:TinkerCore.lib /nologo %OBJListCore%

:DoneBuild
popd
popd
