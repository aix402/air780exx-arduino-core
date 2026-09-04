param(
    [Parameter(Mandatory = $false)]
    [string]$ComPort,
    [string]$PackageFile = ".\runner\air780epm_runner\out\air780epm_runner.binpkg",
    [string]$SocFile = ".\runner\air780epm_runner\out\air780epm_runner_ec718pm.soc",
    [string]$LuatoolsPath,
    [string]$LuatOSCliToolRoot,
    [string]$LuatOSCliPath = ".\tools\luatos-cli-release\luatos-cli.exe",
    [string]$LuatOSCliSourcePath = ".\tools\luatos-cli\target\release\luatos-cli.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $Path = $Path.Trim('"')
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return Resolve-Path -LiteralPath $Path
    }
    return Resolve-Path -LiteralPath (Join-Path $repoRoot $Path)
}

function Normalize-OptionalPath {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or $Path.Contains("{")) {
        return $null
    }
    return $Path.Trim('"')
}

$packageFullPath = Resolve-RepoPath -Path $PackageFile
$socFullPath = Resolve-RepoPath -Path $SocFile
$cliCandidates = @()
$normalizedLuatOSCliToolRoot = Normalize-OptionalPath -Path $LuatOSCliToolRoot
if ($null -ne $normalizedLuatOSCliToolRoot) {
    $cliCandidates += (Join-Path $normalizedLuatOSCliToolRoot "luatos-cli.exe")
}
$cliCandidates += foreach ($candidate in @($LuatOSCliPath, $LuatOSCliSourcePath)) {
    if ([System.IO.Path]::IsPathRooted($candidate)) {
        $candidate
    }
    else {
        Join-Path $repoRoot $candidate
    }
}
$cliFullPath = $cliCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if ($null -ne $cliFullPath) {
    if ([string]::IsNullOrWhiteSpace($ComPort)) {
        $ComPort = "COM3"
    }
    & $cliFullPath flash run --soc $socFullPath --port $ComPort
    exit $LASTEXITCODE
}

Write-Output "luatos-cli executable was not found."
if (-not [string]::IsNullOrWhiteSpace($LuatOSCliToolRoot)) {
    Write-Output "Arduino tool root: $LuatOSCliToolRoot"
}
Write-Output "Install release build: powershell -ExecutionPolicy Bypass -File .\scripts\install_luatos_cli_release.ps1"
Write-Output "Package file: $packageFullPath"
if (-not [string]::IsNullOrWhiteSpace($LuatoolsPath)) {
    Write-Output "Manual fallback: open $LuatoolsPath and flash the package on COM3."
}
if (-not [string]::IsNullOrWhiteSpace($LuatoolsPath) -and (Test-Path -LiteralPath $LuatoolsPath)) {
    Start-Process -FilePath $LuatoolsPath
}
exit 2
