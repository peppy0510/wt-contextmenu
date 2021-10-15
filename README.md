## Windows Terminal Context Menu

Registers Microsoft Windows Terminal in the context menu of the right mouse button.

* Windows Terminal
* Windows Terminal (Admin)

## How to Install

1. Install NirCmd.

   ```bash
   choco install nircmd
   ```

2. Run setup.bat with administrator privilege.

    ```bash
    setup.bat
    ```

3. Register Task Scheduler.

    ```bash
    taskschd.msc
    ```

    Create New Task

* General Tab

    Name: `WindowsTerminalAdmin`

    Check Run only when user is logged on

    Check Run with highest privileges

    Check Hidden

* Action Tab
  
    Create New Action
    
    Program/script: `C:\ProgramData\chocolatey\lib\nircmd\tools\nircmd.exe`
    
    Add arguments: `exec hide "%LOCALAPPDATA%\Microsoft\WindowsApps\wtadmintask.bat"`

## Reference

[https://github.com/microsoft/terminal](https://github.com/microsoft/terminal)

