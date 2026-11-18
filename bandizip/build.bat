@echo off
setlocal
cd /d "%~dp0"
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "usebackq delims=" %%i in (`vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo Visual C++ x64 build tools not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist bin mkdir bin
if not exist obj mkdir obj

cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /DUNICODE /D_UNICODE /LD /Foobj\ src\BdzContextMenu.cpp ^
   /link /DEF:src\BdzContextMenu.def /OUT:bin\BdzContextMenu.dll /IMPLIB:obj\BdzContextMenu.lib || exit /b 1
cl /nologo /O2 /MT /DUNICODE /D_UNICODE /Foobj\ ..\modern\src\Host.cpp ^
   /link /SUBSYSTEM:WINDOWS /OUT:bin\BdzContextMenuHost.exe || exit /b 1
echo build ok
