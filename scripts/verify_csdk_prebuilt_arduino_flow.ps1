param(
    [switch]$CliVerbose,
    [switch]$KeepGoing
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

function Assert-Directory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "$Description was not found: $Path"
    }
}

function Assert-TextContains {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Description
    )

    Assert-File -Path $Path -Description $Description
    if (-not (Select-String -LiteralPath $Path -Pattern $Pattern -SimpleMatch -Quiet)) {
        throw "$Description did not contain expected text '$Pattern': $Path"
    }
}

function Read-JsonFile {
    param([Parameter(Mandatory = $true)][string]$Path)

    Assert-File -Path $Path -Description "JSON file"
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Invoke-VerifyStep {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Script
    )

    Write-Host "==> $Name"
    try {
        & $Script
        Write-Host "PASS: $Name"
        return $true
    }
    catch {
        Write-Error "FAIL: $Name - $($_.Exception.Message)"
        if (-not $KeepGoing) {
            throw
        }
        return $false
    }
}

function Invoke-ArduinoCompile {
    param(
        [Parameter(Mandatory = $true)][string]$SketchPath,
        [Parameter(Mandatory = $true)][string]$SketchName
    )

    $compileArgs = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", (Join-Path $PSScriptRoot "arduino_cli_compile.ps1"),
        "-SketchPath", (Resolve-RepoPath $SketchPath),
        "-BuildPath", (Resolve-RepoPath ".arduino-cli-work\$SketchName"),
        "-Clean"
    )
    if ($CliVerbose) {
        $compileArgs += "-CliVerbose"
    }

    & pwsh @compileArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Arduino CLI compile failed for $SketchName with exit code $LASTEXITCODE"
    }
}

function Assert-IsolatedArduinoCli {
    $configPath = Resolve-RepoPath ".arduino-cli-config\arduino-cli.yaml"
    $cliPath = Resolve-RepoPath "tools\arduino-cli-release\arduino-cli.exe"
    $dataDir = Resolve-RepoPath ".arduino-cli-data"
    $downloadsDir = Resolve-RepoPath ".arduino-cli-downloads"
    $ctagsDir = Resolve-RepoPath ".arduino-cli-data\packages\builtin\tools\ctags"

    Assert-File -Path $cliPath -Description "Project-local arduino-cli"
    Assert-Directory -Path $dataDir -Description "Project-local Arduino CLI data directory"
    Assert-Directory -Path $downloadsDir -Description "Project-local Arduino CLI downloads directory"
    Assert-TextContains -Path $configPath -Pattern "data: $($dataDir.Replace('\', '/'))" -Description "Arduino CLI config"
    Assert-TextContains -Path $configPath -Pattern "downloads: $($downloadsDir.Replace('\', '/'))" -Description "Arduino CLI config"
    Assert-Directory -Path $ctagsDir -Description "Project-local builtin ctags package"
}

function Get-LocalArduinoCliPath {
    $cliPath = Resolve-RepoPath "tools\arduino-cli-release\arduino-cli.exe"
    Assert-File -Path $cliPath -Description "Project-local arduino-cli"
    return $cliPath
}

function Ensure-ArduinoLibrary {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Version
    )

    $libraryRoot = Resolve-RepoPath "libraries\$Name"
    $propertiesPath = Join-Path $libraryRoot "library.properties"
    $installedVersion = ""
    if (Test-Path -LiteralPath $propertiesPath -PathType Leaf) {
        foreach ($line in Get-Content -LiteralPath $propertiesPath) {
            if ($line -match "^\s*version\s*=\s*(.+?)\s*$") {
                $installedVersion = $Matches[1]
                break
            }
        }
    }

    if ($installedVersion -eq $Version) {
        Write-Host "Dependency present: $Name@$Version"
        return
    }

    & (Join-Path $PSScriptRoot "arduino_cli_setup.ps1") | Write-Output

    $cliPath = Get-LocalArduinoCliPath
    $configPath = Resolve-RepoPath ".arduino-cli-config\arduino-cli.yaml"
    Write-Host "Installing dependency: $Name@$Version"
    & $cliPath "--config-file" $configPath "lib" "install" "$Name@$Version"
    if ($LASTEXITCODE -ne 0) {
        throw "arduino-cli lib install failed for $Name@$Version with exit code $LASTEXITCODE"
    }

    Assert-TextContains -Path $propertiesPath -Pattern "name=$Name" -Description "$Name library metadata"
    Assert-TextContains -Path $propertiesPath -Pattern "version=$Version" -Description "$Name library metadata"
}

