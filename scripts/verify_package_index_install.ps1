param(
    [string]$ArduinoCliPath = ".\tools\arduino-cli-release\arduino-cli.exe",
    [string]$SmokeRoot = "$env:LOCALAPPDATA\Arduino15-air780-smoke",
    [int]$Port = 8766,
    [switch]$Clean,
    [switch]$KeepSmokeRoot
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

function Assert-File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
}

function Invoke-ArduinoCli {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    & $arduinoCliFullPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "arduino-cli failed with exit code $LASTEXITCODE`: $($Arguments -join ' ')"
    }
}

function Invoke-SmokeCompile {
    param(
        [Parameter(Mandatory = $true)][string]$SketchPath,
        [Parameter(Mandatory = $true)][string]$BuildName
    )

    $buildPath = Join-Path $smokeFullRoot "build\$BuildName"
    if (Test-Path -LiteralPath $buildPath) {
        Remove-Item -LiteralPath $buildPath -Recurse -Force
    }

    Invoke-ArduinoCli -Arguments @(
        "--config-file", $configPath,
        "compile",
        "-b", "openluat:ec718pm:air780epm_dev",
        "--build-path", $buildPath,
        $SketchPath
    )
}

$arduinoCliFullPath = Resolve-RepoPath $ArduinoCliPath
Assert-File -Path $arduinoCliFullPath -Description "arduino-cli executable"

$smokeFullRoot = if ([System.IO.Path]::IsPathRooted($SmokeRoot)) {
    [System.IO.Path]::GetFullPath($SmokeRoot)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $repoRoot $SmokeRoot))
}

Write-Host "==> Package Arduino platform archive"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "package_arduino_platform.ps1") -Version "0.1.0" -Clean
if ($LASTEXITCODE -ne 0) {
    throw "Arduino platform packaging failed with exit code $LASTEXITCODE"
}

Write-Host "==> Generate package index draft"
$baseUrl = "http://127.0.0.1:$Port"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "generate_package_index_draft.ps1") -BaseUrl $baseUrl
if ($LASTEXITCODE -ne 0) {
    throw "Package index generation failed with exit code $LASTEXITCODE"
}

$releaseDir = Join-Path $repoRoot "dist\releases"
$server = Start-Process `
    -FilePath "py" `
    -ArgumentList @("-3", "-m", "http.server", [string]$Port, "--bind", "127.0.0.1") `
    -WorkingDirectory $releaseDir `
    -WindowStyle Hidden `
    -PassThru

try {
    Start-Sleep -Seconds 2

    if ($Clean -and (Test-Path -LiteralPath $smokeFullRoot)) {
        Remove-Item -LiteralPath $smokeFullRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $smokeFullRoot | Out-Null

    $dataDir = Join-Path $smokeFullRoot "packages-data"
    $downloadsDir = Join-Path $smokeFullRoot "downloads"
    $userDir = Join-Path $smokeFullRoot "sketchbook"
    New-Item -ItemType Directory -Force -Path $dataDir, $downloadsDir, $userDir | Out-Null

    $configPath = Join-Path $smokeFullRoot "arduino-cli.yaml"
    $config = @"
board_manager:
  additional_urls:
    - $baseUrl/package_openluat_ec718pm_index.draft.json
directories:
  data: $($dataDir.Replace("\", "/"))
  downloads: $($downloadsDir.Replace("\", "/"))
  user: $($userDir.Replace("\", "/"))
"@
    [System.IO.File]::WriteAllText($configPath, $config + "`n", [System.Text.UTF8Encoding]::new($false))

    Write-Host "==> Install platform from package index"
    Invoke-ArduinoCli -Arguments @("--config-file", $configPath, "core", "update-index")
    & $arduinoCliFullPath --config-file $configPath core uninstall openluat:ec718pm 2>$null | Write-Output
    Invoke-ArduinoCli -Arguments @("--config-file", $configPath, "core", "install", "openluat:ec718pm")

    Write-Host "==> Compile Blink from installed package"
    Invoke-SmokeCompile `
        -SketchPath (Resolve-RepoPath ".\examples\01.Basics\Blink") `
        -BuildName "Blink"

    Write-Host "==> Compile ComplexLibraryProbe from installed package"
    Invoke-SmokeCompile `
        -SketchPath (Resolve-RepoPath ".\examples\99.Experimental\ComplexLibraryProbe") `
        -BuildName "ComplexLibraryProbe"

    Write-Host "Package index install verification passed: $smokeFullRoot"
}
finally {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force
    }
    if (-not $KeepSmokeRoot -and $Clean -and (Test-Path -LiteralPath $smokeFullRoot)) {
        Remove-Item -LiteralPath $smokeFullRoot -Recurse -Force
    }
}
