@echo off
setlocal
cd /d "%~dp0"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
if not exist bin mkdir bin
if not exist obj mkdir obj

cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /DUNICODE /D_UNICODE /LD /Foobj\ src\WtContextMenu.cpp ^
   /link /DEF:src\WtContextMenu.def /OUT:bin\WtContextMenu.dll /IMPLIB:obj\WtContextMenu.lib || exit /b 1
cl /nologo /O2 /MT /DUNICODE /D_UNICODE /Foobj\ src\Host.cpp ^
   /link /SUBSYSTEM:WINDOWS /OUT:bin\WtContextMenuHost.exe || exit /b 1

copy /y ..\wt.ico bin\wt.ico >nul
echo build ok
