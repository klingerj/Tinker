@echo off
pushd ..
if NOT EXIST .\Build (
    popd
    echo Build directory does not exist. Exiting.
    goto Done
    )
pushd .\Build

SpirvVM.exe

popd
popd
:Done
pause
