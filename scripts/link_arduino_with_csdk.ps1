param(
    [Parameter(Mandatory = $true)]
    [string]$BuildPath,
    [string]$ProjectName = "air780epm_runner",
    [string]$SketchPath,
    [string]$ManifestPath,
    [switch]$RefreshPrebuild,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
. (Join-Path $PSScriptRoot "csdk_prebuild_stamp.ps1")

if ([string]::IsNullOrWhiteSpace($ManifestPath)) {
    $ManifestPath = Join-Path $repoRoot "runner\air780epm_runner\build\arduino_export_manifest.json"
}

function Get-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
}

function Read-Manifest {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        & (Join-Path $PSScriptRoot "export_arduino_build_manifest.ps1") -OutputPath $Path | Write-Output
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }

    $manifest = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    $manifestRoot = Split-Path -Parent ([System.IO.Path]::GetFullPath($Path))
    return Resolve-ManifestPaths -Manifest $manifest -BaseRoot $manifestRoot
}

function Resolve-ManifestPathValue {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory = $true)][string]$BaseRoot
    )

    if ($null -eq $Value) {
        return $null
    }
    if ($Value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($Value) -or [System.IO.Path]::IsPathRooted($Value)) {
            return $Value
        }
        return [System.IO.Path]::GetFullPath((Join-Path $BaseRoot $Value))
    }
    if ($Value -is [System.Array]) {
        return @($Value | ForEach-Object { Resolve-ManifestPathValue -Value $_ -BaseRoot $BaseRoot })
    }
    return $Value
}

function Resolve-ManifestPaths {
    param(
        [Parameter(Mandatory = $true)]$Manifest,
        [Parameter(Mandatory = $true)][string]$BaseRoot
    )

    foreach ($propertyName in @("repo_root", "runner_path", "csdk_root", "luatos_root", "include_dirs")) {
        if ($Manifest.PSObject.Properties.Name -contains $propertyName) {
            $Manifest.$propertyName = Resolve-ManifestPathValue -Value $Manifest.$propertyName -BaseRoot $BaseRoot
        }
    }
    if ($Manifest.PSObject.Properties.Name -contains "toolchain" -and $null -ne $Manifest.toolchain) {
        foreach ($property in $Manifest.toolchain.PSObject.Properties) {
            $property.Value = Resolve-ManifestPathValue -Value $property.Value -BaseRoot $BaseRoot
        }
    }
    if ($Manifest.PSObject.Properties.Name -contains "link" -and $null -ne $Manifest.link) {
        foreach ($propertyName in @("linker_script_template", "linker_script_output", "map_output", "elf_output", "link_dirs", "preprocessed_linker_script")) {
            if ($Manifest.link.PSObject.Properties.Name -contains $propertyName) {
                $Manifest.link.$propertyName = Resolve-ManifestPathValue -Value $Manifest.link.$propertyName -BaseRoot $BaseRoot
            }
        }
    }
    if ($Manifest.PSObject.Properties.Name -contains "package" -and $null -ne $Manifest.package) {
        foreach ($propertyName in @("fcelf", "section_info", "bootloader_bin", "cp_firmware_bin", "mem_map", "include_dirs", "binpkg_output", "soc_output", "pack_dir", "comdb")) {
            if ($Manifest.package.PSObject.Properties.Name -contains $propertyName) {
                $Manifest.package.$propertyName = Resolve-ManifestPathValue -Value $Manifest.package.$propertyName -BaseRoot $BaseRoot
            }
        }
    }
    return $Manifest
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

function Find-LinkLibrary {
    param(
        [Parameter(Mandatory = $true)]$Manifest,
        [Parameter(Mandatory = $true)][string]$FileName
    )

    $candidatePaths = @(
        (Join-Path $Manifest.runner_path "build\air780epm_runner\$FileName"),
        (Join-Path $Manifest.runner_path "build\csdk\$FileName")
    )
    foreach ($linkDir in @($Manifest.link.link_dirs)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$linkDir)) {
            $candidatePaths += (Join-Path ([string]$linkDir) $FileName)
        }
    }

    foreach ($candidatePath in $candidatePaths) {
        if (Test-Path -LiteralPath $candidatePath -PathType Leaf) {
            return $candidatePath
        }
    }

    return $null
}

function Test-PrebuildArtifacts {
    param([Parameter(Mandatory = $true)]$Manifest)

    $isDistributionPackage = ($Manifest.PSObject.Properties.Name -contains "distribution_package") -and [bool]$Manifest.distribution_package
    $linkerScriptInput = if ($isDistributionPackage -and ($Manifest.link.PSObject.Properties.Name -contains "preprocessed_linker_script")) {
        $Manifest.link.preprocessed_linker_script
    }
    else {
        $Manifest.link.linker_script_template
    }

    $requiredFiles = @(
        (Find-LinkLibrary -Manifest $Manifest -FileName "libair780epm_runner.a"),
        (Find-LinkLibrary -Manifest $Manifest -FileName "libcsdk.a"),
        $Manifest.package.bootloader_bin,
        $Manifest.package.cp_firmware_bin,
        $Manifest.package.fcelf,
        $Manifest.package.section_info,
        $linkerScriptInput
    )

    foreach ($requiredFile in $requiredFiles) {
        if ([string]::IsNullOrWhiteSpace([string]$requiredFile) -or -not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
            return $false
        }
    }

    return $true
}

function Copy-IfExists {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (Test-Path -LiteralPath $Source -PathType Leaf) {
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
    }
}

