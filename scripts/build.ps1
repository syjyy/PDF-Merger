param([switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    cmake -S . -B build -A x64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
    cmake --build build --config Release
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    & "$PSScriptRoot/fetch-qpdf.ps1" -Destination "$root/tools/qpdf"
    $env:QPDF_PATH = "$root/tools/qpdf/bin/qpdf.exe"
    if (-not $SkipTests) {
        ctest --test-dir build -C Release --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
    }
    New-Item -ItemType Directory -Force dist/OrderedPdfMerger/tools/qpdf,dist/OrderedPdfMerger/static | Out-Null
    Copy-Item build/Release/OrderedPdfMerger.exe dist/OrderedPdfMerger/
    Copy-Item tools/qpdf/* dist/OrderedPdfMerger/tools/qpdf/ -Recurse -Force
    Copy-Item README.md,LICENSE,THIRD_PARTY_NOTICES.md dist/OrderedPdfMerger/
    Copy-Item static/logo.png dist/OrderedPdfMerger/static/
    Compress-Archive -Path dist/OrderedPdfMerger -DestinationPath dist/PDF-Merger-windows-x64.zip -Force
    Write-Host 'Ready: dist/PDF-Merger-windows-x64.zip'
} finally { Pop-Location }
