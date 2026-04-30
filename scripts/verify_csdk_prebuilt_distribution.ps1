param(
    [switch]$CliVerbose,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$distributionRoot = Join-Path $repoRoot "dist\csdk-prebuilt-air780epm"
$distributionManifest = Join-Path $distributionRoot "arduino_export_manifest.json"

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

function Assert-TextContains {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Description
    )

    Assert-File -Path $Path -Description $Description
    if (-not (Select-String -LiteralPath $Path -Pattern $Pattern -SimpleMatch -Quiet)) {
        throw "$Description did not contain expected text '$Pattern': $Path"
    }
}

function Invoke-DistributionCompile {
    param(
        [Parameter(Mandatory = $true)][string]$SketchPath,
        [Parameter(Mandatory = $true)][string]$BuildName,
        [Parameter(Mandatory = $true)][string]$ProjectName
    )

    $compileArgs = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", (Join-Path $PSScriptRoot "arduino_cli_compile.ps1"),
        "-SketchPath", (Resolve-RepoPath $SketchPath),
        "-BuildPath", (Resolve-RepoPath ".arduino-cli-work\$BuildName"),
        "-Clean"
    )
    if ($CliVerbose) {
        $compileArgs += "-CliVerbose"
    }

    $env:AIR780EPM_ARDUINO_MANIFEST_PATH = $distributionManifest
    try {
        & pwsh @compileArgs
        if ($LASTEXITCODE -ne 0) {
            throw "Arduino CLI compile failed for $BuildName with exit code $LASTEXITCODE"
        }
    }
    finally {
        Remove-Item Env:\AIR780EPM_ARDUINO_MANIFEST_PATH -ErrorAction SilentlyContinue
    }

    $buildPath = Resolve-RepoPath ".arduino-cli-work\$BuildName"
    Assert-File -Path (Join-Path $buildPath "$ProjectName.elf") -Description "$BuildName ELF"
    Assert-File -Path (Join-Path $buildPath "$ProjectName.binpkg") -Description "$BuildName binpkg"
    Assert-File -Path (Join-Path $buildPath "$($ProjectName)_ec718pm.soc") -Description "$BuildName soc"
    Assert-TextContains `
        -Path (Join-Path $buildPath "direct-link\$ProjectName.link.rsp") `
        -Pattern "dist/csdk-prebuilt-air780epm" `
        -Description "$BuildName direct-link response"
}

Write-Host "==> Export CSDK prebuilt distribution"
$exportArgs = @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", (Join-Path $PSScriptRoot "export_csdk_prebuilt_distribution.ps1")
)
if ($Clean) {
    $exportArgs += "-Clean"
}
& pwsh @exportArgs
if ($LASTEXITCODE -ne 0) {
    throw "CSDK prebuilt distribution export failed with exit code $LASTEXITCODE"
}

Assert-File -Path $distributionManifest -Description "CSDK prebuilt distribution manifest"
$manifest = Get-Content -Raw -LiteralPath $distributionManifest | ConvertFrom-Json
if (-not [bool]$manifest.distribution_package) {
    throw "Distribution manifest is missing distribution_package=true"
}
if ([string]$manifest.distribution_copy_policy -ne "generated-linker-mem-map-reduced-headers-required-link-libraries-and-toolchain") {
    throw "Distribution manifest has unexpected copy policy: $($manifest.distribution_copy_policy)"
}
Assert-File -Path ([string]$manifest.link.preprocessed_linker_script) -Description "Distribution preprocessed linker script"
Assert-TextContains `
    -Path ([string]$manifest.link.preprocessed_linker_script) `
    -Pattern "__arduino_init_array_start" `
    -Description "Distribution preprocessed linker script"
