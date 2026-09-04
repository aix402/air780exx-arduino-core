param(
    [string]$ManifestPath,
    [string]$OutputDirectory = ".\dist\releases",
    [string]$Version = "10.2.1-ec718",
    [string]$ToolArchiveRoot = "gnu-rm",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
. (Join-Path $PSScriptRoot "archive_helpers.ps1")

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

function Assert-File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
}

function Get-RepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $base = [System.IO.Path]::GetFullPath([string]$repoRoot)
    $target = [System.IO.Path]::GetFullPath($Path)
    if (-not $base.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $base += [System.IO.Path]::DirectorySeparatorChar
    }
    $relative = ([Uri]$base).MakeRelativeUri([Uri]$target).ToString()
    return [Uri]::UnescapeDataString($relative).Replace("/", [System.IO.Path]::DirectorySeparatorChar)
}

if ([string]::IsNullOrWhiteSpace($ManifestPath)) {
    $ManifestPath = Join-Path $repoRoot "runner\air780epm_runner\build\arduino_export_manifest.json"
}

$manifestFullPath = Resolve-RepoPath $ManifestPath
Assert-File -Path $manifestFullPath -Description "Arduino/CSDK export manifest"
$manifest = Get-Content -Raw -LiteralPath $manifestFullPath | ConvertFrom-Json

$toolchainBin = Resolve-RepoPath ([string]$manifest.toolchain.bin)
$toolchainRoot = [System.IO.Path]::GetFullPath((Join-Path $toolchainBin ".."))
Assert-File -Path (Join-Path $toolchainBin "arm-none-eabi-gcc.exe") -Description "GNU Arm GCC"

$outputRoot = Resolve-RepoPath $OutputDirectory
if (-not (Test-Path -LiteralPath $outputRoot -PathType Container)) {
    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
}

$archiveBaseName = "gnu-rm-$Version"
$zipPath = Join-Path $outputRoot "$archiveBaseName.zip"
$shaPath = Join-Path $outputRoot "$archiveBaseName.zip.sha256"
$releaseManifestPath = Join-Path $outputRoot "$archiveBaseName.manifest.json"

if ($Clean) {
    foreach ($path in @($zipPath, $shaPath, $releaseManifestPath)) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            Remove-Item -LiteralPath $path -Force
        }
    }
}

if (Test-Path -LiteralPath $zipPath -PathType Leaf) {
    throw "Toolchain archive already exists. Pass -Clean to replace it: $zipPath"
}

$toolchainPackageManifest = Join-Path $toolchainRoot "manifest.txt"
$tempManifestBackup = $null
if (Test-Path -LiteralPath $toolchainPackageManifest -PathType Leaf) {
    $tempManifestBackup = Join-Path ([System.IO.Path]::GetTempPath()) ("gnu-rm-manifest-{0}.txt" -f ([guid]::NewGuid()))
    Move-Item -LiteralPath $toolchainPackageManifest -Destination $tempManifestBackup -Force
}

try {
    if ([string]::IsNullOrWhiteSpace($ToolArchiveRoot)) {
        throw "ToolArchiveRoot must not be empty."
    }
    $stagingRoot = Join-Path $repoRoot ".tmp_gnu_rm_tool_package"
    $stagingToolRoot = Join-Path $stagingRoot $ToolArchiveRoot
    if (Test-Path -LiteralPath $stagingRoot) {
        Remove-Item -LiteralPath $stagingRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $stagingToolRoot | Out-Null
    try {
        Get-ChildItem -LiteralPath $toolchainRoot -Force | Where-Object {
            $_.FullName -ne $toolchainPackageManifest
        } | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $stagingToolRoot -Recurse -Force
        }
        New-ProjectZipArchive -SourceDirectory $stagingToolRoot -DestinationPath $zipPath
    }
    finally {
        if (Test-Path -LiteralPath $stagingRoot) {
            Remove-Item -LiteralPath $stagingRoot -Recurse -Force
        }
    }
}
finally {
    if ($tempManifestBackup -and (Test-Path -LiteralPath $tempManifestBackup -PathType Leaf)) {
        Move-Item -LiteralPath $tempManifestBackup -Destination $toolchainPackageManifest -Force
    }
}

$zipInfo = Get-Item -LiteralPath $zipPath
$hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText($shaPath, "$hash  $($zipInfo.Name)`n", [System.Text.UTF8Encoding]::new($false))

$toolchainSizeBytes = (Get-ChildItem -LiteralPath $toolchainRoot -Recurse -File | Where-Object {
    $_.FullName -ne $toolchainPackageManifest
} | Measure-Object -Property Length -Sum).Sum

$releaseManifest = [ordered]@{
    package_name = $archiveBaseName
    version = $Version
    generated_at = (Get-Date).ToString("o")
    tool_name = "gnu-rm"
    archive_layout = "ArduinoTool"
    tool_archive_root = $ToolArchiveRoot
    archive = Get-RepoRelativePath $zipPath
    archive_size_bytes = [Int64]$zipInfo.Length
    sha256 = $hash
    toolchain_size_bytes = [Int64]$toolchainSizeBytes
}

[System.IO.File]::WriteAllText($releaseManifestPath, (($releaseManifest | ConvertTo-Json -Depth 5) + "`n"), [System.Text.UTF8Encoding]::new($false))

Write-Output "Toolchain archive: $zipPath"
Write-Output "SHA256: $hash"
Write-Output "SHA256 file: $shaPath"
Write-Output "Release manifest: $releaseManifestPath"
