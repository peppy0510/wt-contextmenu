@echo off
set /p WORKING_DIRECTORY=< %TEMP%\wtcwd.tmp
%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe -d %WORKING_DIRECTORY%
