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

## Windows 11 Context Menu

Windows 11 shows the entries above only under "Show more options".
`modern` adds them to the top level of the new context menu as sparse packages.

1. Run setup.bat and register Task Scheduler as above. The admin entry runs `WindowsTerminalAdmin`.

2. Build with Visual Studio 2022 and Windows SDK 10.0.26100.

    ```bash
    modern\build.bat
    ```

3. Run install.ps1 without administrator privilege. It asks for elevation only to trust the signing certificate.

    ```bash
    powershell -ExecutionPolicy Bypass -File modern\install.ps1
    ```

To remove, run `modern\uninstall.ps1` with administrator privilege.

## Reference

[https://github.com/microsoft/terminal](https://github.com/microsoft/terminal)

