@echo off
setlocal
setlocal enabledelayedexpansion

::echo cl %~1 
::start /wait /b timeout /t 3 /nobreak > NUL
cl /c %~1 
::echo Done.
