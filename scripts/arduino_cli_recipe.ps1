param(
    [Parameter(Position = 0)]
    [string]$Command,
    [string]$SourceFile,
    [string]$OutputFile,
    [string]$BuildPath,
    [string]$ProjectName,
    [string]$SketchPath,
    [string]$ExtraFlags,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RemainingArgs
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$defaultManifestPath = Join-Path $repoRoot "runner\air780epm_runner\build\arduino_export_manifest.json"
if (-not [string]::IsNullOrWhiteSpace($env:AIR780EPM_ARDUINO_MANIFEST_PATH)) {
    $defaultManifestPath = [System.IO.Path]::GetFullPath($env:AIR780EPM_ARDUINO_MANIFEST_PATH)
}

function Touch-File {
    param([Parameter(Mandatory = $true)][string]$Path)
    $dir = Split-Path -Parent $Path
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    Set-Content -Path $Path -Value "" -NoNewline
}

function Get-ArduinoBuildManifest {
    if (-not (Test-Path -LiteralPath $defaultManifestPath)) {
        & (Join-Path $PSScriptRoot "export_arduino_build_manifest.ps1") -OutputPath $defaultManifestPath | Write-Output
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }

    $manifest = Get-Content -Raw -LiteralPath $defaultManifestPath | ConvertFrom-Json
    $manifestRoot = Split-Path -Parent ([System.IO.Path]::GetFullPath($defaultManifestPath))
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
    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        foreach ($property in $Value.PSObject.Properties) {
            $property.Value = Resolve-ManifestPathValue -Value $property.Value -BaseRoot $BaseRoot
        }
    }
    return $Value
}

