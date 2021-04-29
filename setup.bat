@echo on
xcopy "%~dp0wt.ico" "%LOCALAPPDATA%\Microsoft\WindowsApps\" /b /v /y /q
xcopy "%~dp0wt.bat" "%LOCALAPPDATA%\Microsoft\WindowsApps\" /b /v /y /q
@echo off

set iconPath="\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.ico\""
set openAdminPath="\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.bat\""
set openPath="\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\""

reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminal" /f /v "Icon" /d "%iconPath%"
reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminal" /f /v "MUIVerb" /d "Windows Terminal"
reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminal\command" /f /ve /d "%openPath% -d ""%%V"""

reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminalAdmin" /f /v "HasLUAShield"
reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminalAdmin" /f /v "Icon" /d "%iconPath%"
reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminalAdmin" /f /v "MUIVerb" /d "Windows Terminal (Admin)"
reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminalAdmin\command" /f /ve /d "%openAdminPath% -d ""%%V"""
rem reg.exe add "HKEY_CLASSES_ROOT\Directory\Background\shell\WindowsTerminalAdmin\command" /f /ve /d "powershell -WindowStyle Hidden -Command \"Start-Process cmd -Verb RunAs -ArgumentList '/c start wt -d %CD%'\""
