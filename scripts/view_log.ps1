param(
    [string]$ComPort = "auto",
    [int]$BaudRate = 921600,
    [switch]$TextMode,
    [switch]$NoProbe,
    [string]$LuatOSCliPath = ".\tools\luatos-cli-release\luatos-cli.exe",
    [string]$LuatOSCliSourcePath = ".\tools\luatos-cli\target\release\luatos-cli.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$cliCandidates = @(
    (Join-Path $repoRoot $LuatOSCliPath),
    (Join-Path $repoRoot $LuatOSCliSourcePath)
)
$cliFullPath = $cliCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if ($null -eq $cliFullPath) {
    throw "luatos-cli executable was not found. Run scripts\install_luatos_cli_release.ps1 first."
}

if ($TextMode) {
    & $cliFullPath log view --port $ComPort --baud $BaudRate
    exit $LASTEXITCODE
}

$args = @("log", "view-binary", "--port", $ComPort, "--baud", $BaudRate)
if (-not $NoProbe) {
    $args += "--probe"
}

& $cliFullPath @args
exit $LASTEXITCODE
