@echo off
echo %CD% > %TEMP%\wtcwd.tmp
schtasks /Run /TN "WindowsTerminalAdmin"
rem pwsh -WindowStyle Hidden -Command "schtasks /Run /TN \"WindowsTerminalAdmin\""
