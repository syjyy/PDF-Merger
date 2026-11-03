# Run in an isolated Windows CI runner, not on a developer's normal installation.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not $env:RUNNER_TEMP) { throw 'This smoke test must run in GitHub Actions.' }
$destination = Join-Path $env:RUNNER_TEMP 'PDF Merger installer smoke'
$setup = Join-Path $root 'dist/PDF-Merger-Setup-windows-x64.exe'
$log = Join-Path $env:RUNNER_TEMP 'ordered-pdf-setup.log'
$process = Start-Process -FilePath $setup -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/NOICONS',
    "/DIR=`"$destination`"", "/LOG=`"$log`""
) -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -ne 0) { throw "Setup failed: $($process.ExitCode). See $log" }
foreach ($relative in @('OrderedPdfMerger.exe', 'tools/qpdf/bin/qpdf.exe', 'settings.ini', 'unins000.exe')) {
    if (-not (Test-Path (Join-Path $destination $relative))) { throw "Installed file missing: $relative" }
}
$settings = [IO.File]::ReadAllBytes((Join-Path $destination 'settings.ini'))
if ($settings.Length -lt 2 -or $settings[0] -ne 255 -or $settings[1] -ne 254) {
    throw 'Dependency settings must use UTF-16 LE to support Unicode paths.'
}
& "$destination/tools/qpdf/bin/qpdf.exe" --version
if ($LASTEXITCODE -ne 0) { throw 'Installed PDF dependency failed.' }
$process = Start-Process -FilePath "$destination/unins000.exe" -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
) -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -ne 0) { throw 'Uninstall failed.' }
if (Test-Path "$destination/OrderedPdfMerger.exe") { throw 'Uninstall did not remove application.' }
Write-Host 'PASS: silent install without Office, packaged dependency, Unicode settings and uninstall.'
