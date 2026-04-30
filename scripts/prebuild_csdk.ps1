param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
. (Join-Path $PSScriptRoot "csdk_prebuild_stamp.ps1")
$manifestPath = Join-Path $repoRoot "runner\air780epm_runner\build\arduino_export_manifest.json"
$placeholderBuildPath = Join-Path $repoRoot ".arduino-cli-work\prebuild-csdk"
New-Item -ItemType Directory -Force -Path $placeholderBuildPath | Out-Null

# xmake validates the external Arduino build shape while loading the ELF target,
# even though this script only builds static prebuild targets.
$placeholderSketchDir = Join-Path $placeholderBuildPath "sketch"
$placeholderCoreDir = Join-Path $placeholderBuildPath "core"
New-Item -ItemType Directory -Force -Path $placeholderSketchDir | Out-Null
New-Item -ItemType Directory -Force -Path $placeholderCoreDir | Out-Null
[System.IO.File]::WriteAllBytes((Join-Path $placeholderSketchDir "prebuild_placeholder.o"), [byte[]]::new(0))
[System.IO.File]::WriteAllBytes((Join-Path $placeholderCoreDir "core.a"), [byte[]]::new(0))

$buildArgs = @{
    PrebuildOnly = $true
    UseArduinoCliObjects = $true
    ArduinoBuildPath = $placeholderBuildPath
}
if ($Clean) {
    $buildArgs["Clean"] = $true
}

& (Join-Path $PSScriptRoot "build_core.ps1") @buildArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& (Join-Path $PSScriptRoot "export_arduino_build_manifest.ps1") -OutputPath $manifestPath | Write-Output
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
Write-CsdkPrebuildStamp -Manifest $manifest | Write-Output

Write-Output "CSDK prebuild artifacts are ready."
Write-Output "Manifest: $manifestPath"
