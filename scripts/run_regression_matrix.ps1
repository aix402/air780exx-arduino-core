param(
    [ValidateSet("smoke", "pinmap_contract", "sensor_io")]
    [string]$Profile = "smoke",
    [string]$Fqbn = "openluat:ec718pm:air780epm_dev",
    [string]$ComPort,
    [string]$LogPort = "auto",
    [int]$VerifyDuration = 15,
    [switch]$Upload,
    [switch]$Clean,
    [switch]$ContinueOnError
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Resolve-RepoPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $RelativePath))
}

function New-RegressionEntry {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$SketchPath,
        [string[]]$PassRegex = @()
    )

    return [PSCustomObject]@{
        Name = $Name
        SketchPath = $SketchPath
        PassRegex = @($PassRegex)
    }
}

function Get-ProfileEntries {
    param([Parameter(Mandatory = $true)][string]$ProfileName)

    switch ($ProfileName) {
        "smoke" {
            return @(
                (New-RegressionEntry -Name "Blink" -SketchPath "examples\01.Basics\Blink" -PassRegex @(
                    "\+ARDUINO: AIR780EPM,READY",
                    "\+ARDUINO: BLINK,(HIGH|LOW)"
                )),
                (New-RegressionEntry -Name "CoreApiP0Compile" -SketchPath "examples\00.Core\CoreApiP0Compile"),
                (New-RegressionEntry -Name "SerialApiCompile" -SketchPath "examples\02.Serial\SerialApiCompile"),
                (New-RegressionEntry -Name "BusApiP2Compile" -SketchPath "examples\03.Bus\BusApiP2Compile"),
                (New-RegressionEntry -Name "PwmApiCompile" -SketchPath "examples\04.PWM\PwmApiCompile")
            )
        }
        "pinmap_contract" {
            return @(
                (New-RegressionEntry -Name "PinReport" -SketchPath "validation_sketches\PinReport" -PassRegex @(
                    "\+ARDUINO: PIN_REPORT,READY"
                )),
                (New-RegressionEntry -Name "PinCapabilities" -SketchPath "validation_sketches\PinCapabilities" -PassRegex @(
                    "\+ARDUINO: PIN_CAPS,READY"
                )),
                (New-RegressionEntry -Name "ResourceBoundaryP4Report" -SketchPath "validation_sketches\ResourceBoundaryP4Report" -PassRegex @(
                    "\+ARDUINO: RESOURCE,READY"
                ))
            )
        }
        "sensor_io" {
            $entries = @(
                (New-RegressionEntry -Name "SHT40Wire" -SketchPath "examples\03.Bus\SHT40Wire" -PassRegex @(
                    "\+ARDUINO: SHT40,READY",
                    "\+ARDUINO: SHT40,PASS"
                ))
            )

            $analogSketch = Resolve-RepoPath "examples\06.Analog\AnalogReadReport"
            if (Test-Path -LiteralPath $analogSketch) {
                $entries += New-RegressionEntry -Name "AnalogReadReport" -SketchPath "examples\06.Analog\AnalogReadReport" -PassRegex @(
                    "\+ARDUINO: ADC,READY"
                )
            }

            return $entries
        }
        default {
            throw "Unknown regression profile: $ProfileName"
        }
    }
}

function Invoke-Compile {
    param([Parameter(Mandatory = $true)][pscustomobject]$Entry)

    Write-Output ("[regression] compile {0} -> {1}" -f $Entry.Name, $Entry.SketchPath)
    & (Join-Path $PSScriptRoot "arduino_cli_compile.ps1") `
        -SketchPath (Resolve-RepoPath $Entry.SketchPath) `
        -Fqbn $Fqbn `
        -Clean:$Clean
    if ($LASTEXITCODE -ne 0) {
        throw "Compile failed: $($Entry.Name)"
    }
}

function Invoke-UploadAndVerify {
    param([Parameter(Mandatory = $true)][pscustomobject]$Entry)

    if ([string]::IsNullOrWhiteSpace($ComPort)) {
        throw "The -ComPort parameter is required when -Upload is used."
    }

    Write-Output ("[regression] upload {0} -> {1} ({2})" -f $Entry.Name, $Entry.SketchPath, $ComPort)
    & (Join-Path $PSScriptRoot "arduino_cli_upload.ps1") `
        -SketchPath (Resolve-RepoPath $Entry.SketchPath) `
        -Fqbn $Fqbn `
        -ComPort $ComPort `
        -Clean:$Clean
    if ($LASTEXITCODE -ne 0) {
        throw "Upload failed: $($Entry.Name)"
    }

    if (@($Entry.PassRegex).Count -gt 0) {
        Write-Output ("[regression] verify {0}" -f $Entry.Name)
        & (Join-Path $PSScriptRoot "verify_log.ps1") `
            -ComPort $LogPort `
            -Duration $VerifyDuration `
            -PassRegex $Entry.PassRegex `
            -RequirePass
        if ($LASTEXITCODE -ne 0) {
            throw "Verification failed: $($Entry.Name)"
        }
    }
}

$entries = @(Get-ProfileEntries -ProfileName $Profile)
if ($entries.Count -eq 0) {
    throw "Regression profile '$Profile' resolved to no sketches."
}

$results = New-Object System.Collections.Generic.List[object]
foreach ($entry in $entries) {
    try {
        Invoke-Compile -Entry $entry
        if ($Upload) {
            Invoke-UploadAndVerify -Entry $entry
        }
        $results.Add([PSCustomObject]@{
            Name = $entry.Name
            Status = "PASS"
        }) | Out-Null
    }
    catch {
        $results.Add([PSCustomObject]@{
            Name = $entry.Name
            Status = "FAIL"
            Error = $_.Exception.Message
        }) | Out-Null

        Write-Error $_
        if (-not $ContinueOnError) {
            break
        }
    }
}

$failed = @($results | Where-Object { $_.Status -ne "PASS" })
foreach ($result in $results) {
    if ($result.Status -eq "PASS") {
        Write-Output ("[regression] result {0}: PASS" -f $result.Name)
    }
    else {
        Write-Output ("[regression] result {0}: FAIL ({1})" -f $result.Name, $result.Error)
    }
}

if ($failed.Count -gt 0) {
    throw ("REGRESSION: FAIL PROFILE={0} FAILED={1}" -f $Profile, ($failed.Name -join ","))
}

$mode = if ($Upload) { "compile+upload" } else { "compile-only" }
Write-Output ("REGRESSION: PASS PROFILE={0} MODE={1} COUNT={2}" -f $Profile, $mode, $entries.Count)
