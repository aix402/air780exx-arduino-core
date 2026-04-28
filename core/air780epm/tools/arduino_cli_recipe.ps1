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

function Get-Air780exxRepoRoot {
    $platformDir = Resolve-Path (Join-Path $PSScriptRoot "..")
    $platformItem = Get-Item -LiteralPath $platformDir -Force
    $platformSource = $platformDir.Path

    if (($platformItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 -and $platformItem.Target) {
        $platformSource = [string]$platformItem.Target
    }

    return Resolve-Path (Join-Path $platformSource "..\..")
}

$repoRoot = Get-Air780exxRepoRoot
$recipeScript = Join-Path $repoRoot "scripts\arduino_cli_recipe.ps1"

if (-not (Test-Path -LiteralPath $recipeScript)) {
    throw "AIR780EXX Arduino recipe script was not found: $recipeScript"
}

& $recipeScript `
    $Command `
    -SourceFile $SourceFile `
    -OutputFile $OutputFile `
    -BuildPath $BuildPath `
    -ProjectName $ProjectName `
    -SketchPath $SketchPath
exit $LASTEXITCODE