function Resolve-ManifestPaths {
    param(
        [Parameter(Mandatory = $true)]$Manifest,
        [Parameter(Mandatory = $true)][string]$BaseRoot
    )

    $pathProperties = @(
        "repo_root",
        "runner_path",
        "csdk_root",
        "luatos_root",
        "include_dirs"
    )
    foreach ($propertyName in $pathProperties) {
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

function Add-OutputDirectory {
    param([Parameter(Mandatory = $true)][string]$Path)
    $dir = Split-Path -Parent $Path
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
}

function Convert-DefineToArgument {
    param([Parameter(Mandatory = $true)][string]$Define)
    return ("-D" + $Define)
}

function Split-ExtraFlags {
    param([string]$Flags)

    if ([string]::IsNullOrWhiteSpace($Flags)) {
        return @()
    }
    return @($Flags -split '\s+' |
        ForEach-Object { ([string]$_).Trim('"') } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
}

function Get-ExtraCompilerArgs {
    $args = @(Get-IncludeArgsFromCommandLine)
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $deduped = @()
    foreach ($arg in $args) {
        if ($seen.Add($arg)) {
            $deduped += $arg
        }
    }
    return $deduped
}

function Get-IncludeArgsFromCommandLine {
    $raw = [Environment]::CommandLine
    if ([string]::IsNullOrWhiteSpace($raw)) {
        return @()
    }

    $matches = [regex]::Matches($raw, '-I(?:"([^"]+)"|([A-Za-z]:[\\/][^\s"]+))')
    $args = @()
    foreach ($match in $matches) {
        $path = if ($match.Groups[1].Success) { $match.Groups[1].Value } else { $match.Groups[2].Value }
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            if ((-not (Test-Path -LiteralPath $path)) -and $path.EndsWith("\")) {
                $trimmedPath = $path.TrimEnd("\")
                if (Test-Path -LiteralPath $trimmedPath) {
                    $path = $trimmedPath
                }
            }
            $args += "-I$path"
        }
    }
    return $args
}

function Get-IncludeDirsFromExtraArgs {
    $dirs = @()
    foreach ($arg in @(Get-ExtraCompilerArgs)) {
        if ($arg.StartsWith("-I") -and $arg.Length -gt 2) {
            $dirs += $arg.Substring(2)
        }
    }
    return $dirs
}

function Test-HeaderInDirs {
    param(
        [Parameter(Mandatory = $true)][string]$Header,
        [Parameter(Mandatory = $true)][string[]]$IncludeDirs
    )

    foreach ($dir in $IncludeDirs) {
        if (-not [string]::IsNullOrWhiteSpace($dir)) {
            if (Test-Path -LiteralPath (Join-Path $dir $Header) -PathType Leaf) {
                return $true
            }
        }
    }
    return $false
}

function Test-CompilerBuiltinHeader {
    param([Parameter(Mandatory = $true)][string]$Header)

    $normalized = $Header.Replace("\", "/").ToLowerInvariant()
    $leaf = [System.IO.Path]::GetFileName($normalized)
    $builtinHeaders = @(
        "algorithm",
        "array",
        "assert.h",
        "cctype",
        "cerrno",
        "cfloat",
        "cinttypes",
        "climits",
        "cmath",
        "cstddef",
        "cstdint",
        "cstdio",
        "cstdlib",
        "cstring",
        "ctime",
        "errno.h",
        "float.h",
        "initializer_list",
        "inttypes.h",
        "limits.h",
        "math.h",
        "new",
        "stdbool.h",
        "stddef.h",
        "stdint.h",
        "stdio.h",
        "stdlib.h",
        "string.h",
        "strings.h",
        "type_traits",
        "utility"
    )

    return $builtinHeaders -contains $leaf
}

function Invoke-ArduinoLibraryProbe {
    param([Parameter(Mandatory = $true)][string]$InputPath)

    $manifest = Get-ArduinoBuildManifest
    $sourceDir = Split-Path -Parent ([System.IO.Path]::GetFullPath($InputPath))
    $includeDirs = @($sourceDir) + @(Get-IncludeDirsFromExtraArgs) + @($manifest.include_dirs)
    if ($env:AIR780EPM_RECIPE_DEBUG_INCLUDES) {
        [Console]::Error.WriteLine("AIR780EPM raw command line:")
        [Console]::Error.WriteLine([Environment]::CommandLine)
        [Console]::Error.WriteLine("AIR780EPM include dirs:")
        foreach ($dir in $includeDirs) {
            [Console]::Error.WriteLine("  $dir")
        }
    }
    $sourceLines = Get-Content -LiteralPath $InputPath
    $missingHeader = $false
    for ($index = 0; $index -lt $sourceLines.Count; $index++) {
        $line = $sourceLines[$index]
        $match = [regex]::Match($line, '^\s*#\s*include\s*[<"]([^>"]+)[>"]')
        if ($match.Success) {
            $header = $match.Groups[1].Value
            if (Test-CompilerBuiltinHeader -Header $header) {
                continue
            }
            if (-not (Test-HeaderInDirs -Header $header -IncludeDirs $includeDirs)) {
                $lineNumber = $index + 1
                [Console]::Error.WriteLine("${InputPath}:${lineNumber}:10: fatal error: ${header}: No such file or directory")
                [Console]::Error.WriteLine("   ${lineNumber} | ${line}")
                [Console]::Error.WriteLine("      |          ^")
                [Console]::Error.WriteLine("compilation terminated.")
                $missingHeader = $true
            }
        }
    }
    if ($missingHeader) {
        exit 1
    }
}

function Invoke-ArduinoCompile {
    param(
        [Parameter(Mandatory = $true)][ValidateSet("c", "cpp", "asm")][string]$Language,
        [Parameter(Mandatory = $true)][string]$InputPath,
        [Parameter(Mandatory = $true)][string]$ObjectPath
    )

    $manifest = Get-ArduinoBuildManifest
    Add-OutputDirectory -Path $ObjectPath

    $compiler = if ($Language -eq "cpp") { [string]$manifest.toolchain.cxx } else { [string]$manifest.toolchain.cc }
    if (-not (Test-Path -LiteralPath $compiler)) {
        throw "Compiler was not found: $compiler"
    }

    $args = @("-c")
    if ($Language -eq "c") {
        $args += @($manifest.c_flags)
    }
    elseif ($Language -eq "cpp") {
        $args += @($manifest.cpp_flags)
    }
    else {
        $args += @($manifest.asm_flags)
    }

    if ($Language -ne "asm") {
        $args += @($manifest.common_flags)
        foreach ($forcedInclude in @($manifest.forced_includes)) {
            $args += @("-include", [string]$forcedInclude)
        }
    }

    $extraCompilerArgs = @(Get-ExtraCompilerArgs)
    $args += $extraCompilerArgs

    foreach ($includeDir in @($manifest.include_dirs)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$includeDir)) {
            $args += "-I$includeDir"
        }
    }

    foreach ($define in @($manifest.defines)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$define)) {
            $args += (Convert-DefineToArgument -Define ([string]$define))
        }
    }

    $args += @("-o", $ObjectPath, $InputPath)
    & $compiler @args
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Invoke-ArduinoArchiveObject {
    param(
        [Parameter(Mandatory = $true)][string]$ObjectPath,
        [Parameter(Mandatory = $true)][string]$ArchivePath
    )

    $manifest = Get-ArduinoBuildManifest
    Add-OutputDirectory -Path $ArchivePath
    $archiver = [string]$manifest.toolchain.ar
    if (-not (Test-Path -LiteralPath $archiver)) {
        throw "Archiver was not found: $archiver"
    }
    if (-not (Test-Path -LiteralPath $ObjectPath)) {
        throw "Object file was not found for archive: $ObjectPath"
    }

    & $archiver @("rcs", $ArchivePath, $ObjectPath)
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Invoke-ArduinoPreprocess {
    param(
        [Parameter(Mandatory = $true)][string]$InputPath,
        [string]$PreprocessedPath,
        [switch]$AllowFailure
    )

    $manifest = Get-ArduinoBuildManifest
    $compiler = [string]$manifest.toolchain.cxx
    if (-not (Test-Path -LiteralPath $compiler)) {
        throw "Compiler was not found: $compiler"
    }

    $args = @("-E", "-CC")
    $args += @($manifest.cpp_flags)
    $args += @($manifest.common_flags)
    foreach ($forcedInclude in @($manifest.forced_includes)) {
        $args += @("-include", [string]$forcedInclude)
    }
    $extraCompilerArgs = @(Get-ExtraCompilerArgs)
    $args += $extraCompilerArgs

    foreach ($includeDir in @($manifest.include_dirs)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$includeDir)) {
            $args += "-I$includeDir"
        }
    }
    foreach ($define in @($manifest.defines)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$define)) {
            $args += (Convert-DefineToArgument -Define ([string]$define))
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($PreprocessedPath)) {
        Add-OutputDirectory -Path $PreprocessedPath
        $args += @("-o", $PreprocessedPath)
    }
    $args += $InputPath

    & $compiler @args
    if ($LASTEXITCODE -ne 0 -and -not $AllowFailure) {
        exit $LASTEXITCODE
    }
}

