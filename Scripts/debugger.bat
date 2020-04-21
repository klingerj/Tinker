@echo off
pushd ..
pushd .\Build
devenv /debugexe ..\Build\TinkerPlatform.exe
popd
popd
