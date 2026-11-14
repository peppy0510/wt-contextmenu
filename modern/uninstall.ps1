# Removes the sparse packages and their trusted signing certificate.
#Requires -RunAsAdministrator
$ErrorActionPreference = 'Stop'

'WtContextMenu', 'WtContextMenuAdmin' | ForEach-Object { Get-AppxPackage -Name $_ | Remove-AppxPackage }
Get-ChildItem Cert:\LocalMachine\TrustedPeople | Where-Object Subject -eq 'CN=WtContextMenu' | Remove-Item
