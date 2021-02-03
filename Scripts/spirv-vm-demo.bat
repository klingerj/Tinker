@echo off
pushd ..
if NOT EXIST .\Build (
    popd
    echo Build directory does not exist. Exiting.
    goto Done
    )
pushd .\Build

spirv-vm.exe

popd
popd
:Done
pause
