param(
    [string]$SketchbookPath = "$env:USERPROFILE\Documents\Arduino",
    [string]$IdeCliConfigPath = "$env:USERPROFILE\.arduinoIDE\arduino-cli.yaml",
    [string]$Fqbn = "air780:air780:air780epm_dev",
    [string]$ComPort = "COM3",
    [switch]$NoBackup
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

$sketchbookFullPath = Resolve-RepoPath -Path $SketchbookPath
$configFullPath = Resolve-RepoPath -Path $IdeCliConfigPath
$platformSource = Join-Path $repoRoot "core\air780epm"
$platformVendorDir = Join-Path $sketchbookFullPath "hardware\air780"
$platformLink = Join-Path $platformVendorDir "air780"

if (-not (Test-Path -LiteralPath $sketchbookFullPath)) {
    throw "Sketchbook path does not exist: $sketchbookFullPath"
}

& (Join-Path $PSScriptRoot "arduino_cli_setup.ps1") | Write-Output

New-Item -ItemType Directory -Force -Path $platformVendorDir | Out-Null
if (Test-Path -LiteralPath $platformLink) {
    $item = Get-Item -LiteralPath $platformLink -Force
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0) {
        throw "Path already exists and is not a junction: $platformLink"
    }
}
else {
    New-Item -ItemType Junction -Path $platformLink -Target $platformSource | Out-Null
}

if (-not (Test-Path -LiteralPath $configFullPath)) {
    $configDir = Split-Path -Parent $configFullPath
    if ($configDir -and -not (Test-Path -LiteralPath $configDir)) {
        New-Item -ItemType Directory -Force -Path $configDir | Out-Null
    }
    $defaultConfig = @"
directories:
    data: $env:LOCALAPPDATA\Arduino15
    user: $sketchbookFullPath
"@
    [System.IO.File]::WriteAllText($configFullPath, $defaultConfig + "`n", [System.Text.UTF8Encoding]::new($false))
}
else {
    if (-not $NoBackup) {
        $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $backupPath = "$configFullPath.air780exx-backup-$timestamp"
        Copy-Item -LiteralPath $configFullPath -Destination $backupPath -Force
        Write-Output "Backup: $backupPath"
    }

    $content = [System.IO.File]::ReadAllText($configFullPath)
    $escapedSketchbook = $sketchbookFullPath.Replace("\", "\\")
    if ($content -match "(?m)^(\s*user:\s*).*$") {
        $content = [regex]::Replace($content, "(?m)^(\s*user:\s*).*$", "`${1}$escapedSketchbook", 1)
    }
    elseif ($content -match "(?m)^directories:\s*$") {
        $content = [regex]::Replace($content, "(?m)^directories:\s*$", "directories:`n    user: $escapedSketchbook", 1)
    }
    else {
        $content = "directories:`n    user: $escapedSketchbook`n" + $content
    }
    [System.IO.File]::WriteAllText($configFullPath, $content, [System.Text.UTF8Encoding]::new($false))
}

Write-Output "Arduino IDE sketchbook: $sketchbookFullPath"
Write-Output "Arduino IDE CLI config: $configFullPath"
Write-Output "Arduino IDE platform junction: $platformLink -> $platformSource"

$ideCli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
if (Test-Path -LiteralPath $ideCli) {
    $examples = @(
        (Join-Path $repoRoot "examples\01.Basics\Blink"),
        (Join-Path $repoRoot "examples\02.Serial\SerialEcho")
    )
    foreach ($example in $examples) {
        if (Test-Path -LiteralPath $example) {
            & $ideCli board attach --config-file $configFullPath -b $Fqbn -p $ComPort $example | Write-Output
            if ($LASTEXITCODE -ne 0) {
                exit $LASTEXITCODE
            }
        }
    }
}

Write-Output "Restart Arduino IDE if it is already running."
