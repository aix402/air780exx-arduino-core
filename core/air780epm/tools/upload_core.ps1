param(
    [string]$ComPort = "COM3",
    [string]$PackageFile,
    [string]$SocFile
)

$ErrorActionPreference = "Stop"

function Get-Air780exxRepoRoot {
    $platformDir = Resolve-Path (Join-Path $PSScriptRoot "..")
    $platformItem = Get-Item -LiteralPath $platformDir -Force
    $platformSource = $platformDir.Path

    if (($platformItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 -and $platformItem.Target) {
        $platformSource = [string]$platformItem.Target
    }

    return Resolve-Path (Join-Path $platformSource "..\..")
}

$repoRoot = Get-Air780exxRepoRoot
$uploadScript = Join-Path $repoRoot "scripts\upload_core.ps1"

if (-not (Test-Path -LiteralPath $uploadScript)) {
    throw "AIR780EXX upload script was not found: $uploadScript"
}

& $uploadScript -ComPort $ComPort -PackageFile $PackageFile -SocFile $SocFile
exit $LASTEXITCODE
