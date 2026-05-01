param(
    [string]$PlatformDirectory = ".\core\air780epm",
    [string]$OutputDirectory = ".\dist\releases",
    [string]$Version = "0.1.0",
    [string]$PlatformArchiveRoot = "ec718pm",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

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

function Get-GitValue {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    try {
        $value = & git @Arguments 2>$null
        if ($LASTEXITCODE -ne 0) {
            return $null
        }
        return (($value | Out-String).Trim())
    }
    catch {
        return $null
    }
}

function Get-RepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [System.IO.Path]::GetRelativePath($repoRoot, ([System.IO.Path]::GetFullPath($Path)))
}

$platformRoot = Resolve-RepoPath $PlatformDirectory
$outputRoot = Resolve-RepoPath $OutputDirectory
Assert-File -Path (Join-Path $platformRoot "platform.txt") -Description "Arduino platform.txt"
Assert-File -Path (Join-Path $platformRoot "boards.txt") -Description "Arduino boards.txt"
Assert-File -Path (Join-Path $repoRoot "scripts\arduino_cli_recipe.ps1") -Description "Arduino recipe script"
Assert-File -Path (Join-Path $repoRoot "scripts\upload_core.ps1") -Description "Arduino upload script"
$platformToolScripts = @(
    "arduino_cli_recipe.ps1",
    "upload_core.ps1",
    "link_arduino_with_csdk.ps1",
    "export_arduino_direct_link.ps1",
    "export_arduino_build_manifest.ps1",
    "csdk_prebuild_stamp.ps1"
)
foreach ($scriptName in $platformToolScripts) {
    Assert-File -Path (Join-Path $repoRoot "scripts\$scriptName") -Description "Arduino platform tool script $scriptName"
}

if (-not (Test-Path -LiteralPath $outputRoot -PathType Container)) {
    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
}

$archiveBaseName = "air780exx-arduino-platform-ec718pm-$Version"
$zipPath = Join-Path $outputRoot "$archiveBaseName.zip"
$shaPath = Join-Path $outputRoot "$archiveBaseName.zip.sha256"
$manifestPath = Join-Path $outputRoot "$archiveBaseName.manifest.json"

if ($Clean) {
    foreach ($path in @($zipPath, $shaPath, $manifestPath)) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            Remove-Item -LiteralPath $path -Force
        }
    }
}

if (Test-Path -LiteralPath $zipPath -PathType Leaf) {
    throw "Platform archive already exists. Pass -Clean to replace it: $zipPath"
}

$stagingRoot = Join-Path $repoRoot ".tmp_platform_package"
$stagingPlatformRoot = Join-Path $stagingRoot $PlatformArchiveRoot
if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stagingPlatformRoot | Out-Null

try {
    Get-ChildItem -LiteralPath $platformRoot -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $stagingPlatformRoot -Recurse -Force
    }
    foreach ($scriptName in $platformToolScripts) {
        Copy-Item -LiteralPath (Join-Path $repoRoot "scripts\$scriptName") -Destination (Join-Path $stagingPlatformRoot "tools\$scriptName") -Force
    }

    Compress-Archive -LiteralPath $stagingPlatformRoot -DestinationPath $zipPath -CompressionLevel Optimal
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
    git_branch = Get-GitValue -Arguments @("branch", "--show-current")
    git_commit = Get-GitValue -Arguments @("rev-parse", "HEAD")
    platform_directory = Get-RepoRelativePath $platformRoot
    platform_archive_root = $PlatformArchiveRoot
    archive = Get-RepoRelativePath $zipPath
    archive_size_bytes = [Int64]$zipInfo.Length
    sha256 = $hash
}

[System.IO.File]::WriteAllText($manifestPath, (($releaseManifest | ConvertTo-Json -Depth 6) + "`n"), [System.Text.UTF8Encoding]::new($false))

Write-Output "Platform archive: $zipPath"
Write-Output "SHA256: $hash"
Write-Output "SHA256 file: $shaPath"
Write-Output "Release manifest: $manifestPath"
