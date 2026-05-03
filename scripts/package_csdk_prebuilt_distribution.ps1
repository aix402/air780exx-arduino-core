param(
    [string]$DistributionDirectory = ".\dist\csdk-prebuilt-air780epm",
    [string]$OutputDirectory = ".\dist\releases",
    [string]$Version,
    [ValidateSet("Directory", "ToolRoot")]
    [string]$ArchiveLayout = "Directory",
    [string]$ToolArchiveRoot = "air780epm-csdk",
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

function Assert-Directory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
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

function Get-SafeName {
    param([Parameter(Mandatory = $true)][string]$Value)

    return ($Value -replace '[^A-Za-z0-9._-]+', '-').Trim('-')
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

$distributionRoot = Resolve-RepoPath $DistributionDirectory
$outputRoot = Resolve-RepoPath $OutputDirectory
$manifestPath = Join-Path $distributionRoot "arduino_export_manifest.json"

Assert-Directory -Path $distributionRoot -Description "CSDK prebuilt distribution directory"
Assert-File -Path $manifestPath -Description "CSDK prebuilt distribution manifest"

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
if (-not [bool]$manifest.distribution_package) {
    throw "Distribution manifest is missing distribution_package=true: $manifestPath"
}

$commit = Get-GitValue -Arguments @("rev-parse", "HEAD")
$shortCommit = Get-GitValue -Arguments @("rev-parse", "--short", "HEAD")
$branch = Get-GitValue -Arguments @("branch", "--show-current")
if ([string]::IsNullOrWhiteSpace($branch)) {
    $branch = "detached"
}
if ([string]::IsNullOrWhiteSpace($shortCommit)) {
    $shortCommit = "nogit"
}
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = "$(Get-SafeName $branch)-$shortCommit"
}

if (-not (Test-Path -LiteralPath $outputRoot -PathType Container)) {
    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
}

$archiveBaseName = "csdk-prebuilt-air780epm-$Version"
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
    throw "Release archive already exists. Pass -Clean to replace it: $zipPath"
}

$distributionSizeBytes = (Get-ChildItem -LiteralPath $distributionRoot -Recurse -File | Measure-Object -Property Length -Sum).Sum
$archiveParent = Split-Path -Parent $distributionRoot
$archiveLeaf = Split-Path -Leaf $distributionRoot

if ($ArchiveLayout -eq "Directory") {
    Push-Location $archiveParent
    try {
        Compress-Archive -LiteralPath $archiveLeaf -DestinationPath $zipPath -CompressionLevel Optimal
    }
    finally {
        Pop-Location
    }
}
else {
    if ([string]::IsNullOrWhiteSpace($ToolArchiveRoot)) {
        throw "ToolArchiveRoot must not be empty for ToolRoot archives."
    }
    $stagingRoot = Join-Path $repoRoot ".tmp_csdk_tool_package"
    $stagingToolRoot = Join-Path $stagingRoot $ToolArchiveRoot
    if (Test-Path -LiteralPath $stagingRoot) {
        Remove-Item -LiteralPath $stagingRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $stagingToolRoot | Out-Null
    try {
        Get-ChildItem -LiteralPath $distributionRoot -Force | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $stagingToolRoot -Recurse -Force
        }
        Compress-Archive -LiteralPath $stagingToolRoot -DestinationPath $zipPath -CompressionLevel Optimal
    }
    finally {
        if (Test-Path -LiteralPath $stagingRoot) {
            Remove-Item -LiteralPath $stagingRoot -Recurse -Force
        }
    }
}

$zipInfo = Get-Item -LiteralPath $zipPath
$hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText($shaPath, "$hash  $($zipInfo.Name)`n", [System.Text.UTF8Encoding]::new($false))

$releaseManifest = [ordered]@{
    package_name = $archiveBaseName
    version = $Version
    generated_at = (Get-Date).ToString("o")
    git_branch = $branch
    git_commit = $commit
    distribution_directory = Get-RepoRelativePath $distributionRoot
    archive_layout = $ArchiveLayout
    tool_archive_root = if ($ArchiveLayout -eq "ToolRoot") { $ToolArchiveRoot } else { $null }
    distribution_size_bytes = [Int64]$distributionSizeBytes
    archive = Get-RepoRelativePath $zipPath
    archive_size_bytes = [Int64]$zipInfo.Length
    sha256 = $hash
    distribution_manifest = Get-RepoRelativePath $manifestPath
    chip_target = $manifest.chip_target
    distribution_copy_policy = $manifest.distribution_copy_policy
    toolchain_bin = $manifest.toolchain.bin
}

[System.IO.File]::WriteAllText($releaseManifestPath, (($releaseManifest | ConvertTo-Json -Depth 6) + "`n"), [System.Text.UTF8Encoding]::new($false))

Write-Output "Release archive: $zipPath"
Write-Output "SHA256: $hash"
Write-Output "SHA256 file: $shaPath"
Write-Output "Release manifest: $releaseManifestPath"