function Invoke-ArduinoCsdkPrebuiltCombine {
    $combineArgs = @{
        BuildPath = $BuildPath
        ProjectName = $ProjectName
        SketchPath = $SketchPath
        ManifestPath = $defaultManifestPath
    }
    if ($env:AIR780EPM_REFRESH_CSDK_PREBUILD) {
        $combineArgs["RefreshPrebuild"] = $true
    }

    & (Join-Path $PSScriptRoot "link_arduino_with_csdk.ps1") @combineArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Invoke-ArduinoReportSize {
    if ([string]::IsNullOrWhiteSpace($BuildPath) -or [string]::IsNullOrWhiteSpace($ProjectName)) {
        throw "BuildPath and ProjectName are required for report-size"
    }

    $manifest = Get-ArduinoBuildManifest
    $sizeTool = [string]$manifest.toolchain.size
    if (-not (Test-Path -LiteralPath $sizeTool)) {
        throw "Size tool was not found: $sizeTool"
    }

    $elfPath = Join-Path $BuildPath "$ProjectName.elf"
    if (-not (Test-Path -LiteralPath $elfPath -PathType Leaf)) {
        throw "ELF file was not found for size report: $elfPath"
    }

    & $sizeTool "-A" $elfPath
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

switch ($Command) {
    "preprocess-stdout" {
        if ($SourceFile) {
            Get-Content -LiteralPath $SourceFile
        }
    }
    "preprocess-copy" {
        if ([string]::IsNullOrWhiteSpace($OutputFile) -or $OutputFile.Equals("nul", [System.StringComparison]::OrdinalIgnoreCase)) {
            Invoke-ArduinoLibraryProbe -InputPath $SourceFile
        }
        else {
            Copy-Item -LiteralPath $SourceFile -Destination $OutputFile -Force
        }
    }
    "touch-file" {
        Touch-File -Path $OutputFile
    }
    "compile-c" {
        Invoke-ArduinoCompile -Language "c" -InputPath $SourceFile -ObjectPath $OutputFile
    }
    "compile-cpp" {
        Invoke-ArduinoCompile -Language "cpp" -InputPath $SourceFile -ObjectPath $OutputFile
    }
    "compile-asm" {
        Invoke-ArduinoCompile -Language "asm" -InputPath $SourceFile -ObjectPath $OutputFile
    }
    "archive-object" {
        Invoke-ArduinoArchiveObject -ObjectPath $SourceFile -ArchivePath $OutputFile
    }
    "export-manifest" {
        & (Join-Path $PSScriptRoot "export_arduino_build_manifest.ps1") -OutputPath $defaultManifestPath | Write-Output
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
    "combine-csdk-prebuilt" {
        Invoke-ArduinoCsdkPrebuiltCombine
    }
    "combine-xmake-build" {
        Invoke-ArduinoCsdkPrebuiltCombine
    }
    "report-size" {
        Invoke-ArduinoReportSize
    }
    default {
        throw "Unknown Arduino CLI recipe command: $Command"
    }
}
