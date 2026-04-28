param(
    [Parameter(Position = 0)]
    [string]$Command,
    [string]$SourceFile,
    [string]$OutputFile,
    [string]$BuildPath,
    [string]$ProjectName,
    [string]$SketchPath
)

$ErrorActionPreference = "Stop"

function Touch-File {
    param([Parameter(Mandatory = $true)][string]$Path)
    $dir = Split-Path -Parent $Path
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    Set-Content -Path $Path -Value "" -NoNewline
}

switch ($Command) {
    "preprocess-stdout" {
        if ($SourceFile) {
            Get-Content -LiteralPath $SourceFile
        }
    }
    "preprocess-copy" {
        if ([string]::IsNullOrWhiteSpace($OutputFile) -or $OutputFile.Equals("nul", [System.StringComparison]::OrdinalIgnoreCase)) {
            "// AIR780EPM Arduino bridge stages third-party libraries during the xmake combine step."
        }
        else {
            Copy-Item -LiteralPath $SourceFile -Destination $OutputFile -Force
        }
    }
    "touch-file" {
        Touch-File -Path $OutputFile
    }
    "combine-xmake-build" {
        & (Join-Path $PSScriptRoot "build_core.ps1") -SketchPath $SketchPath -ArduinoBuildPath $BuildPath
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }

        if (-not [string]::IsNullOrWhiteSpace($BuildPath)) {
            if (-not (Test-Path -LiteralPath $BuildPath)) {
                New-Item -ItemType Directory -Force -Path $BuildPath | Out-Null
            }
            $repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
            $outDir = Join-Path $repoRoot "runner\air780epm_runner\out"
            $project = if ([string]::IsNullOrWhiteSpace($ProjectName)) { "air780epm_runner" } else { $ProjectName }
            Copy-Item -LiteralPath (Join-Path $outDir "air780epm_runner.binpkg") -Destination (Join-Path $BuildPath "$project.binpkg") -Force
            Copy-Item -LiteralPath (Join-Path $outDir "air780epm_runner_ec718pm.soc") -Destination (Join-Path $BuildPath "$($project)_ec718pm.soc") -Force
        }
    }
    "report-size" {
        ".text 0"
        ".data 0"
        ".eeprom 0"
    }
    default {
        throw "Unknown Arduino CLI recipe command: $Command"
    }
}