$buildFullPath = Get-FullPath $BuildPath
if (-not (Test-Path -LiteralPath $buildFullPath)) {
    New-Item -ItemType Directory -Force -Path $buildFullPath | Out-Null
}

$manifestFullPath = Get-FullPath $ManifestPath
$manifest = Read-Manifest -Path $manifestFullPath

$sketchObjectDir = Join-Path $buildFullPath "sketch"
$sketchObjects = @(Get-ChildItem -LiteralPath $sketchObjectDir -Filter "*.o" -File -ErrorAction SilentlyContinue)
if ($sketchObjects.Count -eq 0) {
    throw "No Arduino CLI sketch objects found in: $sketchObjectDir"
}

$coreArchive = Join-Path $buildFullPath "core\core.a"
Assert-File -Path $coreArchive -Description "Arduino CLI core archive"

$libraryDir = Join-Path $buildFullPath "libraries"
$libraryObjects = @(Get-ChildItem -LiteralPath $libraryDir -Recurse -Filter "*.o" -File -ErrorAction SilentlyContinue)
$libraryArchives = @(Get-ChildItem -LiteralPath $libraryDir -Recurse -Filter "*.a" -File -ErrorAction SilentlyContinue)

$linkInputManifest = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    project_name = $ProjectName
    build_path = $buildFullPath
    manifest_path = $manifestFullPath
    link_mode = "direct-manifest-link-package"
    sketch_objects = @($sketchObjects | ForEach-Object { $_.FullName })
    library_objects = @($libraryObjects | ForEach-Object { $_.FullName })
    library_archives = @($libraryArchives | ForEach-Object { $_.FullName })
    core_archive = $coreArchive
    expected_outputs = [ordered]@{
        elf = (Join-Path $buildFullPath "$ProjectName.elf")
        map = (Join-Path $buildFullPath "$ProjectName.map")
        binpkg = (Join-Path $buildFullPath "$ProjectName.binpkg")
        soc = (Join-Path $buildFullPath "$($ProjectName)_ec718pm.soc")
    }
}

$linkInputPath = Join-Path $buildFullPath "arduino_csdklib_link_inputs.json"
$encoding = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($linkInputPath, (($linkInputManifest | ConvertTo-Json -Depth 5) + "`n"), $encoding)
Write-Output "Arduino/CSDK link inputs: $linkInputPath"

$prebuildReady = (Test-PrebuildArtifacts -Manifest $manifest)
$isDistributionPackage = [bool]$manifest.distribution_package
$stampStatus = if ($isDistributionPackage -and $prebuildReady) {
    [PSCustomObject]@{ Valid = $true; Reason = "distribution package artifacts are present" }
}
elseif ($prebuildReady) {
    Test-CsdkPrebuildStamp -Manifest $manifest
}
else {
    [PSCustomObject]@{ Valid = $false; Reason = "required CSDK prebuild artifacts are missing" }
}
if ($Clean -or $RefreshPrebuild -or -not $prebuildReady -or -not $stampStatus.Valid) {
    if ($isDistributionPackage) {
        throw "CSDK distribution package cannot refresh prebuild artifacts: $($stampStatus.Reason)"
    }
    $reason = if ($Clean) {
        "clean requested"
    }
    elseif ($RefreshPrebuild) {
        "refresh requested"
    }
    elseif (-not $prebuildReady) {
        "required CSDK prebuild artifacts are missing"
    }
    else {
        $stampStatus.Reason
    }
    Write-Output "Refreshing CSDK prebuild artifacts: $reason"

    $buildCoreArgs = @{
        ArduinoBuildPath = $buildFullPath
        UseArduinoCliObjects = $true
        PrebuildOnly = $true
    }
    if (-not [string]::IsNullOrWhiteSpace($SketchPath)) {
        $buildCoreArgs["SketchPath"] = $SketchPath
    }
    if ($Clean) {
        $buildCoreArgs["Clean"] = $true
    }

    & (Join-Path $PSScriptRoot "build_core.ps1") @buildCoreArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    & (Join-Path $PSScriptRoot "export_arduino_build_manifest.ps1") -OutputPath $manifestFullPath | Write-Output
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    $manifest = Read-Manifest -Path $manifestFullPath
    Write-CsdkPrebuildStamp -Manifest $manifest | Write-Output
}
else {
    Write-Output "Reusing existing CSDK prebuild artifacts: $($stampStatus.Reason)."
}

$project = if ([string]::IsNullOrWhiteSpace($ProjectName)) { "air780epm_runner" } else { $ProjectName }
$directLinkDir = Join-Path $buildFullPath "direct-link"

& (Join-Path $PSScriptRoot "export_arduino_direct_link.ps1") `
    -BuildPath $buildFullPath `
    -ManifestPath $manifestFullPath `
    -ProjectName $project `
    -OutputDirectory $directLinkDir `
    -Package
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Copy-IfExists -Source (Join-Path $directLinkDir "$project.binpkg") -Destination (Join-Path $buildFullPath "$project.binpkg")
Copy-IfExists -Source (Join-Path $directLinkDir "$($project)_$($manifest.chip_target).soc") -Destination (Join-Path $buildFullPath "$($project)_$($manifest.chip_target).soc")
Copy-IfExists -Source (Join-Path $directLinkDir "$project.elf") -Destination (Join-Path $buildFullPath "$project.elf")
Copy-IfExists -Source (Join-Path $directLinkDir "$project.map") -Destination (Join-Path $buildFullPath "$project.map")

Write-Output "Arduino/CSDK firmware outputs copied to: $buildFullPath"
