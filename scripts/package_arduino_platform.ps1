param(
    [string]$PlatformDirectory = ".\core\air780epm",
    [string]$ExamplesDirectory = ".\examples",
    [string[]]$PackageExampleDirectories = @(
        "01.Basics",
        "02.Serial",
        "03.Bus",
        "04.PWM",
        "05.Display",
        "06.Analog",
        "07.Servo",
        "08.Network",
        "09.NVM",
        "10.FileSystem",
        "11.OTA",
        "12.Sleep"
    ),
    [string]$LibrariesDirectory = ".\libraries",
    [string]$ExampleLibraryName = "AIR780",
    [string[]]$PackageLibraries = @(),
    [string]$OutputDirectory = ".\dist\releases",
    [string]$Version = "0.2.0",
    [string]$PlatformArchiveRoot = "air780",
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

    $base = [System.IO.Path]::GetFullPath([string]$repoRoot)
    $target = [System.IO.Path]::GetFullPath($Path)
    if (-not $base.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $base += [System.IO.Path]::DirectorySeparatorChar
    }
    $relative = ([Uri]$base).MakeRelativeUri([Uri]$target).ToString()
    return [Uri]::UnescapeDataString($relative).Replace("/", [System.IO.Path]::DirectorySeparatorChar)
}

function Copy-PackagedExamples {
    param([Parameter(Mandatory = $true)][string]$DestinationExamplesRoot)

    New-Item -ItemType Directory -Force -Path $DestinationExamplesRoot | Out-Null
    foreach ($exampleDirectory in @($PackageExampleDirectories)) {
        if ([string]::IsNullOrWhiteSpace($exampleDirectory)) {
            continue
        }
        $exampleRoot = Join-Path $examplesRoot $exampleDirectory
        if (Test-Path -LiteralPath $exampleRoot -PathType Container) {
            Copy-Item -LiteralPath $exampleRoot -Destination (Join-Path $DestinationExamplesRoot $exampleDirectory) -Recurse -Force
        }
    }
}

function New-ExampleLibrary {
    param([Parameter(Mandatory = $true)][string]$DestinationLibrariesRoot)

    if ([string]::IsNullOrWhiteSpace($ExampleLibraryName)) {
        return
    }

    $libraryRoot = Join-Path $DestinationLibrariesRoot $ExampleLibraryName
    $sourceRoot = Join-Path $libraryRoot "src"
    New-Item -ItemType Directory -Force -Path $sourceRoot | Out-Null

    $libraryProperties = @"
name=$ExampleLibraryName
version=$Version
author=AIR780 Arduino Core Contributors
maintainer=AIR780 Arduino Core Contributors
sentence=Examples for AIR780 Arduino boards.
paragraph=Board examples for the AIR780 Arduino Core package.
category=Other
url=https://github.com/aix402/air780exx-arduino-core
architectures=air780
"@
    [System.IO.File]::WriteAllText(
        (Join-Path $libraryRoot "library.properties"),
        ($libraryProperties + "`n"),
        [System.Text.UTF8Encoding]::new($false)
    )
    [System.IO.File]::WriteAllText(
        (Join-Path $sourceRoot "$ExampleLibraryName.h"),
        "#pragma once`n",
        [System.Text.UTF8Encoding]::new($false)
    )
    Copy-PackagedExamples -DestinationExamplesRoot (Join-Path $libraryRoot "examples")
}

$platformRoot = Resolve-RepoPath $PlatformDirectory
$examplesRoot = Resolve-RepoPath $ExamplesDirectory
$librariesRoot = Resolve-RepoPath $LibrariesDirectory
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

$archiveBaseName = "air780-arduino-platform-$Version"
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
    $stagingLibrariesRoot = Join-Path $stagingPlatformRoot "libraries"
    New-Item -ItemType Directory -Force -Path $stagingLibrariesRoot | Out-Null
    New-ExampleLibrary -DestinationLibrariesRoot $stagingLibrariesRoot

    if (Test-Path -LiteralPath $librariesRoot -PathType Container) {
        foreach ($libraryName in @($PackageLibraries)) {
            if ([string]::IsNullOrWhiteSpace($libraryName)) {
                continue
            }
            $libraryRoot = Join-Path $librariesRoot $libraryName
            if (Test-Path -LiteralPath $libraryRoot -PathType Container) {
                Copy-Item -LiteralPath $libraryRoot -Destination (Join-Path $stagingLibrariesRoot $libraryName) -Recurse -Force
            }
        }
    }
    foreach ($scriptName in $platformToolScripts) {
        Copy-Item -LiteralPath (Join-Path $repoRoot "scripts\$scriptName") -Destination (Join-Path $stagingPlatformRoot "tools\$scriptName") -Force
    }

    New-ProjectZipArchive -SourceDirectory $stagingPlatformRoot -DestinationPath $zipPath
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
    examples_directory = if (Test-Path -LiteralPath $examplesRoot -PathType Container) { Get-RepoRelativePath $examplesRoot } else { $null }
    packaged_example_directories = @($PackageExampleDirectories)
    packaged_example_library = $ExampleLibraryName
    libraries_directory = if (Test-Path -LiteralPath $librariesRoot -PathType Container) { Get-RepoRelativePath $librariesRoot } else { $null }
    packaged_libraries = @($PackageLibraries)
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
