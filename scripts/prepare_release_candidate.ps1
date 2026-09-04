param(
    [string]$OutputDirectory = ".\dist\release-candidate",
    [string]$ReleasesDirectory = ".\dist\releases",
    [string]$CsdkDistributionDirectory = ".\dist\csdk-prebuilt-air780epm-notoolchain",
    [string]$BaseUrl = "https://github.com/aix402/air780exx-arduino-core/releases/download/v0.2.0",
    [string]$PlatformVersion = "0.2.0",
    [string]$CsdkVersion = "0.2.0",
    [string]$GnuRmVersion = "10.2.1-ec718",
    [string]$LuatOSCliVersion = "1.8.0",
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

$outputRoot = Resolve-RepoPath $OutputDirectory
$releasesRoot = Resolve-RepoPath $ReleasesDirectory

if ($Clean -and (Test-Path -LiteralPath $outputRoot)) {
    Remove-Item -LiteralPath $outputRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

Write-Host "==> Link default runner and validate static constructor map"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build_core.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "Default runner link failed with exit code $LASTEXITCODE"
}
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "check_static_ctors_map.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "Static constructor map validation failed with exit code $LASTEXITCODE"
}

Write-Host "==> Build clean CSDK/runner prebuild and export manifest"
$prebuildArgs = @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", (Join-Path $PSScriptRoot "prebuild_csdk.ps1")
)
if ($Clean) {
    $prebuildArgs += "-Clean"
}
& pwsh @prebuildArgs
if ($LASTEXITCODE -ne 0) {
    throw "CSDK/runner prebuild and manifest export failed with exit code $LASTEXITCODE"
}

Write-Host "==> Export no-toolchain CSDK prebuilt distribution"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "export_csdk_prebuilt_distribution.ps1") `
    -ManifestPath ".\runner\air780epm_runner\build\arduino_export_manifest.json" `
    -OutputDirectory $CsdkDistributionDirectory `
    -NoToolchain `
    -Clean
if ($LASTEXITCODE -ne 0) {
    throw "No-toolchain CSDK prebuilt distribution export failed with exit code $LASTEXITCODE"
}

Write-Host "==> Package no-toolchain CSDK tool archive"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "package_csdk_prebuilt_distribution.ps1") `
    -DistributionDirectory $CsdkDistributionDirectory `
    -OutputDirectory $releasesRoot `
    -Version "$CsdkVersion-notoolchain-toolroot" `
    -ArchiveLayout ToolRoot `
    -ToolArchiveRoot "air780epm-csdk" `
    -Clean
if ($LASTEXITCODE -ne 0) {
    throw "No-toolchain CSDK tool archive packaging failed with exit code $LASTEXITCODE"
}

Write-Host "==> Package Arduino platform archive"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "package_arduino_platform.ps1") `
    -OutputDirectory $releasesRoot `
    -Version $PlatformVersion `
    -Clean
if ($LASTEXITCODE -ne 0) {
    throw "Arduino platform packaging failed with exit code $LASTEXITCODE"
}

Write-Host "==> Package luatos-cli tool archive"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "package_luatos_cli_tool.ps1") `
    -OutputDirectory $releasesRoot `
    -Version $LuatOSCliVersion `
    -Clean
if ($LASTEXITCODE -ne 0) {
    throw "luatos-cli tool packaging failed with exit code $LASTEXITCODE"
}

$platformArchive = Join-Path $releasesRoot "air780-arduino-platform-$PlatformVersion.zip"
$csdkArchive = Join-Path $releasesRoot "csdk-prebuilt-air780epm-$CsdkVersion-notoolchain-toolroot.zip"
$gnuRmArchive = Join-Path $releasesRoot "gnu-rm-$GnuRmVersion.zip"
$luatosCliArchive = Join-Path $releasesRoot "luatos-cli-$LuatOSCliVersion.zip"

Assert-File -Path $platformArchive -Description "Arduino platform archive"
Assert-File -Path $csdkArchive -Description "CSDK ABI tool archive"
Assert-File -Path $gnuRmArchive -Description "GNU Arm toolchain archive"
Assert-File -Path $luatosCliArchive -Description "luatos-cli archive"

$candidateIndex = Join-Path $outputRoot "package_air780_index.json"

Write-Host "==> Generate release candidate package index"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "generate_package_index_draft.ps1") `
    -OutputPath $candidateIndex `
    -BaseUrl $BaseUrl `
    -PlatformArchive $platformArchive `
    -CsdkArchive $csdkArchive `
    -GnuRmArchive $gnuRmArchive `
    -LuatOSCliArchive $luatosCliArchive `
    -PlatformVersion $PlatformVersion `
    -CsdkVersion $CsdkVersion `
    -GnuRmVersion $GnuRmVersion `
    -LuatOSCliVersion $LuatOSCliVersion
if ($LASTEXITCODE -ne 0) {
    throw "Package index generation failed with exit code $LASTEXITCODE"
}

$releaseFiles = @(
    $candidateIndex,
    $platformArchive,
    $csdkArchive,
    $gnuRmArchive,
    $luatosCliArchive
)

Write-Host "==> Copy release candidate files"
foreach ($file in $releaseFiles) {
    $source = [System.IO.Path]::GetFullPath($file)
    $destination = [System.IO.Path]::GetFullPath((Join-Path $outputRoot (Split-Path -Leaf $file)))
    if ($source -ne $destination) {
        Copy-Item -LiteralPath $source -Destination $destination -Force
    }
}

$manifest = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    base_url = $BaseUrl
    package_index = "package_air780_index.json"
    files = @(
        foreach ($file in $releaseFiles) {
            $item = Get-Item -LiteralPath $file
            [ordered]@{
                name = $item.Name
                size = [Int64]$item.Length
                sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
    )
}
$manifestPath = Join-Path $outputRoot "release-candidate.manifest.json"
[System.IO.File]::WriteAllText($manifestPath, (($manifest | ConvertTo-Json -Depth 5) + "`n"), [System.Text.UTF8Encoding]::new($false))

Write-Output "Release candidate directory: $outputRoot"
Get-ChildItem -LiteralPath $outputRoot -File | Sort-Object Name | Select-Object Name, Length
