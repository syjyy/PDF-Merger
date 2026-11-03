param(
    [switch]$SkipBuild,
    [string]$Payload = '',
    [string]$Compiler = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not $SkipBuild) { & "$PSScriptRoot/build.ps1" }
if (-not $Payload) { $Payload = Join-Path $root 'dist/OrderedPdfMerger' }
$Payload = [IO.Path]::GetFullPath($Payload)
if (-not (Test-Path (Join-Path $Payload 'OrderedPdfMerger.exe'))) { throw 'Application payload missing.' }
if (-not (Test-Path (Join-Path $Payload 'tools/qpdf/bin/qpdf.exe'))) { throw 'qpdf payload missing.' }
if (-not $Compiler) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $Compiler = $command.Source }
    else {
        foreach ($candidate in @(
            "${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe",
            "$env:LOCALAPPDATA/Programs/Inno Setup 6/ISCC.exe"
        )) {
            if (Test-Path $candidate) { $Compiler = $candidate; break }
        }
    }
}
if (-not $Compiler -or -not (Test-Path $Compiler)) {
    throw 'Install Inno Setup 6.7.3 or newer, or pass -Compiler C:\path\ISCC.exe.'
}
$dist = Join-Path $root 'dist'
New-Item -ItemType Directory -Force $dist | Out-Null
& $Compiler "/DPayloadDir=$Payload" "/DOutputDir=$dist" "$root/installer/setup.iss"
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
Write-Host "Ready: $dist/PDF-Merger-Setup-windows-x64.exe"
