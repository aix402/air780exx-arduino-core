param(
    [switch]$CliVerbose,
    [switch]$KeepGoing,
    [switch]$SkipSoftware,
    [switch]$IncludeDistribution,
    [switch]$FlashDistribution,
    [string]$FlashComPort = "",
    [int]$LogDuration = 20
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$scriptHostPath = (Get-Process -Id $PID).Path

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

if (-not $SkipSoftware) {
    $verifyArgs = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", (Join-Path $PSScriptRoot "verify_csdk_prebuilt_arduino_flow.ps1")
    )
    if ($CliVerbose) {
        $verifyArgs += "-CliVerbose"
    }
    if ($KeepGoing) {
        $verifyArgs += "-KeepGoing"
    }

    Write-Host "==> Software acceptance: Arduino CLI + CSDK prebuilt direct-link flow"
    & $scriptHostPath @verifyArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Software acceptance failed with exit code $LASTEXITCODE"
    }
}

if ($IncludeDistribution) {
    $distributionArgs = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", (Join-Path $PSScriptRoot "verify_csdk_prebuilt_distribution.ps1"),
        "-Clean"
    )
    if ($CliVerbose) {
        $distributionArgs += "-CliVerbose"
    }

    Write-Host "==> Distribution acceptance: bundled CSDK prebuilt package"
    & $scriptHostPath @distributionArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Distribution acceptance failed with exit code $LASTEXITCODE"
    }
}

if (-not [string]::IsNullOrWhiteSpace($FlashComPort)) {
    if ($FlashDistribution -and -not $IncludeDistribution) {
        throw "-FlashDistribution requires -IncludeDistribution so the distribution firmware is freshly built"
    }

    if ($FlashDistribution) {
        $packageFile = Resolve-RepoPath ".arduino-cli-work\ComplexLibraryProbeDistPackage\ComplexLibraryProbe.ino.binpkg"
        $socFile = Resolve-RepoPath ".arduino-cli-work\ComplexLibraryProbeDistPackage\ComplexLibraryProbe.ino_ec718pm.soc"
        $hardwareName = "ComplexLibraryProbe distribution package"
        $passRegex = @(
            "\+ARDUINO: CTOR,PASS",
            "\+ARDUINO: COMPLEX_LIB_PROBE,VALUE,494780",
            "\+ARDUINO: COMPLEX_LIB_PROBE,PASS"
        )
        $failRegex = @("ASSERT", "PANIC", "FATAL", "\+ARDUINO: COMPLEX_LIB_PROBE,FAIL")
    }
    else {
        $packageFile = Resolve-RepoPath ".arduino-cli-work\Blink\Blink.ino.binpkg"
        $socFile = Resolve-RepoPath ".arduino-cli-work\Blink\Blink.ino_ec718pm.soc"
        $hardwareName = "Blink"
        $passRegex = @(
            "\+ARDUINO: AIR780EPM,READY",
            "\+ARDUINO: CTOR,PASS",
            "\+ARDUINO: BLINK,(HIGH|LOW)"
        )
        $failRegex = @("ASSERT", "PANIC", "FATAL")
    }
    Assert-File -Path $packageFile -Description "$hardwareName binpkg"
    Assert-File -Path $socFile -Description "$hardwareName soc"

    Write-Host "==> Hardware acceptance: flash $hardwareName on $FlashComPort"
    & $scriptHostPath `
        -NoProfile `
        -ExecutionPolicy Bypass `
        -File (Join-Path $PSScriptRoot "upload_core.ps1") `
        -ComPort $FlashComPort `
        -PackageFile $packageFile `
        -SocFile $socFile
    if ($LASTEXITCODE -ne 0) {
        throw "Hardware flash failed with exit code $LASTEXITCODE"
    }

    Write-Host "==> Hardware acceptance: verify $hardwareName log on $FlashComPort"
    & $scriptHostPath `
        -NoProfile `
        -ExecutionPolicy Bypass `
        -File (Join-Path $PSScriptRoot "verify_log.ps1") `
        -ComPort $FlashComPort `
        -Duration $LogDuration `
        -PassRegex $passRegex `
        -FailRegex $failRegex `
        -RequirePass
    if ($LASTEXITCODE -ne 0) {
        throw "Hardware log verification failed with exit code $LASTEXITCODE"
    }
}

Write-Host "Phase 1 CSDK prebuilt Arduino experiment acceptance passed."