function Assert-FirmwareOutputs {
    param(
        [Parameter(Mandatory = $true)][string]$SketchName,
        [Parameter(Mandatory = $true)][string]$ProjectName
    )

    $buildPath = Resolve-RepoPath ".arduino-cli-work\$SketchName"
    Assert-File -Path (Join-Path $buildPath "$ProjectName.elf") -Description "$SketchName ELF"
    Assert-File -Path (Join-Path $buildPath "$ProjectName.map") -Description "$SketchName map"
    Assert-File -Path (Join-Path $buildPath "$ProjectName.binpkg") -Description "$SketchName binpkg"
    Assert-File -Path (Join-Path $buildPath "$($ProjectName)_ec718pm.soc") -Description "$SketchName soc"
    Assert-File -Path (Join-Path $buildPath "direct-link\$ProjectName.direct_link.json") -Description "$SketchName direct-link manifest"
}

function Assert-DirectLinkInputs {
    param(
        [Parameter(Mandatory = $true)][string]$SketchName,
        [Parameter(Mandatory = $true)][string]$ProjectName,
        [string[]]$ExpectedLibraryObjectNames = @(),
        [switch]$ExpectNoLibraryObjects
    )

    $buildPath = Resolve-RepoPath ".arduino-cli-work\$SketchName"
    $directManifestPath = Join-Path $buildPath "direct-link\$ProjectName.direct_link.json"
    $directManifest = Read-JsonFile -Path $directManifestPath
    $inputs = $directManifest.inputs

    if (@($inputs.sketch_objects).Count -lt 1) {
        throw "$SketchName direct-link manifest did not record sketch objects"
    }
    Assert-File -Path $inputs.core_archive -Description "$SketchName Arduino core archive"
    Assert-File -Path $inputs.runner_archive -Description "$SketchName runner archive"
    Assert-File -Path $inputs.csdk_archive -Description "$SketchName CSDK archive"

    foreach ($objectName in $ExpectedLibraryObjectNames) {
        $matched = @($inputs.library_objects | Where-Object {
            [System.IO.Path]::GetFileName($_) -eq $objectName
        })
        if ($matched.Count -eq 0) {
            throw "$SketchName direct-link manifest did not record library object: $objectName"
        }
    }

    if ($ExpectNoLibraryObjects -and @($inputs.library_objects).Count -ne 0) {
        throw "$SketchName direct-link manifest recorded unexpected library objects"
    }
}

function Assert-MapEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$SketchName,
        [Parameter(Mandatory = $true)][string]$ProjectName,
        [string[]]$ExpectedTexts = @()
    )

    $mapPath = Resolve-RepoPath ".arduino-cli-work\$SketchName\$ProjectName.map"
    Assert-TextContains -Path $mapPath -Pattern "$ProjectName.cpp.o" -Description "$SketchName map"
    Assert-TextContains -Path $mapPath -Pattern "core.a" -Description "$SketchName map"
    Assert-TextContains -Path $mapPath -Pattern ".arduino_init_array" -Description "$SketchName map"
    Assert-TextContains -Path $mapPath -Pattern "__arduino_init_array_start" -Description "$SketchName map"
    Assert-TextContains -Path $mapPath -Pattern "__arduino_init_array_end" -Description "$SketchName map"

    foreach ($text in $ExpectedTexts) {
        Assert-TextContains -Path $mapPath -Pattern $text -Description "$SketchName map"
    }
}

