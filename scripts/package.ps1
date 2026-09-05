param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\build\windows\Release\RegShare.exe'),
    [string]$Destination = (Join-Path $PSScriptRoot '..\dist')
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
$binary = (Resolve-Path $Executable).ProviderPath
$version = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($binary).ProductVersion
if ($version -notmatch '^\d+\.\d+\.\d+(-[a-zA-Z0-9.]+)?$') { throw 'Unexpected executable version.' }
$name = "RegionShare-$version-windows-x64"
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$staging = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
$folder = Join-Path $staging $name
try {
    New-Item -ItemType Directory -Path $folder | Out-Null
    Copy-Item $binary (Join-Path $folder 'RegShare.exe')
    foreach ($file in @('README.md', 'LICENSE', 'CHANGELOG.md', 'CONTRIBUTING.md', 'regionshare_requirements.md')) {
        Copy-Item (Join-Path $root $file) $folder
    }
    foreach ($directory in @('docs', 'assets')) {
        Copy-Item (Join-Path $root $directory) $folder -Recurse
    }
    $archive = Join-Path $Destination "$name.zip"
    Compress-Archive -Path $folder -DestinationPath $archive -Force
    $hash = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    [System.IO.File]::WriteAllText("$archive.sha256", "$hash  $name.zip`n")
    Write-Output "Package: $archive"
    Write-Output "SHA256: $hash"
} finally {
    if (Test-Path $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
}
