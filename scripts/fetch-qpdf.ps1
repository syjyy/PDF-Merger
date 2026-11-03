param([string]$Destination = "$PSScriptRoot/../tools/qpdf")
$ErrorActionPreference = 'Stop'
$version = '12.2.0'
$expectedHash = '7CF812816E7D688E6D782FB7C060EE18613E6366568F6DE4819BFA7DB7A05CBC'
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('ordered-qpdf-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $temporary | Out-Null
try {
    $archive = Join-Path $temporary 'qpdf.zip'
    Invoke-WebRequest "https://github.com/qpdf/qpdf/releases/download/v$version/qpdf-$version-msvc64.zip" -OutFile $archive
    if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expectedHash) {
        throw 'qpdf archive checksum mismatch. Download was not installed.'
    }
    Expand-Archive -LiteralPath $archive -DestinationPath $temporary
    $source = Join-Path $temporary "qpdf-$version-msvc64"
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    # Keep the entire upstream archive, including DLLs and license notices.
    Copy-Item -Path (Join-Path $source '*') -Destination $Destination -Recurse -Force
    if (-not (Test-Path (Join-Path $Destination 'bin/qpdf.exe'))) { throw 'Unexpected qpdf archive layout.' }
    Write-Host "qpdf $version installed: $Destination"
} finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force
}
