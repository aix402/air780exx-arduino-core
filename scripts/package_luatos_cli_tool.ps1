param(
    [string]$SourcePath = ".\tools\luatos-cli-release\luatos-cli.exe",
    [string]$OutputDirectory = ".\dist\releases",
    [string]$Version,
    [string]$ToolArchiveRoot = "luatos-cli",
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

$sourceFullPath = Resolve-RepoPath $SourcePath
$outputRoot = Resolve-RepoPath $OutputDirectory
Assert-File -Path $sourceFullPath -Description "luatos-cli executable"

if ([string]::IsNullOrWhiteSpace($Version)) {
    $versionText = (& $sourceFullPath --version 2>$null | Out-String).Trim()
    if ($versionText -match '([0-9]+(?:\.[0-9]+)+)') {
        $Version = $Matches[1]
    }
    else {
        throw "Could not determine luatos-cli version from: $versionText"
    }
}

if ([string]::IsNullOrWhiteSpace($ToolArchiveRoot)) {
    throw "ToolArchiveRoot must not be empty."
}

if (-not (Test-Path -LiteralPath $outputRoot -PathType Container)) {
    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
}

$archiveBaseName = "luatos-cli-$Version"
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
    throw "luatos-cli archive already exists. Pass -Clean to replace it: $zipPath"
}

$stagingRoot = Join-Path $repoRoot ".tmp_luatos_cli_tool_package"
$stagingToolRoot = Join-Path $stagingRoot $ToolArchiveRoot
if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stagingToolRoot | Out-Null

try {
    Copy-Item -LiteralPath $sourceFullPath -Destination (Join-Path $stagingToolRoot "luatos-cli.exe") -Force
    New-ProjectZipArchive -SourceDirectory $stagingToolRoot -DestinationPath $zipPath
}
finally {
    if (Test-Path -LiteralPath $stagingRoot) {
        Remove-Item -LiteralPath $stagingRoot -Recurse -Force
    }
}

$zipInfo = Get-Item -LiteralPath $zipPath
$hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText($shaPath, "$hash  $($zipInfo.Name)`n", [System.Text.UTF8Encoding]::new($false))

$releaseManifest = [ordered]@{
    package_name = $archiveBaseName
    version = $Version
    generated_at = (Get-Date).ToString("o")
    tool_name = "luatos-cli"
    archive_layout = "ArduinoTool"
    tool_archive_root = $ToolArchiveRoot
    source = Get-RepoRelativePath $sourceFullPath
    archive = Get-RepoRelativePath $zipPath
    archive_size_bytes = [Int64]$zipInfo.Length
    sha256 = $hash
}

[System.IO.File]::WriteAllText($releaseManifestPath, (($releaseManifest | ConvertTo-Json -Depth 5) + "`n"), [System.Text.UTF8Encoding]::new($false))

Write-Output "luatos-cli archive: $zipPath"
Write-Output "SHA256: $hash"
Write-Output "SHA256 file: $shaPath"
Write-Output "Release manifest: $releaseManifestPath"
