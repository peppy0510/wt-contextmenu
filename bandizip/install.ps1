# Registers the two sparse packages that put Bandizip's compress and smart-extract entries at the top
# level of the Windows 11 context menu. Same flow as ..\modern\install.ps1, with its own certificate.
# Run as the normal user after build.bat; only the certificate trust step asks for elevation.
# -SkipRegister stops before Add-AppxPackage, for an elevated ssh session where registration fails
# with 0x80070005; -RegisterOnly then registers the signed packages from a normal desktop session.
param([switch]$SkipRegister, [switch]$RegisterOnly)
$ErrorActionPreference = 'Stop'

$root = $PSScriptRoot
$bin = Join-Path $root 'bin'
$obj = Join-Path $root 'obj'
$cer = Join-Path $obj 'BdzContextMenu.cer'
$subject = 'CN=BdzContextMenu'
$packages = 'BdzCompress', 'BdzExtract'

function Register-Packages {
    foreach ($name in $packages) {
        Add-AppxPackage -Path (Join-Path $obj "$name.msix") -ExternalLocation $bin
    }
    $packages | ForEach-Object { Get-AppxPackage -Name $_ } | Select-Object Name, Version, Status
}

if ($RegisterOnly) {
    Register-Packages
    return
}

if (-not (Test-Path (Join-Path $bin 'BdzContextMenu.dll'))) { throw 'Run build.bat first.' }
$sdk = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.*\x64\makeappx.exe" |
    Sort-Object { [version]$_.Directory.Parent.Name } | Select-Object -Last 1 | ForEach-Object DirectoryName
if (-not $sdk) { throw 'Windows SDK makeappx.exe not found.' }
$bandizip = (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Bandizip.exe').'(default)'
$icon = Join-Path (Split-Path $bandizip) 'shellicons\bandizip.ico'

# The package logo is drawn from the installed Bandizip icon, so the repository carries no Bandizip asset.
Add-Type -AssemblyName System.Drawing
foreach ($name in $packages) {
    Get-AppxPackage -Name $name | Remove-AppxPackage
    $stage = Join-Path $obj $name
    New-Item -ItemType Directory -Force $stage | Out-Null
    Copy-Item (Join-Path $root "package\$name\AppxManifest.xml") $stage -Force
    $ico = New-Object Drawing.Icon($icon, 256, 256)
    $bitmap = $ico.ToBitmap()
    $bitmap.Save((Join-Path $stage 'logo.png'), [Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    $ico.Dispose()
    & "$sdk\makeappx.exe" pack /o /nv /d $stage /p (Join-Path $obj "$name.msix") | Out-Null
    if ($LASTEXITCODE) { throw "makeappx failed for $name ($LASTEXITCODE)" }
}

$cert = New-SelfSignedCertificate -Type Custom -Subject $subject -KeyUsage DigitalSignature `
    -FriendlyName 'bandizip context menu package signing' -CertStoreLocation Cert:\CurrentUser\My `
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
Register-Packages
