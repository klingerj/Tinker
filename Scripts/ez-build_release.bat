@echo off

rem This is meant to be an easy way to build and run the app/benchmarks/tests.
rem This script can just be double clicked in Windows Explorer.
rem All other scripts should be run from the command line, after running shell.bat.

rem Run vcvars since this isn't executing from the shell - this won't hurt, though.
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
echo.

rem Assume release mode
set BuildConfig=Release

rem Run other build scripts
call build_app.bat %BuildConfig%
call build_server.bat 
call build_benchmarks.bat %BuildConfig%
call build_tests.bat %BuildConfig%

echo.
echo Build finished.
echo Try running game.bat, benchmarks.bat, or test.bat!
pause