function Assert-DirectLinkOrder {
    param(
        [Parameter(Mandatory = $true)][string]$SketchName,
        [Parameter(Mandatory = $true)][string]$ProjectName
    )

    $manifestPath = Resolve-RepoPath ".arduino-cli-work\$SketchName\direct-link\$ProjectName.direct_link.json"
    $manifest = Read-JsonFile -Path $manifestPath
    $order = @($manifest.link_order)
    $expected = @(
        "vendor_whole_group",
        "sketch_objects",
        "library_objects",
        "library_archives",
        "core_archive",
        "runner_archive",
        "libm"
    )

    if ($order.Count -ne $expected.Count) {
        throw "$SketchName direct-link manifest recorded unexpected link order length"
    }

    for ($i = 0; $i -lt $expected.Count; $i++) {
        if ($order[$i] -ne $expected[$i]) {
            throw "$SketchName direct-link manifest link order[$i] was '$($order[$i])', expected '$($expected[$i])'"
        }
    }

    $responsePath = Resolve-RepoPath ".arduino-cli-work\$SketchName\direct-link\$ProjectName.link.rsp"
    Assert-File -Path $responsePath -Description "$SketchName direct-link response"
    $response = [System.IO.File]::ReadAllLines($responsePath)
    $firstVendor = [array]::FindIndex($response, [Predicate[string]]{ param($line) $line -eq "-lcsdk" })
    $firstCore = [array]::FindIndex($response, [Predicate[string]]{ param($line) $line -like "*core.a*" })
    $firstRunner = [array]::FindIndex($response, [Predicate[string]]{ param($line) $line -like "*libair780epm_runner.a*" })

    if ($firstVendor -lt 0 -or $firstCore -lt 0 -or $firstRunner -lt 0) {
        throw "$SketchName direct-link response file is missing vendor/core/runner link inputs"
    }
    if (-not ($firstVendor -lt $firstCore -and $firstCore -lt $firstRunner)) {
        throw "$SketchName direct-link response order did not keep CSDK before Arduino core before runner"
    }
}

function Read-BinpkgEntryNames {
    param([Parameter(Mandatory = $true)][string]$Path)

    Assert-File -Path $Path -Description "binpkg"
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x1D8) {
        throw "binpkg is too small: $Path"
    }

    $offset = 0x34
    $pkgmode = [System.Text.Encoding]::ASCII.GetString($bytes, 0x38, 7)
    if ($pkgmode -eq "pkgmode") {
        $offset = 0x1D8
    }

    $entryNames = [System.Collections.Generic.List[string]]::new()
    $entryMetaSize = 364
    while (($offset + $entryMetaSize) -le $bytes.Length) {
        $nameBytes = $bytes[$offset..($offset + 63)]
        $name = [System.Text.Encoding]::ASCII.GetString($nameBytes).Split([char]0)[0]
        $imageSize = [System.BitConverter]::ToUInt32($bytes, $offset + 76)
        [void]$entryNames.Add($name)
        $offset += $entryMetaSize + [int]$imageSize
    }

    return @($entryNames)
}

function Assert-BinpkgEntries {
    param(
        [Parameter(Mandatory = $true)][string]$SketchName,
        [Parameter(Mandatory = $true)][string]$ProjectName
    )

    $binpkgPath = Resolve-RepoPath ".arduino-cli-work\$SketchName\$ProjectName.binpkg"
    $entryNames = @(Read-BinpkgEntryNames -Path $binpkgPath)
    foreach ($expectedName in @("ap_bootloader", "ap", "cp-demo-flash")) {
        if ($entryNames -notcontains $expectedName) {
            throw "$SketchName binpkg did not contain expected entry '$expectedName'; entries: $($entryNames -join ', ')"
        }
    }
    if ($entryNames -contains $ProjectName) {
        throw "$SketchName binpkg used sketch project name '$ProjectName' as a flash entry; AP entry must be named 'ap'"
    }
}

$results = [System.Collections.Generic.List[object]]::new()

$results.Add((Invoke-VerifyStep -Name "Project-local Arduino CLI version" -Script {
    $cliPath = Resolve-RepoPath "tools\arduino-cli-release\arduino-cli.exe"
    Assert-File -Path $cliPath -Description "Project-local arduino-cli"
    & $cliPath version
    if ($LASTEXITCODE -ne 0) {
        throw "arduino-cli version failed with exit code $LASTEXITCODE"
    }
})) | Out-Null

$results.Add((Invoke-VerifyStep -Name "Platform recipes use CSDK prebuilt combine" -Script {
    $platformPath = Resolve-RepoPath "core\air780epm\platform.txt"
    Assert-TextContains -Path $platformPath -Pattern "combine-csdk-prebuilt" -Description "AIR780EPM platform recipes"
    Assert-TextContains -Path $platformPath -Pattern "report-size" -Description "AIR780EPM platform recipes"
})) | Out-Null

