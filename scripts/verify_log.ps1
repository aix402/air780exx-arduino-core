param(
    [string]$ComPort = "auto",
    [int]$BaudRate = 921600,
    [int]$Duration = 15,
    [string[]]$PassRegex = @(
        "\+ARDUINO: AIR780EPM,READY",
        "\+ARDUINO: CTOR,PASS",
        "\+ARDUINO: BLINK,(HIGH|LOW)"
    ),
    [string[]]$FailRegex = @("ASSERT", "PANIC", "FATAL"),
    [switch]$RequirePass,
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

function ConvertTo-ProcessArgument {
    param([Parameter(Mandatory = $true)][string]$Value)

    if ($Value -match '[\s"]') {
        return '"' + $Value.Replace('"', '\"') + '"'
    }
    return $Value
}

$arguments = if ($TextMode) {
    @("log", "view", "--port", $ComPort, "--baud", $BaudRate)
}
else {
    $items = @("log", "view-binary", "--port", $ComPort, "--baud", $BaudRate)
    if (-not $NoProbe) {
        $items += "--probe"
    }
    $items
}

$psi = [System.Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $cliFullPath
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.Arguments = ($arguments | ForEach-Object { ConvertTo-ProcessArgument $_ }) -join " "

$process = [System.Diagnostics.Process]::Start($psi)
Start-Sleep -Seconds $Duration
if (-not $process.HasExited) {
    $process.Kill()
    $process.WaitForExit()
}

$stdout = $process.StandardOutput.ReadToEnd()
$stderr = $process.StandardError.ReadToEnd()

if ($stdout) {
    Write-Output $stdout.TrimEnd()
}
if ($stderr) {
    Write-Output $stderr.TrimEnd()
}

$matched = @{}
foreach ($regex in $PassRegex) {
    $matched[$regex] = ($stdout -match $regex)
}

$failed = @($FailRegex | Where-Object { $stdout -match $_ })
if ($failed.Count -gt 0) {
    throw "Fail regex was observed on ${ComPort}: $($failed -join ', ')"
}

$missing = @($PassRegex | Where-Object { -not $matched[$_] })
if ($missing.Count -eq 0) {
    Write-Output "LOG_VERIFY: PASS"
}
else {
    Write-Output "LOG_VERIFY: NO_MATCH $($missing -join ', ')"
    if ($RequirePass) {
        throw "Pass regex was not observed on ${ComPort}: $($missing -join ', ')"
    }
}
