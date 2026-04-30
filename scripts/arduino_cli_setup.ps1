param(
    [string]$PackageName = "openluat",
    [string]$Architecture = "ec718pm",
    [switch]$UseSystemData
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$platformSource = Join-Path $repoRoot "core\air780epm"
$hardwareVendorDir = Join-Path $repoRoot "hardware\$PackageName"
$platformLink = Join-Path $hardwareVendorDir $Architecture
$configDir = Join-Path $repoRoot ".arduino-cli-config"
$configPath = Join-Path $configDir "arduino-cli.yaml"
$systemArduinoData = Join-Path $env:LOCALAPPDATA "Arduino15"

if (-not (Test-Path -LiteralPath (Join-Path $platformSource "platform.txt"))) {
    throw "Arduino platform source was not found: $platformSource"
}

New-Item -ItemType Directory -Force -Path $hardwareVendorDir | Out-Null

if (Test-Path -LiteralPath $platformLink) {
    $item = Get-Item -LiteralPath $platformLink -Force
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -eq 0) {
        throw "Path already exists and is not a junction: $platformLink"
    }
}
else {
    New-Item -ItemType Junction -Path $platformLink -Target $platformSource | Out-Null
}

New-Item -ItemType Directory -Force -Path $configDir | Out-Null

if ($UseSystemData -and (Test-Path -LiteralPath (Join-Path $systemArduinoData "library_index.json"))) {
    $dataDir = $systemArduinoData
}
else {
    $dataDir = Join-Path $repoRoot ".arduino-cli-data"
}

$downloadsDir = Join-Path $repoRoot ".arduino-cli-downloads"
New-Item -ItemType Directory -Force -Path $dataDir | Out-Null
New-Item -ItemType Directory -Force -Path $downloadsDir | Out-Null

$repoYamlPath = $repoRoot.Path.Replace("\", "/")
$dataYamlPath = ([System.IO.Path]::GetFullPath($dataDir)).Replace("\", "/")
$downloadsYamlPath = ([System.IO.Path]::GetFullPath($downloadsDir)).Replace("\", "/")
$config = @"
directories:
  data: $dataYamlPath
  downloads: $downloadsYamlPath
  user: $repoYamlPath
"@

[System.IO.File]::WriteAllText($configPath, $config + "`n", [System.Text.UTF8Encoding]::new($false))

Write-Output "Arduino platform: $PackageName`:$Architecture"
Write-Output "FQBN: $PackageName`:$Architecture`:air780epm_dev"
Write-Output "Data: $dataDir"
Write-Output "Config: $configPath"
