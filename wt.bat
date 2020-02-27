rem nircmd.exe win hide ititle "cmd.exe"
powershell -WindowStyle Hidden -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c start wt -d %CD%'"