@echo off
setlocal

SET VcVarsBat="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
SET VsDevCmdBat="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

%comspec% /k "%VcVarsBat% && %VsDevCmdBat%"
pause
