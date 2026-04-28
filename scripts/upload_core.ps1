param(
    [Parameter(Mandatory = $false)]
    [string]$ComPort,
    [string]$PackageFile = ".\runner\air780epm_runner\out\air780epm_runner.binpkg",
    [string]$SocFile = ".\runner\air780epm_runner\out\air780epm_runner_ec718pm.soc",
    [string]$LuatoolsPath = "F:\hezhou\luatools\Luatools_v3.exe",
    [string]$LuatOSCliPath = ".\tools\luatos-cli-release\luatos-cli.exe",
    [string]$LuatOSCliSourcePath = ".\tools\luatos-cli\target\release\luatos-cli.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return Resolve-Path -LiteralPath $Path
    }
    return Resolve-Path -LiteralPath (Join-Path $repoRoot $Path)
}

$packageFullPath = Resolve-RepoPath -Path $PackageFile
$socFullPath = Resolve-RepoPath -Path $SocFile
$cliCandidates = @(
    (Join-Path $repoRoot $LuatOSCliPath),
    (Join-Path $repoRoot $LuatOSCliSourcePath)
)
$cliFullPath = $cliCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if ($null -ne $cliFullPath) {
    if ([string]::IsNullOrWhiteSpace($ComPort)) {
        $ComPort = "COM3"
    }
    & $cliFullPath flash run --soc $socFullPath --port $ComPort
    exit $LASTEXITCODE
}

Write-Output "luatos-cli executable was not found."
Write-Output "Install release build: powershell -ExecutionPolicy Bypass -File .\scripts\install_luatos_cli_release.ps1"
Write-Output "Package file: $packageFullPath"
Write-Output "Manual fallback: open $LuatoolsPath and flash the package on COM3."
if (Test-Path -LiteralPath $LuatoolsPath) {
    Start-Process -FilePath $LuatoolsPath
}
exit 2
