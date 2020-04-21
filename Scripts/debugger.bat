@echo off
pushd ..
pushd .\Build
devenv /debugexe ..\Build\TinkerPlatform.exe /nosplash
popd
popd
