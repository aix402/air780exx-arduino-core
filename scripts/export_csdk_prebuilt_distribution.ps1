param(
    [string]$ManifestPath,
    [string]$OutputDirectory = ".\dist\csdk-prebuilt-air780epm",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

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

function Assert-File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
}

function Replace-FirstLiteral {
    param(
        [Parameter(Mandatory = $true)][string]$Content,
        [Parameter(Mandatory = $true)][string]$Find,
        [Parameter(Mandatory = $true)][string]$Replace
    )

    $index = $Content.IndexOf($Find, [System.StringComparison]::Ordinal)
    if ($index -lt 0) {
        throw "Could not find linker template anchor: $Find"
    }

    return $Content.Remove($index, $Find.Length).Insert($index, $Replace)
}

function Write-ArduinoStaticConstructorLinkerTemplate {
    param(
        [Parameter(Mandatory = $true)][string]$InputPath,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    Assert-File -Path $InputPath -Description "Linker script template"

    $content = [System.IO.File]::ReadAllText((Get-FullPath $InputPath))
    if (-not $content.Contains("__arduino_init_array_start")) {
        $content = Replace-FirstLiteral `
            -Content $content `
            -Find "        *(.init*)" `
            -Replace "        *(.init)`n        *(.init.*)"

        $arduinoSection = @"
    .arduino_init_array :
    {
        . = ALIGN(4);
        __arduino_init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        __arduino_init_array_end = .;
        . = ALIGN(4);
    } > FLASH_AREA

"@

        $content = Replace-FirstLiteral `
            -Content $content `
            -Find "    .preinit_fun_array :" `
            -Replace ($arduinoSection + "    .preinit_fun_array :")
    }

    $outputFullPath = Get-FullPath $OutputPath
    $outputDirectory = Split-Path -Parent $outputFullPath
    if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory)) {
        New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    }
    [System.IO.File]::WriteAllText($outputFullPath, $content, [System.Text.UTF8Encoding]::new($false))
}

function Invoke-PreprocessFile {
    param(
        [Parameter(Mandatory = $true)][string]$Compiler,
        [Parameter(Mandatory = $true)][string]$InputPath,
        [Parameter(Mandatory = $true)][string]$OutputPath,
        [string[]]$Defines = @(),
        [string[]]$IncludeDirs = @(),
        [switch]$KeepDefines
    )

    Assert-File -Path $InputPath -Description "Preprocess input"

    $outputFullPath = Get-FullPath $OutputPath
    $outputDirectory = Split-Path -Parent $outputFullPath
    if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory)) {
        New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    }

    $preprocessArgs = [System.Collections.Generic.List[string]]::new()
    $preprocessArgs.Add("-E") | Out-Null
    $preprocessArgs.Add("-P") | Out-Null
    if ($KeepDefines) {
        $preprocessArgs.Add("-dD") | Out-Null
    }
    foreach ($define in $Defines) {
        $preprocessArgs.Add("-D$define") | Out-Null
    }
    foreach ($includeDir in $IncludeDirs) {
        if (Test-Path -LiteralPath $includeDir -PathType Container) {
            $preprocessArgs.Add("-I") | Out-Null
            $preprocessArgs.Add((Get-FullPath $includeDir)) | Out-Null
        }
    }
    $preprocessArgs.Add("-o") | Out-Null
    $preprocessArgs.Add($outputFullPath) | Out-Null
    $preprocessArgs.Add((Get-FullPath $InputPath)) | Out-Null

    & $Compiler @preprocessArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Copy-FilePreservingRepoPath {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$SourceRepoRoot
    )

    $sourceFull = Get-FullPath $Source
    Assert-File -Path $sourceFull -Description "distribution source file"
    $relative = [System.IO.Path]::GetRelativePath($SourceRepoRoot, $sourceFull)
    if ($relative.StartsWith("..")) {
        throw "Refusing to copy file outside source repo root: $sourceFull"
    }
    $destination = Join-Path $DestinationRoot $relative
    $destinationDir = Split-Path -Parent $destination
    if ($destinationDir -and -not (Test-Path -LiteralPath $destinationDir)) {
        New-Item -ItemType Directory -Force -Path $destinationDir | Out-Null
    }
    Copy-Item -LiteralPath $sourceFull -Destination $destination -Force
}

function Copy-DirectoryPreservingRepoPath {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$SourceRepoRoot
    )

    $sourceFull = Get-FullPath $Source
    if (-not (Test-Path -LiteralPath $sourceFull -PathType Container)) {
        return
    }
    $relative = [System.IO.Path]::GetRelativePath($SourceRepoRoot, $sourceFull)
    if ($relative.StartsWith("..")) {
        throw "Refusing to copy directory outside source repo root: $sourceFull"
    }
    $destination = Join-Path $DestinationRoot $relative
    $destinationParent = Split-Path -Parent $destination
    if ($destinationParent -and -not (Test-Path -LiteralPath $destinationParent)) {
        New-Item -ItemType Directory -Force -Path $destinationParent | Out-Null
    }
    Copy-Item -LiteralPath $sourceFull -Destination $destinationParent -Recurse -Force
}

function Copy-DirectoryToDestination {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $sourceFull = Get-FullPath $Source
    if (-not (Test-Path -LiteralPath $sourceFull -PathType Container)) {
        throw "Distribution source directory was not found: $sourceFull"
    }
    $destinationFull = Get-FullPath $Destination
    $destinationParent = Split-Path -Parent $destinationFull
    if ($destinationParent -and -not (Test-Path -LiteralPath $destinationParent)) {
        New-Item -ItemType Directory -Force -Path $destinationParent | Out-Null
    }
    if (Test-Path -LiteralPath $destinationFull) {
        Remove-Item -LiteralPath $destinationFull -Recurse -Force
    }
    Copy-Item -LiteralPath $sourceFull -Destination $destinationParent -Recurse -Force
    $copiedPath = Join-Path $destinationParent (Split-Path -Leaf $sourceFull)
    if ($copiedPath -ne $destinationFull) {
        Move-Item -LiteralPath $copiedPath -Destination $destinationFull -Force
    }
}

function Copy-FileToDestination {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $sourceFull = Get-FullPath $Source
    Assert-File -Path $sourceFull -Description "distribution source file"
    $destinationFull = Get-FullPath $Destination
    $destinationDirectory = Split-Path -Parent $destinationFull
    if ($destinationDirectory -and -not (Test-Path -LiteralPath $destinationDirectory)) {
        New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
    }
    Copy-Item -LiteralPath $sourceFull -Destination $destinationFull -Force
}

function Copy-HeaderDirectoryPreservingRepoPath {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$SourceRepoRoot
    )

    $sourceFull = Get-FullPath $Source
    if (-not (Test-Path -LiteralPath $sourceFull -PathType Container)) {
        return
    }
    $headerExtensions = @(".h", ".hh", ".hpp", ".hxx", ".inc", ".inl", ".def")
    Get-ChildItem -LiteralPath $sourceFull -Recurse -File | Where-Object {
        $_.Extension -in $headerExtensions
    } | ForEach-Object {
        $headerFullPath = [System.IO.Path]::GetFullPath($_.FullName)
        if (-not ($script:DistributionHeaderExcludeFiles -and $script:DistributionHeaderExcludeFiles.Contains($headerFullPath))) {
            Copy-FilePreservingRepoPath -Source $_.FullName -DestinationRoot $DestinationRoot -SourceRepoRoot $SourceRepoRoot
        }
    }
}

function Copy-RunnerRootHeaders {
    param(
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$SourceRepoRoot
    )

    Get-ChildItem -LiteralPath $RunnerPath -File | Where-Object {
        $_.Extension -in @(".h", ".hpp")
    } | ForEach-Object {
        Copy-FilePreservingRepoPath -Source $_.FullName -DestinationRoot $DestinationRoot -SourceRepoRoot $SourceRepoRoot
    }
}

function ConvertTo-DistributionValue {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$DestinationRoot
    )

    if ($null -eq $Value) {
        return $null
    }
    if ($Value -is [string]) {
        if ($Value.StartsWith($SourceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            return ($DestinationRoot + $Value.Substring($SourceRoot.Length))
        }
        return $Value
    }
    if ($Value -is [System.Array]) {
        return @($Value | ForEach-Object {
            ConvertTo-DistributionValue -Value $_ -SourceRoot $SourceRoot -DestinationRoot $DestinationRoot
        })
    }
    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        $result = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $result[$property.Name] = ConvertTo-DistributionValue `
                -Value $property.Value `
                -SourceRoot $SourceRoot `
                -DestinationRoot $DestinationRoot
        }
        return $result
    }
    return $Value
}

