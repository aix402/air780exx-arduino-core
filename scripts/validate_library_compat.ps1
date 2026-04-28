param(
    [string[]]$Case = @(),
    [string]$Fqbn = "openluat:ec718pm:air780epm_dev",
    [string]$ArduinoCliPath = ".\tools\arduino-cli-release\arduino-cli.exe",
    [switch]$Clean,
    [switch]$CliVerbose,
    [switch]$ContinueOnError
)

$ErrorActionPreference = "Stop"

function Get-RepoRoot {
    return [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

function Get-SketchbookLibrariesRoot {
    return Join-Path ([Environment]::GetFolderPath("MyDocuments")) "Arduino\libraries"
}

function New-LibraryCompatCase {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [Parameter(Mandatory = $true)][string]$Library,
        [Parameter(Mandatory = $true)][string]$SketchPath,
        [Parameter(Mandatory = $true)][string]$Tier,
        [Parameter(Mandatory = $true)][string]$Hardware,
        [string[]]$RequiredLibraries = @(),
        [string]$Note = ""
    )

    return [PSCustomObject]@{
        Id = $Id
        Library = $Library
        SketchPath = $SketchPath
        Tier = $Tier
        Hardware = $Hardware
        RequiredLibraries = $RequiredLibraries
        Note = $Note
    }
}

function Get-ArduinoLibraryDisplayName {
    param([Parameter(Mandatory = $true)][string]$LibraryRoot)

    $propertiesPath = Join-Path $LibraryRoot "library.properties"
    if (-not (Test-Path -LiteralPath $propertiesPath)) {
        return ""
    }

    foreach ($line in Get-Content -LiteralPath $propertiesPath) {
        if ($line -match "^\s*name\s*=\s*(.+?)\s*$") {
            return $Matches[1]
        }
    }

    return ""
}

function Test-ArduinoLibraryInstalled {
    param([Parameter(Mandatory = $true)][string]$LibraryName)

    $librariesRoot = Get-SketchbookLibrariesRoot
    if (-not (Test-Path -LiteralPath $librariesRoot)) {
        return $false
    }

    foreach ($libraryDirectory in Get-ChildItem -LiteralPath $librariesRoot -Directory) {
        if ($libraryDirectory.Name -eq $LibraryName) {
            return $true
        }

        $displayName = Get-ArduinoLibraryDisplayName -LibraryRoot $libraryDirectory.FullName
        if ($displayName -eq $LibraryName) {
            return $true
        }
    }

    return $false
}

function Get-MissingArduinoLibraries {
    param([string[]]$RequiredLibraries)

    $missing = @()
    foreach ($libraryName in $RequiredLibraries) {
        if (-not (Test-ArduinoLibraryInstalled -LibraryName $libraryName)) {
            $missing += $libraryName
        }
    }

    return $missing
}

function Get-LibraryCompatCatalog {
    $repoRoot = Get-RepoRoot
    $librariesRoot = Get-SketchbookLibrariesRoot

    return @(
        New-LibraryCompatCase `
            -Id "arduinojson_runtime_smoke" `
            -Library "ArduinoJson" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\ArduinoJsonRuntimeSmoke") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "none" `
            -RequiredLibraries @("ArduinoJson") `
            -Note "Exercises JSON parse, F() assignment, and serialization without external hardware."
        New-LibraryCompatCase `
            -Id "ntpclient_report" `
            -Library "NTPClient" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\NTPClientReport") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "cellular network" `
            -RequiredLibraries @("NTPClient") `
            -Note "Runtime validation uses WiFiUDP compatibility alias over the cellular UDP layer."
        New-LibraryCompatCase `
            -Id "pubsubclient_mqtts_ca_smoke" `
            -Library "PubSubClient" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\MqttsPubSubClientCaSmoke") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "cellular network" `
            -RequiredLibraries @("PubSubClient") `
            -Note "Runtime validation uses CellularClientSecure with a CA certificate."
        New-LibraryCompatCase `
            -Id "mqttclient_256dpi_smoke" `
            -Library "MQTT" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\Mqtt256dpiSmoke") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "cellular network" `
            -RequiredLibraries @("MQTT") `
            -Note "Runtime validation uses the 256dpi MQTTClient wrapper over CellularClient."
        New-LibraryCompatCase `
            -Id "onewire_basic_compile" `
            -Library "OneWire" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\OneWireCompile") `
            -Tier "compile-only" `
            -Hardware "optional 1-Wire device" `
            -RequiredLibraries @("OneWire") `
            -Note "Exercises OneWire object creation, search API, and GPIO timing helpers."
        New-LibraryCompatCase `
            -Id "dallas_temperature_compile" `
            -Library "DallasTemperature + OneWire" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\DallasTemperatureCompile") `
            -Tier "compile-only" `
            -Hardware "optional DS18B20" `
            -RequiredLibraries @("DallasTemperature", "OneWire") `
            -Note "Exercises DallasTemperature dependency chain without requiring a sensor response."
        New-LibraryCompatCase `
            -Id "u8g2_ssd1306_compile" `
            -Library "U8g2" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\U8g2Ssd1306Compile") `
            -Tier "compile-only" `
            -Hardware "optional SSD1306 I2C display" `
            -RequiredLibraries @("U8g2") `
            -Note "Exercises a common U8g2 I2C constructor and font path."
        New-LibraryCompatCase `
            -Id "adafruit_ssd1306_compile" `
            -Library "Adafruit SSD1306 + Adafruit GFX Library + Adafruit BusIO" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\AdafruitSsd1306Compile") `
            -Tier "compile-only" `
            -Hardware "optional SSD1306 I2C display" `
            -RequiredLibraries @("Adafruit SSD1306", "Adafruit GFX Library", "Adafruit BusIO") `
            -Note "Exercises Adafruit's common SSD1306 display dependency chain."
        New-LibraryCompatCase `
            -Id "rtclib_compile" `
            -Library "RTClib + Adafruit BusIO" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\RTClibCompile") `
            -Tier "compile-only" `
            -Hardware "optional DS3231/PCF8523 RTC" `
            -RequiredLibraries @("RTClib", "Adafruit BusIO") `
            -Note "Exercises DateTime/TimeSpan and RTC wrapper types without touching hardware."
        New-LibraryCompatCase `
            -Id "arduino_httpclient_compile" `
            -Library "ArduinoHttpClient" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\ArduinoHttpClientCompile") `
            -Tier "compile-only" `
            -Hardware "none" `
            -RequiredLibraries @("ArduinoHttpClient") `
            -Note "Exercises a common HTTP Client wrapper on top of CellularClient."
        New-LibraryCompatCase `
            -Id "arduino_mqttclient_compile" `
            -Library "ArduinoMqttClient" `
            -SketchPath (Join-Path $repoRoot "validation_sketches\ArduinoMqttClientCompile") `
            -Tier "compile-only" `
            -Hardware "none" `
            -RequiredLibraries @("ArduinoMqttClient") `
            -Note "Exercises the official Arduino MQTT client wrapper on top of CellularClient."
        New-LibraryCompatCase `
            -Id "sparkfun_scd4x_basic" `
            -Library "SparkFun SCD4x Arduino Library" `
            -SketchPath (Join-Path $librariesRoot "SparkFun_SCD4x_Arduino_Library\examples\Example1_BasicReadings") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "SCD40/SCD41 on Wire" `
            -RequiredLibraries @("SparkFun SCD4x Arduino Library") `
            -Note "Previously hardware-observed with CO2 output on AIR780EPM."
        New-LibraryCompatCase `
            -Id "sht40_basic" `
            -Library "SHT40" `
            -SketchPath (Join-Path $librariesRoot "SHT40\examples\Basic") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "SHT40 on Wire" `
            -RequiredLibraries @("SHT40") `
            -Note "Previously hardware-observed with temperature and humidity output on AIR780EPM."
        New-LibraryCompatCase `
            -Id "sensirion_sht4x_example_usage" `
            -Library "Sensirion I2C SHT4x + Sensirion Core" `
            -SketchPath (Join-Path $librariesRoot "Sensirion_I2C_SHT4x\examples\exampleUsage") `
            -Tier "compile-plus-runtime-verified" `
            -Hardware "SHT40 on Wire" `
            -RequiredLibraries @("Sensirion I2C SHT4x", "Sensirion Core") `
            -Note "Exercises an external I2C sensor dependency chain."
    )
}

function Select-LibraryCompatCases {
    param(
        [object[]]$Catalog,
        [string[]]$RequestedCaseIds
    )

    if ($RequestedCaseIds.Count -eq 0) {
        return $Catalog
    }

    $normalizedCaseIds = @()
    foreach ($caseToken in $RequestedCaseIds) {
        foreach ($caseId in ($caseToken -split ",")) {
            $trimmedCaseId = $caseId.Trim()
            if ($trimmedCaseId.Length -gt 0) {
                $normalizedCaseIds += $trimmedCaseId
            }
        }
    }

    $selected = @()
    foreach ($caseId in $normalizedCaseIds) {
        $match = $Catalog | Where-Object { $_.Id -eq $caseId } | Select-Object -First 1
        if (-not $match) {
            throw "Unknown library compatibility case: $caseId"
        }
        $selected += $match
    }

    return $selected
}

$compileScript = Join-Path $PSScriptRoot "arduino_cli_compile.ps1"
$catalog = Get-LibraryCompatCatalog
$selectedCases = @(Select-LibraryCompatCases -Catalog $catalog -RequestedCaseIds $Case)
$results = @()

foreach ($compatCase in $selectedCases) {
    if (-not (Test-Path -LiteralPath $compatCase.SketchPath)) {
        Write-Output "[library_compat] SKIP $($compatCase.Id) - sketch not found: $($compatCase.SketchPath)"
        $results += [PSCustomObject]@{
            Id = $compatCase.Id
            Result = "SKIP"
            Detail = "Sketch not found"
        }
        continue
    }

    $missingLibraries = @(Get-MissingArduinoLibraries -RequiredLibraries $compatCase.RequiredLibraries)
    if ($missingLibraries.Count -gt 0) {
        Write-Output "[library_compat] SKIP $($compatCase.Id) - missing library: $($missingLibraries -join ', ')"
        $results += [PSCustomObject]@{
            Id = $compatCase.Id
            Result = "SKIP"
            Detail = "Missing library: $($missingLibraries -join ', ')"
        }
        continue
    }

    Write-Output "[library_compat] COMPILE $($compatCase.Id) - $($compatCase.Library)"

    $compileArguments = @{
        SketchPath = $compatCase.SketchPath
        Fqbn = $Fqbn
        ArduinoCliPath = $ArduinoCliPath
    }

    if ($Clean) {
        $compileArguments.Clean = $true
    }

    if ($CliVerbose) {
        $compileArguments.CliVerbose = $true
    }

    & $compileScript @compileArguments
    if ($LASTEXITCODE -eq 0) {
        $results += [PSCustomObject]@{
            Id = $compatCase.Id
            Result = "PASS"
            Detail = $compatCase.Tier
        }
        continue
    }

    $results += [PSCustomObject]@{
        Id = $compatCase.Id
        Result = "FAIL"
        Detail = "compile exit $LASTEXITCODE"
    }

    if (-not $ContinueOnError) {
        break
    }
}

Write-Output "[library_compat] SUMMARY"
foreach ($result in $results) {
    Write-Output ("[library_compat] {0,-36} {1,-5} {2}" -f $result.Id, $result.Result, $result.Detail)
}

if (($results | Where-Object { $_.Result -eq "FAIL" } | Measure-Object).Count -gt 0) {
    exit 1
}

exit 0