$results.Add((Invoke-VerifyStep -Name "Ensure ArduinoJson dependency" -Script {
    Ensure-ArduinoLibrary -Name "ArduinoJson" -Version "7.4.3"
})) | Out-Null

$cases = @(
    [ordered]@{
        Name = "Blink"
        SketchPath = "examples\01.Basics\Blink"
        ProjectName = "Blink.ino"
        ExpectedMapText = @()
        ExpectedLibraryObjects = @()
        ExpectNoLibraryObjects = $true
    },
    [ordered]@{
        Name = "ComplexLibraryProbe"
        SketchPath = "examples\99.Experimental\ComplexLibraryProbe"
        ProjectName = "ComplexLibraryProbe.ino"
        ExpectedMapText = @(
            "Air780EpmComplexLibProbe.cpp.o",
            "ComplexProbeC.c.o",
            "ComplexDetail.cpp.o",
            "air780epmComplexProbeValue",
            "air780epm_complex_probe_c_step",
            "air780epmComplexDetailValue"
        )
        ExpectedLibraryObjects = @(
            "Air780EpmComplexLibProbe.cpp.o",
            "ComplexProbeC.c.o",
            "ComplexDetail.cpp.o"
        )
        ExpectNoLibraryObjects = $false
    },
    [ordered]@{
        Name = "ArduinoJsonProbe"
        SketchPath = "examples\99.Experimental\ArduinoJsonProbe"
        ProjectName = "ArduinoJsonProbe.ino"
        ExpectedMapText = @(
            "ArduinoJsonProbe.ino.cpp.o",
            "JsonDocument"
        )
        ExpectedLibraryObjects = @()
        ExpectNoLibraryObjects = $true
    },
    [ordered]@{
        Name = "OtaApiReport"
        SketchPath = "examples\11.OTA\OtaApiReport"
        ProjectName = "OtaApiReport.ino"
        ExpectedMapText = @(
            "AIR780EPMOTA.cpp.o",
            "arduinoCoreOtaBegin"
        )
        ExpectedLibraryObjects = @()
        ExpectNoLibraryObjects = $true
    },
    [ordered]@{
        Name = "SleepReport"
        SketchPath = "examples\12.Sleep\SleepReport"
        ProjectName = "SleepReport.ino"
        ExpectedMapText = @(
            "AIR780EPMSleep.cpp.o",
            "arduinoCoreSleepDeep"
        )
        ExpectedLibraryObjects = @()
        ExpectNoLibraryObjects = $true
    }
)

foreach ($case in $cases) {
    $results.Add((Invoke-VerifyStep -Name "Compile and package $($case.Name)" -Script {
        Invoke-ArduinoCompile -SketchPath $case.SketchPath -SketchName $case.Name
    })) | Out-Null

    $results.Add((Invoke-VerifyStep -Name "Verify outputs for $($case.Name)" -Script {
        Assert-FirmwareOutputs -SketchName $case.Name -ProjectName $case.ProjectName
        Assert-DirectLinkInputs `
            -SketchName $case.Name `
            -ProjectName $case.ProjectName `
            -ExpectedLibraryObjectNames $case.ExpectedLibraryObjects `
            -ExpectNoLibraryObjects:([bool]$case.ExpectNoLibraryObjects)
        Assert-DirectLinkOrder -SketchName $case.Name -ProjectName $case.ProjectName
        Assert-BinpkgEntries -SketchName $case.Name -ProjectName $case.ProjectName
        Assert-MapEvidence `
            -SketchName $case.Name `
            -ProjectName $case.ProjectName `
            -ExpectedTexts $case.ExpectedMapText
    })) | Out-Null
}

$results.Add((Invoke-VerifyStep -Name "Verify isolated Arduino CLI data" -Script {
    Assert-IsolatedArduinoCli
})) | Out-Null

$failedCount = @($results | Where-Object { -not $_ }).Count
if ($failedCount -gt 0) {
    throw "$failedCount verification step(s) failed"
}

Write-Host "All CSDK prebuilt Arduino flow checks passed."
