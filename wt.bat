rem nircmd.exe win hide ititle "cmd.exe"
powershell -WindowStyle Hidden -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c start %LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe -d %CD%'"