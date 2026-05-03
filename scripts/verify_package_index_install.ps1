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
        [Parameter(Mandatory = $true)][string]$BuildName,
        [switch]$UseDefaultBuildPath
    )

    $buildPath = Join-Path $smokeFullRoot "build\$BuildName"
    if (-not $UseDefaultBuildPath -and (Test-Path -LiteralPath $buildPath)) {
        Remove-Item -LiteralPath $buildPath -Recurse -Force
    }

    $compileArgs = @(
        "--config-file", $configPath,
        "compile",
        "-b", "air780:air780:air780epm_dev",
        $SketchPath
    )
    if (-not $UseDefaultBuildPath) {
        $compileArgs = @(
            "--config-file", $configPath,
            "compile",
            "-b", "air780:air780:air780epm_dev",
            "--build-path", $buildPath,
            $SketchPath
        )
    }
    else {
        $compileArgs += "--clean"
    }

    Invoke-ArduinoCli -Arguments $compileArgs
}

function Copy-SmokeLibrary {
    param([Parameter(Mandatory = $true)][string]$LibraryName)

    $sourceRoot = Resolve-RepoPath ".\libraries\$LibraryName"
    if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
        throw "Smoke library was not found: $sourceRoot"
    }

    $destinationRoot = Join-Path $userDir "libraries\$LibraryName"
    if (Test-Path -LiteralPath $destinationRoot) {
        Remove-Item -LiteralPath $destinationRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destinationRoot) | Out-Null
    Copy-Item -LiteralPath $sourceRoot -Destination $destinationRoot -Recurse -Force
}

function Assert-InstalledPlatformShape {
    param([Parameter(Mandatory = $true)][string]$PlatformRoot)

    $requiredExample = Join-Path $PlatformRoot "libraries\AIR780\examples\01.Basics\Blink\Blink.ino"
    Assert-File -Path $requiredExample -Description "installed Blink example"

    $unexpectedPaths = @(
        (Join-Path $PlatformRoot "examples"),
        (Join-Path $PlatformRoot "examples\00.Core"),
        (Join-Path $PlatformRoot "examples\99.Experimental"),
        (Join-Path $PlatformRoot "libraries\ArduinoJson"),
        (Join-Path $PlatformRoot "libraries\Air780EpmComplexLibProbe"),
        (Join-Path $PlatformRoot "libraries\Air780EpmLinkProbe")
    )
    foreach ($path in $unexpectedPaths) {
        if (Test-Path -LiteralPath $path) {
            throw "Unexpected release package content was installed: $path"
        }
    }
}

function Assert-Air780LibraryExamples {
    $examplesOutput = (& $arduinoCliFullPath --config-file $configPath lib examples AIR780 --fqbn air780:air780:air780epm_dev 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "arduino-cli lib examples AIR780 failed: $examplesOutput"
    }
    if (-not $examplesOutput.Contains("AIR780 (air780:air780@0.1.0)")) {
        throw "AIR780 platform library examples were not listed for the AIR780 board: $examplesOutput"
    }
    if (-not $examplesOutput.Contains("examples\01.Basics\") -or -not $examplesOutput.Contains("Blink")) {
        throw "AIR780 Blink example was not listed by arduino-cli: $examplesOutput"
    }
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

Write-Host "==> Package luatos-cli tool archive"
& pwsh -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "package_luatos_cli_tool.ps1") -Clean
if ($LASTEXITCODE -ne 0) {
    throw "luatos-cli tool packaging failed with exit code $LASTEXITCODE"
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
    - $baseUrl/package_air780_index.draft.json
directories:
  data: $($dataDir.Replace("\", "/"))
  downloads: $($downloadsDir.Replace("\", "/"))
  user: $($userDir.Replace("\", "/"))
"@
    [System.IO.File]::WriteAllText($configPath, $config + "`n", [System.Text.UTF8Encoding]::new($false))

    Write-Host "==> Install platform from package index"
    Invoke-ArduinoCli -Arguments @("--config-file", $configPath, "core", "update-index")
    & $arduinoCliFullPath --config-file $configPath core uninstall air780:air780 2>$null | Write-Output
    Invoke-ArduinoCli -Arguments @("--config-file", $configPath, "core", "install", "air780:air780")
    $installedLuatOSCli = Join-Path $dataDir "packages\air780\tools\luatos-cli\1.8.0\luatos-cli.exe"
    Assert-File -Path $installedLuatOSCli -Description "package-index installed luatos-cli"
    & $installedLuatOSCli --version | Write-Output

    $installedPlatformRoot = Join-Path $dataDir "packages\air780\hardware\air780\0.1.0"
    Assert-InstalledPlatformShape -PlatformRoot $installedPlatformRoot
    Assert-Air780LibraryExamples

    $installedBlink = Join-Path $installedPlatformRoot "libraries\AIR780\examples\01.Basics\Blink"
    if (-not (Test-Path -LiteralPath $installedBlink -PathType Container)) {
        throw "Installed Blink example was not found: $installedBlink"
    }

    Write-Host "==> Compile Blink from installed package with default Arduino build path"
    Invoke-SmokeCompile `
        -SketchPath $installedBlink `
        -BuildName "Blink" `
        -UseDefaultBuildPath

    Copy-SmokeLibrary -LibraryName "Air780EpmComplexLibProbe"
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
