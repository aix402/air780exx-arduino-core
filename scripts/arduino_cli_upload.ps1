param(
    [Parameter(Mandatory = $true)]
    [string]$SketchPath,
    [string]$ComPort = "COM3",
    [string]$Fqbn = "air780:air780:air780epm_dev",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$resolvedSketch = Resolve-Path -LiteralPath $SketchPath
$sketchItem = Get-Item -LiteralPath $resolvedSketch
$sketchDir = if ($sketchItem.PSIsContainer) { $sketchItem.FullName } else { $sketchItem.Directory.FullName }
$sketchName = Split-Path -Leaf $sketchDir
$buildPath = Join-Path $repoRoot ".arduino-cli-work\$sketchName"

& (Join-Path $PSScriptRoot "arduino_cli_compile.ps1") -SketchPath $sketchDir -Fqbn $Fqbn -BuildPath $buildPath -Clean:$Clean
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$package = Get-ChildItem -LiteralPath $buildPath -File -Filter "*.binpkg" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
$soc = Get-ChildItem -LiteralPath $buildPath -File -Filter "*_ec718pm.soc" |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if ($null -eq $package -or $null -eq $soc) {
    throw "Arduino CLI artifacts were not found in $buildPath"
}

& (Join-Path $PSScriptRoot "upload_core.ps1") `
    -ComPort $ComPort `
    -PackageFile $package.FullName `
    -SocFile $soc.FullName
exit $LASTEXITCODE
