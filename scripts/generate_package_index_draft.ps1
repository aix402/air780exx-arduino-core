param(
    [string]$OutputPath = ".\dist\releases\package_openluat_ec718pm_index.draft.json",
    [string]$BaseUrl = "https://example.com/openluat/arduino/releases",
    [string]$PlatformArchive,
    [string]$CsdkArchive,
    [string]$GnuRmArchive,
    [string]$PlatformVersion = "0.1.0",
    [string]$CsdkVersion = "0.1.0",
    [string]$GnuRmVersion = "10.2.1-ec718"
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

function Get-ArchiveEntry {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$BaseUrl
    )

    $fullPath = Resolve-RepoPath $Path
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Archive was not found: $fullPath"
    }

    $item = Get-Item -LiteralPath $fullPath
    $hash = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $base = $BaseUrl.TrimEnd("/")
    return [ordered]@{
        url = "$base/$($item.Name)"
        archiveFileName = $item.Name
        checksum = "SHA-256:$hash"
        size = [string]$item.Length
    }
}

function Find-LatestArchive {
    param([Parameter(Mandatory = $true)][string]$Pattern)

    $releaseDir = Resolve-RepoPath ".\dist\releases"
    $match = Get-ChildItem -LiteralPath $releaseDir -Filter $Pattern -File |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
    if ($null -eq $match) {
        throw "Archive matching '$Pattern' was not found in $releaseDir"
    }
    return $match.FullName
}

function New-ToolSystem {
    param(
        [Parameter(Mandatory = $true)][string]$HostName,
        [Parameter(Mandatory = $true)][hashtable]$Archive
    )

    return [ordered]@{
        host = $HostName
        url = $Archive.url
        archiveFileName = $Archive.archiveFileName
        checksum = $Archive.checksum
        size = $Archive.size
    }
}

if ([string]::IsNullOrWhiteSpace($PlatformArchive)) {
    $PlatformArchive = Find-LatestArchive -Pattern "air780exx-arduino-platform-ec718pm-*.zip"
}
if ([string]::IsNullOrWhiteSpace($CsdkArchive)) {
    $CsdkArchive = Find-LatestArchive -Pattern "csdk-prebuilt-air780epm-*-notoolchain-toolroot.zip"
}
if ([string]::IsNullOrWhiteSpace($GnuRmArchive)) {
    $GnuRmArchive = Find-LatestArchive -Pattern "gnu-rm-*.zip"
}

$csdk = Get-ArchiveEntry -Path $CsdkArchive -BaseUrl $BaseUrl
$gnuRm = Get-ArchiveEntry -Path $GnuRmArchive -BaseUrl $BaseUrl

$platform = [ordered]@{
    name = "AIR780EXX Arduino Core"
    architecture = "ec718pm"
    version = $PlatformVersion
    category = "Contributed"
    help = [ordered]@{
        online = "https://gitee.com/openLuat"
    }
    boards = @(
        [ordered]@{ name = "AIR780EPM Dev Board" }
    )
    toolsDependencies = @(
        [ordered]@{
            packager = "openluat"
            name = "air780epm-csdk"
            version = $CsdkVersion
        },
        [ordered]@{
            packager = "openluat"
            name = "gnu-rm"
            version = $GnuRmVersion
        }
    )
}

$platformArchiveEntry = Get-ArchiveEntry -Path $PlatformArchive -BaseUrl $BaseUrl
foreach ($property in $platformArchiveEntry.GetEnumerator()) {
    $platform[$property.Key] = $property.Value
}

$index = [ordered]@{
    packages = @(
        [ordered]@{
            name = "openluat"
            maintainer = "OpenLuat"
            websiteURL = "https://gitee.com/openLuat"
            email = ""
            platforms = @($platform)
            tools = @(
                [ordered]@{
                    name = "air780epm-csdk"
                    version = $CsdkVersion
                    systems = @(
                        (New-ToolSystem -HostName "i686-mingw32" -Archive $csdk)
                    )
                },
                [ordered]@{
                    name = "gnu-rm"
                    version = $GnuRmVersion
                    systems = @(
                        (New-ToolSystem -HostName "i686-mingw32" -Archive $gnuRm)
                    )
                }
            )
        }
    )
}

$outputFullPath = Resolve-RepoPath $OutputPath
$outputDirectory = Split-Path -Parent $outputFullPath
if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory -PathType Container)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

[System.IO.File]::WriteAllText($outputFullPath, (($index | ConvertTo-Json -Depth 10) + "`n"), [System.Text.UTF8Encoding]::new($false))
Write-Output "Package index draft: $outputFullPath"
