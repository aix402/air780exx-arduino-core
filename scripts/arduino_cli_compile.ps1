param(
    [Parameter(Mandatory = $true)]
    [string]$SketchPath,
    [string]$Fqbn = "air780:air780:air780epm_dev",
    [string]$BuildPath,
    [string]$ArduinoCliPath = ".\tools\arduino-cli-release\arduino-cli.exe",
    [switch]$Clean,
    [switch]$CliVerbose
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$configPath = Join-Path $repoRoot ".arduino-cli-config\arduino-cli.yaml"
$commandCli = Get-Command arduino-cli -ErrorAction SilentlyContinue

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

$localCli = Resolve-RepoPath -Path $ArduinoCliPath

if (Test-Path -LiteralPath $localCli) {
    $cli = $localCli
}
elseif ($null -ne $commandCli) {
    $cli = $commandCli.Source
}
else {
    throw "arduino-cli was not found. Run scripts\install_arduino_cli_release.ps1 first."
}

& (Join-Path $PSScriptRoot "arduino_cli_setup.ps1") | Write-Output

$resolvedSketch = Resolve-Path -LiteralPath $SketchPath
$sketchItem = Get-Item -LiteralPath $resolvedSketch
$sketchDir = if ($sketchItem.PSIsContainer) { $sketchItem.FullName } else { $sketchItem.Directory.FullName }
$sketchName = Split-Path -Leaf $sketchDir

if ([string]::IsNullOrWhiteSpace($BuildPath)) {
    $BuildPath = Join-Path $repoRoot ".arduino-cli-work\$sketchName"
}
$buildFullPath = Resolve-RepoPath -Path $BuildPath
$workRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot ".arduino-cli-work"))

if ($Clean -and (Test-Path -LiteralPath $buildFullPath)) {
    if (-not $buildFullPath.StartsWith($workRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean build path outside .arduino-cli-work: $buildFullPath"
    }
    Remove-Item -LiteralPath $buildFullPath -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $buildFullPath | Out-Null

$args = @(
    "compile",
    "--config-file", $configPath,
    "--fqbn", $Fqbn,
    "--build-path", $buildFullPath
)
if ($CliVerbose) {
    $args += "--verbose"
}
$args += $sketchDir

& $cli @args
exit $LASTEXITCODE
