# Replaces the trusted package-signing certificate (public part only). Needs elevation.
#Requires -RunAsAdministrator
param([Parameter(Mandatory)][string]$Cer)
$ErrorActionPreference = 'Stop'

Get-ChildItem Cert:\LocalMachine\TrustedPeople | Where-Object Subject -eq 'CN=BdzContextMenu' | Remove-Item
Import-Certificate -FilePath $Cer -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