function ConvertTo-ManifestRelativePath {
    param(
        [AllowNull()][string]$Value,
        [Parameter(Mandatory = $true)][string]$BaseRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }
    if ([System.IO.Path]::IsPathRooted($Value) -and $Value.StartsWith($BaseRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return [System.IO.Path]::GetRelativePath($BaseRoot, $Value)
    }
    return $Value
}

function ConvertTo-ManifestRelativePathArray {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory = $true)][string]$BaseRoot
    )

    if ($null -eq $Value) {
        return @()
    }
    return @($Value | ForEach-Object { ConvertTo-ManifestRelativePath -Value ([string]$_) -BaseRoot $BaseRoot })
}

$manifestFullPath = Get-FullPath $ManifestPath
Assert-File -Path $manifestFullPath -Description "Arduino/CSDK export manifest"
$manifest = Get-Content -Raw -LiteralPath $manifestFullPath | ConvertFrom-Json

$sourceRepoRoot = Get-FullPath $manifest.repo_root
$outputFullDirectory = Get-FullPath $OutputDirectory
if ($Clean -and (Test-Path -LiteralPath $outputFullDirectory)) {
    $resolvedOutput = Resolve-Path -LiteralPath $outputFullDirectory
    if (-not $resolvedOutput.Path.StartsWith((Join-Path $repoRoot "dist"), [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean distribution directory outside repo dist: $($resolvedOutput.Path)"
    }
    Remove-Item -LiteralPath $outputFullDirectory -Recurse -Force
}
if (-not (Test-Path -LiteralPath $outputFullDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputFullDirectory | Out-Null
}

$generatedAbiDirectory = Join-Path $outputFullDirectory "abi"
$generatedLinkerTemplate = Join-Path $generatedAbiDirectory "linker\air780epm_flash.arduino_ctor.c"
$generatedLinkerScript = Join-Path $generatedAbiDirectory "linker\air780epm_flash.ld"
$generatedMemMap = Join-Path $generatedAbiDirectory "package\mem_map.txt"
$distributionLibDirectory = Join-Path $generatedAbiDirectory "lib"
$distributionPackageDirectory = Join-Path $generatedAbiDirectory "package"
$distributionPackDirectory = Join-Path $outputFullDirectory "tools\pack"

$linkerIncludeDirs = @(
    $manifest.runner_path,
    (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\pkginc"),
    (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\inc")
)

Write-ArduinoStaticConstructorLinkerTemplate `
    -InputPath $manifest.link.linker_script_template `
    -OutputPath $generatedLinkerTemplate

Invoke-PreprocessFile `
    -Compiler $manifest.toolchain.cc `
    -InputPath $generatedLinkerTemplate `
    -OutputPath $generatedLinkerScript `
    -Defines @($manifest.defines) `
    -IncludeDirs $linkerIncludeDirs
Remove-Item -LiteralPath $generatedLinkerTemplate -Force

Invoke-PreprocessFile `
    -Compiler $manifest.toolchain.cc `
    -InputPath (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\inc\mem_map.h") `
    -OutputPath $generatedMemMap `
    -Defines @($manifest.defines) `
    -IncludeDirs @($manifest.package.include_dirs) `
    -KeepDefines

$script:DistributionHeaderExcludeFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$sdkMemoryMapHeaderDirectory = Get-FullPath (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\inc")
if (Test-Path -LiteralPath $sdkMemoryMapHeaderDirectory -PathType Container) {
    Get-ChildItem -LiteralPath $sdkMemoryMapHeaderDirectory -Filter "mem_map*.h" -File | ForEach-Object {
        [void]$script:DistributionHeaderExcludeFiles.Add([System.IO.Path]::GetFullPath($_.FullName))
    }
}
$sdkNvmHeader = Get-FullPath (Join-Path $manifest.csdk_root "PLAT\prebuild\PLAT\inc\osanvm.h")
if (Test-Path -LiteralPath $sdkNvmHeader -PathType Leaf) {
    [void]$script:DistributionHeaderExcludeFiles.Add($sdkNvmHeader)
}
foreach ($sdkNetworkHeader in @(
    "PLAT\middleware\developed\cms\psdial\inc\psdial_ps_ctrl.h",
    "PLAT\middleware\developed\cms\psdial\inc\psdial.h",
    "PLAT\middleware\developed\ecapi\psapi\inc\ps_lib_api.h",
    "PLAT\middleware\thirdparty\lwip\src\include\lwip_config_cat.h",
    "PLAT\os\freertos\CMSIS\ap\inc\cmsis_os2.h",
    "PLAT\prebuild\PLAT\inc\osasys.h",
    "PLAT\prebuild\PS\inc\cmips.h",
    "PLAT\prebuild\PS\inc\networkmgr.h"
)) {
    $sdkNetworkHeaderPath = Get-FullPath (Join-Path $manifest.csdk_root $sdkNetworkHeader)
    if (Test-Path -LiteralPath $sdkNetworkHeaderPath -PathType Leaf) {
        [void]$script:DistributionHeaderExcludeFiles.Add($sdkNetworkHeaderPath)
    }
}

$includeDirectories = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($directory in @($manifest.include_dirs)) {
    if (-not [string]::IsNullOrWhiteSpace([string]$directory)) {
        [void]$includeDirectories.Add((Get-FullPath ([string]$directory)))
    }
}

foreach ($directory in $includeDirectories) {
    if ($directory -eq (Get-FullPath $manifest.runner_path)) {
        Copy-RunnerRootHeaders -RunnerPath $directory -DestinationRoot $outputFullDirectory -SourceRepoRoot $sourceRepoRoot
    }
    else {
        Copy-HeaderDirectoryPreservingRepoPath -Source $directory -DestinationRoot $outputFullDirectory -SourceRepoRoot $sourceRepoRoot
    }
}

$linkDirectories = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($directory in @($manifest.link.link_dirs)) {
    if (-not [string]::IsNullOrWhiteSpace([string]$directory)) {
        [void]$linkDirectories.Add((Get-FullPath ([string]$directory)))
    }
}

$requiredLinkLibraries = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($libraryName in @($manifest.link.link_groups.vendor_whole_group + $manifest.link.link_groups.app_whole)) {
    if (-not [string]::IsNullOrWhiteSpace([string]$libraryName)) {
        [void]$requiredLinkLibraries.Add("lib$libraryName.a")
    }
}

foreach ($directory in $linkDirectories) {
    foreach ($libraryFileName in $requiredLinkLibraries) {
        $libraryPath = Join-Path $directory $libraryFileName
        if (Test-Path -LiteralPath $libraryPath -PathType Leaf) {
            Copy-FileToDestination -Source $libraryPath -Destination (Join-Path $distributionLibDirectory $libraryFileName)
        }
    }
}

foreach ($directory in @(
    (Join-Path $manifest.csdk_root "tools\pack")
)) {
    Copy-DirectoryToDestination -Source $directory -Destination $distributionPackDirectory
}

$missingLibraries = @()
foreach ($libraryFileName in $requiredLinkLibraries) {
    $found = $false
    foreach ($directory in $linkDirectories) {
        if (Test-Path -LiteralPath (Join-Path $directory $libraryFileName) -PathType Leaf) {
            $found = $true
            break
        }
    }
    if (-not $found) {
        $missingLibraries += $libraryFileName
    }
}
if ($missingLibraries.Count -gt 0) {
    throw "Required link libraries were not found: $($missingLibraries -join ', ')"
}

$files = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($file in @(
    $manifest.package.fcelf,
    $manifest.package.section_info,
    $manifest.package.bootloader_bin,
    $manifest.package.cp_firmware_bin,
    (Join-Path $manifest.csdk_root "PLAT\tools\$($manifest.chip_target)\comdb.txt")
)) {
    if (-not [string]::IsNullOrWhiteSpace([string]$file)) {
        [void]$files.Add((Get-FullPath ([string]$file)))
    }
}

foreach ($file in $files) {
    Copy-FileToDestination -Source $file -Destination (Join-Path $distributionPackageDirectory (Split-Path -Leaf $file))
}

$toolchainBin = Get-FullPath $manifest.toolchain.bin
$toolchainRoot = Get-FullPath (Join-Path $toolchainBin "..")
$distributionToolchainRoot = Join-Path $outputFullDirectory "toolchain\gnu-rm"
Copy-DirectoryToDestination -Source $toolchainRoot -Destination $distributionToolchainRoot
$toolchainPackageManifest = Join-Path $distributionToolchainRoot "manifest.txt"
if (Test-Path -LiteralPath $toolchainPackageManifest -PathType Leaf) {
    Remove-Item -LiteralPath $toolchainPackageManifest -Force
}
$distributionToolchainBin = Join-Path $distributionToolchainRoot "bin"

$distributionManifest = ConvertTo-DistributionValue `
    -Value $manifest `
    -SourceRoot $sourceRepoRoot `
    -DestinationRoot $outputFullDirectory
$distributionManifest["toolchain"] = [ordered]@{
    bin = $distributionToolchainBin
    cc = (Join-Path $distributionToolchainBin "arm-none-eabi-gcc.exe")
    cxx = (Join-Path $distributionToolchainBin "arm-none-eabi-g++.exe")
    ar = (Join-Path $distributionToolchainBin "arm-none-eabi-gcc-ar.exe")
    objcopy = (Join-Path $distributionToolchainBin "arm-none-eabi-objcopy.exe")
    objdump = (Join-Path $distributionToolchainBin "arm-none-eabi-objdump.exe")
    size = (Join-Path $distributionToolchainBin "arm-none-eabi-size.exe")
}
$distributionManifest["distribution_package"] = $true
$distributionManifest["distribution_source_manifest"] = [System.IO.Path]::GetRelativePath($repoRoot, $manifestFullPath)
$distributionManifest["distribution_copy_policy"] = "generated-linker-mem-map-reduced-headers-required-link-libraries-and-toolchain"
$distributionManifest["repo_root"] = $outputFullDirectory
$distributionManifest["csdk_root"] = $null
$distributionManifest["link"]["link_dirs"] = @($distributionLibDirectory)
$distributionManifest["link"]["linker_script_template"] = $null
$distributionManifest["link"]["linker_script_output"] = $generatedLinkerScript
$distributionManifest["link"]["preprocessed_linker_script"] = $generatedLinkerScript
$distributionManifest["package"]["fcelf"] = (Join-Path $distributionPackageDirectory "fcelf.exe")
$distributionManifest["package"]["section_info"] = (Join-Path $distributionPackageDirectory "sectionInfo_ec718pm.json")
$distributionManifest["package"]["bootloader_bin"] = (Join-Path $distributionPackageDirectory "ap_bootloader.bin")
$distributionManifest["package"]["cp_firmware_bin"] = (Join-Path $distributionPackageDirectory "cp-demo-flash.bin")
$distributionManifest["package"]["mem_map"] = $generatedMemMap
$distributionManifest["package"]["include_dirs"] = @()
$distributionManifest["package"]["pack_dir"] = $distributionPackDirectory
$distributionManifest["package"]["comdb"] = (Join-Path $distributionPackageDirectory "comdb.txt")
foreach ($propertyName in @("repo_root", "runner_path", "luatos_root", "distribution_source_manifest")) {
    if ($distributionManifest.Contains($propertyName)) {
        $distributionManifest[$propertyName] = ConvertTo-ManifestRelativePath -Value ([string]$distributionManifest[$propertyName]) -BaseRoot $outputFullDirectory
    }
}
$distributionManifest["include_dirs"] = ConvertTo-ManifestRelativePathArray -Value $distributionManifest["include_dirs"] -BaseRoot $outputFullDirectory
foreach ($propertyName in @("bin", "cc", "cxx", "ar", "objcopy", "objdump", "size")) {
    $distributionManifest["toolchain"][$propertyName] = ConvertTo-ManifestRelativePath -Value ([string]$distributionManifest["toolchain"][$propertyName]) -BaseRoot $outputFullDirectory
}
foreach ($propertyName in @("linker_script_output", "map_output", "elf_output", "preprocessed_linker_script")) {
    $distributionManifest["link"][$propertyName] = ConvertTo-ManifestRelativePath -Value ([string]$distributionManifest["link"][$propertyName]) -BaseRoot $outputFullDirectory
}
$distributionManifest["link"]["link_dirs"] = ConvertTo-ManifestRelativePathArray -Value $distributionManifest["link"]["link_dirs"] -BaseRoot $outputFullDirectory
foreach ($propertyName in @("fcelf", "section_info", "bootloader_bin", "cp_firmware_bin", "mem_map", "binpkg_output", "soc_output", "pack_dir", "comdb")) {
    $distributionManifest["package"][$propertyName] = ConvertTo-ManifestRelativePath -Value ([string]$distributionManifest["package"][$propertyName]) -BaseRoot $outputFullDirectory
}

$distributionManifestPath = Join-Path $outputFullDirectory "arduino_export_manifest.json"
$encoding = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($distributionManifestPath, (($distributionManifest | ConvertTo-Json -Depth 8) + "`n"), $encoding)

Write-Output "CSDK prebuilt distribution: $outputFullDirectory"
Write-Output "Distribution manifest: $distributionManifestPath"
