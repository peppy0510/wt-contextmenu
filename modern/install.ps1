# Registers the sparse packages so the two entries show at the top level of the Windows 11 context menu.
# Run as the normal user after build.bat; only the certificate trust step asks for elevation.
# A fresh self-signed certificate is created per install. Its private key is deleted right after
# signing, and only the public part stays in LocalMachine\TrustedPeople.
# -SkipRegister stops before Add-AppxPackage, for running this from an elevated ssh session,
# where registration fails with 0x80070005; register afterwards from a normal desktop session.
param([switch]$SkipRegister)
$ErrorActionPreference = 'Stop'

$root = $PSScriptRoot
$bin = Join-Path $root 'bin'
$obj = Join-Path $root 'obj'
$cer = Join-Path $obj 'WtContextMenu.cer'
$subject = 'CN=WtContextMenu'
$sdk = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
$packages = 'WtContextMenu', 'WtContextMenuAdmin'

if (-not (Test-Path (Join-Path $bin 'WtContextMenu.dll'))) { throw 'Run build.bat first.' }
New-Item -ItemType Directory -Force $obj | Out-Null

foreach ($name in $packages) {
    Get-AppxPackage -Name $name | Remove-AppxPackage
    & "$sdk\makeappx.exe" pack /o /nv /d (Join-Path $root "package\$name") /p (Join-Path $obj "$name.msix") | Out-Null
    if ($LASTEXITCODE) { throw "makeappx failed for $name ($LASTEXITCODE)" }
}

$cert = New-SelfSignedCertificate -Type Custom -Subject $subject -KeyUsage DigitalSignature `
    -FriendlyName 'wt-contextmenu package signing' -CertStoreLocation Cert:\CurrentUser\My `
    -NotAfter (Get-Date).AddYears(20) `
    -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}')
try {
    Export-Certificate -Cert $cert -FilePath $cer | Out-Null
    foreach ($name in $packages) {
        & "$sdk\signtool.exe" sign /q /fd SHA256 /sha1 $cert.Thumbprint /s My (Join-Path $obj "$name.msix")
        if ($LASTEXITCODE) { throw "signtool failed for $name ($LASTEXITCODE)" }
    }
}
finally {
    Remove-Item "Cert:\CurrentUser\My\$($cert.Thumbprint)" -DeleteKey
}

$trust = Join-Path $root 'trust.ps1'
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    & $trust -Cer $cer
}
else {
    $proc = Start-Process powershell.exe -Verb RunAs -Wait -PassThru `
        -ArgumentList '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$trust`"", '-Cer', "`"$cer`""
    if ($proc.ExitCode) { throw "trust.ps1 failed ($($proc.ExitCode))" }
}

if ($SkipRegister) { return }
foreach ($name in $packages) {
    Add-AppxPackage -Path (Join-Path $obj "$name.msix") -ExternalLocation $bin
}
$packages | ForEach-Object { Get-AppxPackage -Name $_ } | Select-Object Name, Version, Status
