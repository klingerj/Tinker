@echo off
pushd ..
pushd .\Build
devenv /debugexe ..\Build\%1 /nosplash
popd
popd