Assert-File -Path ([string]$manifest.package.mem_map) -Description "Distribution preprocessed memory map"
Assert-TextContains `
    -Path ([string]$manifest.package.mem_map) `
    -Pattern "AP_FLASH_LOAD_ADDR" `
    -Description "Distribution preprocessed memory map"
if (Test-Path -LiteralPath (Join-Path $distributionRoot "abi\linker\air780epm_flash.arduino_ctor.c") -PathType Leaf) {
    throw "Distribution should not ship the intermediate linker template"
}
$sdkMemoryMapHeaders = @(
    Get-ChildItem `
        -LiteralPath (Join-Path $distributionRoot "external\luatos-soc-2024\PLAT\device\target\board\ec7xx_0h00\common\inc") `
        -Filter "mem_map*.h" `
        -File `
        -ErrorAction SilentlyContinue
)
if ($sdkMemoryMapHeaders.Count -gt 0) {
    throw "Distribution should not ship SDK memory-map headers: $($sdkMemoryMapHeaders.Name -join ', ')"
}
if (Test-Path -LiteralPath (Join-Path $distributionRoot "external\luatos-soc-2024\PLAT\prebuild\PLAT\inc\osanvm.h") -PathType Leaf) {
    throw "Distribution should not ship SDK NVM header osanvm.h"
}
foreach ($relativeNetworkHeader in @(
    "external\luatos-soc-2024\PLAT\middleware\developed\cms\psdial\inc\psdial_ps_ctrl.h",
    "external\luatos-soc-2024\PLAT\middleware\developed\cms\psdial\inc\psdial.h",
    "external\luatos-soc-2024\PLAT\middleware\developed\ecapi\psapi\inc\ps_lib_api.h",
    "external\luatos-soc-2024\PLAT\middleware\thirdparty\lwip\src\include\lwip_config_cat.h",
    "external\luatos-soc-2024\PLAT\os\freertos\CMSIS\ap\inc\cmsis_os2.h",
    "external\luatos-soc-2024\PLAT\prebuild\PLAT\inc\osasys.h",
    "external\luatos-soc-2024\PLAT\prebuild\PS\inc\cmips.h",
    "external\luatos-soc-2024\PLAT\prebuild\PS\inc\networkmgr.h"
)) {
    if (Test-Path -LiteralPath (Join-Path $distributionRoot $relativeNetworkHeader) -PathType Leaf) {
        throw "Distribution should not ship SDK network-private header: $relativeNetworkHeader"
    }
}
$toolchainPaths = @($manifest.toolchain.bin, $manifest.toolchain.cc, $manifest.toolchain.cxx, $manifest.toolchain.ar, $manifest.toolchain.objcopy, $manifest.toolchain.objdump, $manifest.toolchain.size)
foreach ($toolPath in $toolchainPaths) {
    if (-not ([string]$toolPath).StartsWith($distributionRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Distribution toolchain path is outside distribution root: $toolPath"
    }
    if ([string]$toolPath -match '\\.xmake\\') {
        throw "Distribution toolchain path still references xmake cache: $toolPath"
    }
}
Assert-Directory -Path ([string]$manifest.toolchain.bin) -Description "Distribution toolchain bin directory"
foreach ($toolPath in @($manifest.toolchain.cc, $manifest.toolchain.cxx, $manifest.toolchain.ar, $manifest.toolchain.objcopy, $manifest.toolchain.objdump, $manifest.toolchain.size)) {
    Assert-File -Path ([string]$toolPath) -Description "Distribution toolchain file"
}

$distributionSizeBytes = (Get-ChildItem -LiteralPath $distributionRoot -Recurse -File | Measure-Object -Property Length -Sum).Sum
$distributionSizeMb = [math]::Round($distributionSizeBytes / 1MB, 2)
Write-Host "Distribution size: $distributionSizeMb MB"
if ($distributionSizeBytes -gt (900MB)) {
    throw "Distribution package is larger than expected with bundled toolchain: $distributionSizeMb MB"
}

Write-Host "==> Verify Blink with CSDK prebuilt distribution"
Invoke-DistributionCompile `
    -SketchPath "examples\01.Basics\Blink" `
    -BuildName "BlinkDistPackage" `
    -ProjectName "Blink.ino"

Write-Host "==> Verify ComplexLibraryProbe with CSDK prebuilt distribution"
Invoke-DistributionCompile `
    -SketchPath "examples\99.Experimental\ComplexLibraryProbe" `
    -BuildName "ComplexLibraryProbeDistPackage" `
    -ProjectName "ComplexLibraryProbe.ino"

Write-Host "==> Verify OtaApiReport with CSDK prebuilt distribution"
Invoke-DistributionCompile `
    -SketchPath "examples\11.OTA\OtaApiReport" `
    -BuildName "OtaApiReportDistPackage" `
    -ProjectName "OtaApiReport.ino"

Write-Host "==> Verify SleepReport with CSDK prebuilt distribution"
Invoke-DistributionCompile `
    -SketchPath "examples\12.Sleep\SleepReport" `
    -BuildName "SleepReportDistPackage" `
    -ProjectName "SleepReport.ino"

Write-Host "CSDK prebuilt distribution verification passed."
