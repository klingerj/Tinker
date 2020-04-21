@echo off

IF NOT EXIST ./Build mkdir ./Build
pushd Build
del *.pdb > NUL 2> NUL

SET CommonCompileFlags=/nologo /Zi /O2 /std:c++17 /MT /W4 /WX /wd4201 /wd4324 /wd4100 /wd4189 /EHsc /GR- /Gm-
SET CommonLinkFlags=/incremental:no /opt:ref

REM TinkerCore - static library (.lib)
SET SourceListCore=..\Core\Math\VectorTypes.cpp
cl /c %CommonCompileFlags% /FdTinkerCore.pdb %SourceListCore%
lib user32.lib /machine:x64 /Wx /out:TinkerCore.lib /nologo

REM TinkerPlatform - primary exe
SET SourceListPlatform=../Platform/Win32Layer.cpp ../Platform/Win32PlatformGameAPI.cpp
cl %CommonCompileFlags% /FdTinkerPlatform.pdb %SourceListPlatform% /link %CommonLinkFlags% /out:TinkerPlatform.exe

REM TinkerGame - shared library (.dll)
SET SourceListGame=../Game/GameMain.cpp ../Platform/Win32PlatformGameAPI.cpp

IF "%TIME:~0,1%" == " " (
    SET BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__0%TIME:~1,1%_%TIME:~3,2%_%TIME:~6,2%
    ) ELSE (
        SET BuildTimestamp=%DATE:~4,2%_%DATE:~7,2%_%DATE:~10,4%__%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
        )
SET GameDllPdbName=TinkerGame_%BuildTimestamp%.pdb
cl %CommonCompileFlags% /Fd%GameDllPdbName% %SourceListGame% /link %CommonLinkFlags% /DLL /export:GameUpdate /PDB:%GameDllPdbName% /out:TinkerGame.dll

popd
