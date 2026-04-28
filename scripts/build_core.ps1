param(
    [string]$RunnerPath = ".\runner\air780epm_runner",
    [ValidateSet("ec718pm")]
    [string]$ChipTarget = "ec718pm",
    [bool]$LspdMode = $true,
    [bool]$DenoiseForce = $false,
    [switch]$EnableStaticConstructors,
    [switch]$DisableStaticConstructors,
    [string]$SketchPath,
    [string]$ArduinoBuildPath,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$runnerFullPath = Resolve-Path (Join-Path $repoRoot $RunnerPath)
$csdkRoot = Join-Path $repoRoot "external\luatos-soc-2024"
$linkerTemplatePath = Join-Path $csdkRoot "PLAT\core\ld\ec7xxxm_0h00_flash.c"
$generatedLinkerFiles = @(
    (Join-Path $csdkRoot "PLAT\core\ld\ec7xxxm_0h00_flash.ld"),
    (Join-Path $csdkRoot "PLAT\core\ld\ec718xm\ec7xx_0h00_flash_bl.ld")
)
$linkerTemplateOriginal = $null
$staticConstructorsEnabled = $true
$generatedSketchDir = Join-Path $runnerFullPath "generated"
$runnerMemMapPath = Join-Path $runnerFullPath "mem_map_7xx.h"
$sdkMemMapPath = Join-Path $csdkRoot "PLAT\device\target\board\ec7xx_0h00\common\inc\mem_map_7xx.h"
$sdkMemMapOriginal = $null
$sdkMemMapSynced = $false

if ($EnableStaticConstructors -and $DisableStaticConstructors) {
    throw "Use either -EnableStaticConstructors or -DisableStaticConstructors, not both."
}

if ($DisableStaticConstructors) {
    $staticConstructorsEnabled = $false
}

function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Content
    )

    $encoding = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($Path, $Content, $encoding)
}

function Get-FullPathString {
    param([Parameter(Mandatory = $true)][string]$Path)
    return [System.IO.Path]::GetFullPath($Path)
}

function Get-RelativePathCompat {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $baseFull = Get-FullPathString $BasePath
    $pathFull = Get-FullPathString $Path
    if (-not $baseFull.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $baseFull += [System.IO.Path]::DirectorySeparatorChar
    }
    if (-not $pathFull.StartsWith($baseFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside base path: $pathFull"
    }
    return $pathFull.Substring($baseFull.Length)
}

function Get-RelativeIncludePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BaseDirectory,
        [Parameter(Mandatory = $true)]
        [string]$TargetPath
    )

    $baseFull = Get-FullPathString $BaseDirectory
    if (-not $baseFull.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $baseFull += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = [System.Uri]::new($baseFull)
    $targetUri = [System.Uri]::new((Get-FullPathString $TargetPath))
    return [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString()).Replace("\", "/")
}

function Clear-GeneratedSketch {
    $runnerRoot = Get-FullPathString $runnerFullPath
    $generatedRoot = Get-FullPathString $generatedSketchDir
    if (-not $generatedRoot.StartsWith($runnerRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Generated sketch directory is outside runner root: $generatedRoot"
    }
    if (Test-Path -LiteralPath $generatedRoot) {
        Remove-Item -LiteralPath $generatedRoot -Recurse -Force
    }
}

function Get-SketchDirectory {
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = Resolve-Path -LiteralPath $Path
    $item = Get-Item -LiteralPath $resolved
    if ($item.PSIsContainer) {
        return $item.FullName
    }
    return $item.Directory.FullName
}

function ConvertTo-LineDirectivePath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return $Path.Replace("\", "\\").Replace('"', '\"')
}

function Resolve-ExistingPathString {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return ""
    }

    $item = Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 -and $item.Target) {
        $target = @($item.Target) -join ""
        if (-not [string]::IsNullOrWhiteSpace($target) -and (Test-Path -LiteralPath $target)) {
            return (Resolve-Path -LiteralPath $target).Path
        }
    }

    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-ArduinoSketchbookLibrariesRoots {
    $roots = New-Object System.Collections.Generic.List[string]
    $candidates = @(
        (Join-Path $repoRoot ".arduino-cli-user\libraries"),
        (Join-Path ([Environment]::GetFolderPath("MyDocuments")) "Arduino\libraries")
    )

    foreach ($candidate in $candidates) {
        $resolved = Resolve-ExistingPathString -Path $candidate
        if ([string]::IsNullOrWhiteSpace($resolved)) {
            continue
        }

        if (-not $roots.Contains($resolved)) {
            $roots.Add($resolved) | Out-Null
        }
    }

    return $roots.ToArray()
}

function Get-ArduinoBridgeGlobalIncludeDirectories {
    param([string]$StageDirectory)

    $directories = New-Object System.Collections.Generic.List[string]
    foreach ($candidate in @(
        $StageDirectory,
        (Join-Path $repoRoot "core\air780epm\cores\air780epm"),
        (Join-Path $repoRoot "core\air780epm\variants\air780epm_dev")
    )) {
        if ([string]::IsNullOrWhiteSpace($candidate) -or -not (Test-Path -LiteralPath $candidate)) {
            continue
        }
        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        if (-not $directories.Contains($resolved)) {
            $directories.Add($resolved) | Out-Null
        }
    }

    return $directories.ToArray()
}

function Test-ArduinoSystemIncludeName {
    param([string]$IncludeName)

    if ([string]::IsNullOrWhiteSpace($IncludeName)) {
        return $true
    }

    $normalizedInclude = ($IncludeName -replace '/', '\').Trim()
    $includeLeaf = (Split-Path -Path $normalizedInclude -Leaf).ToLowerInvariant()
    $systemIncludeNames = @(
        "arduino.h",
        "binary.h",
        "hardwareserial.h",
        "pins_arduino.h",
        "print.h",
        "printable.h",
        "spi.h",
        "stream.h",
        "wire.h",
        "wprogram.h",
        "wstring.h",
        "pgmspace.h",
        "assert.h",
        "ctype.h",
        "errno.h",
        "float.h",
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
        "time.h",
        "algorithm",
        "array",
        "atomic",
        "bitset",
        "chrono",
        "cmath",
        "complex",
        "cstddef",
        "cstdint",
        "cstdio",
        "cstdlib",
        "cstring",
        "deque",
        "exception",
        "functional",
        "initializer_list",
        "ios",
        "iosfwd",
        "iterator",
        "limits",
        "list",
        "map",
        "memory",
        "optional",
        "queue",
        "set",
        "span",
        "stack",
        "stdexcept",
        "string",
        "string_view",
        "tuple",
        "type_traits",
        "unordered_map",
        "unordered_set",
        "utility",
        "variant",
        "vector"
    )

    return $systemIncludeNames -contains $includeLeaf
}

function Test-IncludeResolvedInDirectories {
    param(
        [string]$IncludeName,
        [string[]]$SearchDirectories
    )

    if ([string]::IsNullOrWhiteSpace($IncludeName)) {
        return $true
    }

    $normalizedInclude = $IncludeName -replace '/', '\'
    foreach ($directory in $SearchDirectories) {
        if ([string]::IsNullOrWhiteSpace($directory)) {
            continue
        }

        if (Test-Path -LiteralPath (Join-Path $directory $normalizedInclude)) {
            return $true
        }
    }

    return $false
}

function Get-IncludeNamesFromFiles {
    param([System.IO.FileInfo[]]$Files)

    $includeNames = New-Object System.Collections.Generic.List[string]
    foreach ($sourceFile in $Files) {
        foreach ($line in Get-Content -LiteralPath $sourceFile.FullName -ErrorAction SilentlyContinue) {
            $match = [regex]::Match($line, '^\s*#\s*include\s*[<"]([^">]+)[">]')
            if (-not $match.Success) {
                continue
            }

            $includeName = $match.Groups[1].Value.Trim()
            if ([string]::IsNullOrWhiteSpace($includeName)) {
                continue
            }

            if (-not $includeNames.Contains($includeName)) {
                $includeNames.Add($includeName) | Out-Null
            }
        }
    }

    return $includeNames.ToArray()
}

function Get-ArduinoBridgeSourceFiles {
    param([string]$StageDirectory)

    if (-not (Test-Path -LiteralPath $StageDirectory)) {
        return @()
    }

    $librariesPrefix = (Join-Path $StageDirectory "libraries").TrimEnd('\') + '\'
    return @(Get-ChildItem -LiteralPath $StageDirectory -Recurse -File | Where-Object {
        $_.Extension -in @(".ino", ".h", ".hh", ".hpp", ".c", ".cc", ".cpp", ".cxx") -and
        -not $_.FullName.StartsWith($librariesPrefix, [System.StringComparison]::OrdinalIgnoreCase)
    })
}

function Get-ArduinoLibraryPropertyValue {
    param(
        [string]$LibraryRoot,
        [string]$PropertyName
    )

    $propertiesPath = Join-Path $LibraryRoot "library.properties"
    if (-not (Test-Path -LiteralPath $propertiesPath)) {
        return ""
    }

    $pattern = '^\s*' + [regex]::Escape($PropertyName) + '\s*=\s*(.*)$'
    $line = Get-Content -LiteralPath $propertiesPath | Where-Object { $_ -match $pattern } | Select-Object -First 1
    if (-not $line) {
        return ""
    }

    return ([regex]::Match($line, $pattern)).Groups[1].Value.Trim()
}

function Get-ArduinoLibraryDisplayName {
    param([string]$LibraryRoot)

    $displayName = Get-ArduinoLibraryPropertyValue -LibraryRoot $LibraryRoot -PropertyName "name"
    if (-not [string]::IsNullOrWhiteSpace($displayName)) {
        return $displayName
    }

    return (Split-Path -Path $LibraryRoot -Leaf)
}

function Get-ArduinoLibraryPublicHeaderNames {
    param([string]$LibraryRoot)

    $includesValue = Get-ArduinoLibraryPropertyValue -LibraryRoot $LibraryRoot -PropertyName "includes"
    if ([string]::IsNullOrWhiteSpace($includesValue)) {
        return @()
    }

    $headerNames = New-Object System.Collections.Generic.List[string]
    foreach ($headerName in ($includesValue -split ',')) {
        $trimmed = Split-Path -Path (($headerName -replace '/', '\').Trim()) -Leaf
        if ([string]::IsNullOrWhiteSpace($trimmed)) {
            continue
        }
        if (-not $headerNames.Contains($trimmed)) {
            $headerNames.Add($trimmed) | Out-Null
        }
    }

    return $headerNames.ToArray()
}

function Find-ArduinoLibraryRootByHeader {
    param(
        [string[]]$LibrariesRoots,
        [string]$IncludeName
    )

    if ([string]::IsNullOrWhiteSpace($IncludeName) -or (Test-ArduinoSystemIncludeName -IncludeName $IncludeName)) {
        return ""
    }

    $normalizedInclude = $IncludeName -replace '/', '\'
    $includeLeaf = Split-Path -Path $normalizedInclude -Leaf

    foreach ($librariesRoot in $LibrariesRoots) {
        if (-not (Test-Path -LiteralPath $librariesRoot)) {
            continue
        }

        foreach ($libraryDirectory in Get-ChildItem -LiteralPath $librariesRoot -Directory) {
            foreach ($probe in @(
                (Join-Path $libraryDirectory.FullName $normalizedInclude),
                (Join-Path $libraryDirectory.FullName ("src\" + $normalizedInclude)),
                (Join-Path $libraryDirectory.FullName ("utility\" + $normalizedInclude)),
                (Join-Path $libraryDirectory.FullName ("util\" + $normalizedInclude))
            )) {
                if (Test-Path -LiteralPath $probe) {
                    return $libraryDirectory.FullName
                }
            }

            foreach ($publicHeaderName in Get-ArduinoLibraryPublicHeaderNames -LibraryRoot $libraryDirectory.FullName) {
                if ($publicHeaderName.Equals($includeLeaf, [System.StringComparison]::OrdinalIgnoreCase)) {
                    return $libraryDirectory.FullName
                }
            }
        }
    }

    return ""
}

function Get-ArduinoLibraryRootFromSketchOrigin {
    param(
        [string]$SketchSourceDirectory,
        [string[]]$LibrariesRoots
    )

    if ([string]::IsNullOrWhiteSpace($SketchSourceDirectory) -or -not (Test-Path -LiteralPath $SketchSourceDirectory)) {
        return ""
    }

    $resolvedSketchSource = (Resolve-Path -LiteralPath $SketchSourceDirectory).Path
    foreach ($librariesRoot in $LibrariesRoots) {
        if (-not (Test-Path -LiteralPath $librariesRoot)) {
            continue
        }

        foreach ($libraryDirectory in Get-ChildItem -LiteralPath $librariesRoot -Directory) {
            $prefix = $libraryDirectory.FullName.TrimEnd('\') + '\'
            if ($resolvedSketchSource.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                return $libraryDirectory.FullName
            }
        }
    }

    return ""
}

function Get-ArduinoLibrarySourceFiles {
    param([string]$LibraryRoot)

    $sourceFiles = New-Object System.Collections.Generic.List[System.IO.FileInfo]
    Get-ChildItem -LiteralPath $LibraryRoot -File -ErrorAction SilentlyContinue | Where-Object {
        $_.Extension -in @(".h", ".hh", ".hpp", ".c", ".cc", ".cpp", ".cxx", ".S", ".s")
    } | ForEach-Object {
        $sourceFiles.Add($_) | Out-Null
    }

    foreach ($subDirectoryName in @("src", "utility", "util")) {
        $subDirectory = Join-Path $LibraryRoot $subDirectoryName
        if (-not (Test-Path -LiteralPath $subDirectory)) {
            continue
        }

        Get-ChildItem -LiteralPath $subDirectory -Recurse -File | Where-Object {
            $_.Extension -in @(".h", ".hh", ".hpp", ".c", ".cc", ".cpp", ".cxx", ".S", ".s")
        } | ForEach-Object {
            $sourceFiles.Add($_) | Out-Null
        }
    }

    return $sourceFiles.ToArray()
}

function Get-ArduinoLibraryLocalIncludeDirectories {
    param([string]$LibraryRoot)

    $directories = New-Object System.Collections.Generic.List[string]
    foreach ($candidate in @(
        $LibraryRoot,
        (Join-Path $LibraryRoot "src"),
        (Join-Path $LibraryRoot "utility"),
        (Join-Path $LibraryRoot "util")
    )) {
        if (-not (Test-Path -LiteralPath $candidate)) {
            continue
        }
        $resolved = (Resolve-Path -LiteralPath $candidate).Path
        if (-not $directories.Contains($resolved)) {
            $directories.Add($resolved) | Out-Null
        }
    }

    return $directories.ToArray()
}

function Resolve-ArduinoLibraryDependencyClosure {
    param(
        [string[]]$SeedLibraryRoots,
        [string[]]$LibrariesRoots,
        [string]$StageDirectory
    )

    $resolvedLibraryRoots = New-Object System.Collections.Generic.List[string]
    $pendingLibraryRoots = New-Object System.Collections.Generic.Queue[string]

    foreach ($seedLibraryRoot in $SeedLibraryRoots) {
        if ([string]::IsNullOrWhiteSpace($seedLibraryRoot) -or -not (Test-Path -LiteralPath $seedLibraryRoot)) {
            continue
        }
        $pendingLibraryRoots.Enqueue((Resolve-Path -LiteralPath $seedLibraryRoot).Path)
    }

    while ($pendingLibraryRoots.Count -gt 0) {
        $libraryRoot = $pendingLibraryRoots.Dequeue()
        if ($resolvedLibraryRoots.Contains($libraryRoot)) {
            continue
        }

        $resolvedLibraryRoots.Add($libraryRoot) | Out-Null
        $searchDirectories = New-Object System.Collections.Generic.List[string]
        foreach ($directory in Get-ArduinoBridgeGlobalIncludeDirectories -StageDirectory $StageDirectory) {
            if (-not $searchDirectories.Contains($directory)) {
                $searchDirectories.Add($directory) | Out-Null
            }
        }
        foreach ($directory in Get-ArduinoLibraryLocalIncludeDirectories -LibraryRoot $libraryRoot) {
            if (-not $searchDirectories.Contains($directory)) {
                $searchDirectories.Add($directory) | Out-Null
            }
        }

        foreach ($includeName in Get-IncludeNamesFromFiles -Files (Get-ArduinoLibrarySourceFiles -LibraryRoot $libraryRoot)) {
            if (Test-ArduinoSystemIncludeName -IncludeName $includeName) {
                continue
            }
            if (Test-IncludeResolvedInDirectories -IncludeName $includeName -SearchDirectories $searchDirectories.ToArray()) {
                continue
            }

            $dependencyRoot = Find-ArduinoLibraryRootByHeader -LibrariesRoots $LibrariesRoots -IncludeName $includeName
            if (-not [string]::IsNullOrWhiteSpace($dependencyRoot)) {
                $resolvedDependencyRoot = (Resolve-Path -LiteralPath $dependencyRoot).Path
                if (-not $resolvedLibraryRoots.Contains($resolvedDependencyRoot)) {
                    $pendingLibraryRoots.Enqueue($resolvedDependencyRoot)
                }
            }
        }
    }

    return $resolvedLibraryRoots.ToArray()
}

function Copy-ArduinoLibraryForRunnerStage {
    param(
        [string]$LibraryRoot,
        [string]$DestinationLibrariesDirectory
    )

    $safeName = [regex]::Replace((Split-Path -Path $LibraryRoot -Leaf), '[^A-Za-z0-9_.-]+', '_')
    $destinationRoot = Join-Path $DestinationLibrariesDirectory $safeName
    New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null

    $srcDirectory = Join-Path $LibraryRoot "src"
    if (Test-Path -LiteralPath $srcDirectory) {
        Copy-Item -LiteralPath $srcDirectory -Destination $destinationRoot -Recurse -Force
    }
    else {
        Get-ChildItem -LiteralPath $LibraryRoot -File | Where-Object {
            $_.Extension -in @(".h", ".hh", ".hpp", ".c", ".cc", ".cpp", ".cxx", ".S", ".s")
        } | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $destinationRoot -Force
        }

        foreach ($supportDirectoryName in @("utility", "util")) {
            $supportDirectory = Join-Path $LibraryRoot $supportDirectoryName
            if (Test-Path -LiteralPath $supportDirectory) {
                Copy-Item -LiteralPath $supportDirectory -Destination $destinationRoot -Recurse -Force
            }
        }
    }

    return $destinationRoot
}

function Get-ArduinoLibraryStageManifestData {
    param([string]$StagedLibraryRoot)

    $includeDirectories = New-Object System.Collections.Generic.List[string]
    $sourceFiles = New-Object System.Collections.Generic.List[string]

    Get-ChildItem -LiteralPath $StagedLibraryRoot -Recurse -File | Where-Object {
        $_.Extension -in @(".h", ".hh", ".hpp")
    } | ForEach-Object {
        $directory = $_.Directory.FullName
        if (-not $includeDirectories.Contains($directory)) {
            $includeDirectories.Add($directory) | Out-Null
        }
    }

    Get-ChildItem -LiteralPath $StagedLibraryRoot -Recurse -File | Where-Object {
        $_.Extension -in @(".c", ".cc", ".cpp", ".cxx", ".S", ".s")
    } | ForEach-Object {
        $sourceFiles.Add($_.FullName) | Out-Null
    }

    return [PSCustomObject]@{
        IncludeDirectories = $includeDirectories.ToArray()
        SourceFiles = $sourceFiles.ToArray()
    }
}

function Update-ArduinoOneWireIncludeCollision {
    param([Parameter(Mandatory = $true)][string]$StageDirectory)

    $oneWireHeader = Join-Path $StageDirectory "libraries\OneWire\OneWire.h"
    if (-not (Test-Path -LiteralPath $oneWireHeader)) {
        return
    }

    $pattern = '(?m)^(\s*#\s*include\s*)[<"]OneWire\.h[>"]'
    $sourceFiles = Get-ChildItem -LiteralPath $StageDirectory -Recurse -File | Where-Object {
        $_.Extension -in @(".ino", ".h", ".hh", ".hpp", ".c", ".cc", ".cpp", ".cxx")
    }

    $changed = 0
    foreach ($file in $sourceFiles) {
        $content = [System.IO.File]::ReadAllText($file.FullName)
        if (-not [regex]::IsMatch($content, $pattern)) {
            continue
        }

        $relativeHeader = Get-RelativeIncludePath -BaseDirectory $file.Directory.FullName -TargetPath $oneWireHeader
        $replacement = '$1"' + $relativeHeader + '"'
        $updated = [regex]::Replace($content, $pattern, $replacement)
        if ($updated -ne $content) {
            Write-Utf8NoBom -Path $file.FullName -Content $updated
            $changed++
        }
    }

    if ($changed -gt 0) {
        Write-Output ("[arduino_bridge] rewrote OneWire include collision in {0} staged file(s)" -f $changed)
    }
}

function Stage-ArduinoLibraries {
    param(
        [string]$SketchDirectory,
        [string]$StageDirectory
    )

    $librariesRoots = Get-ArduinoSketchbookLibrariesRoots
    $resolvedLibraryRoots = New-Object System.Collections.Generic.List[string]
    $sketchIncludeNames = Get-IncludeNamesFromFiles -Files (Get-ArduinoBridgeSourceFiles -StageDirectory $StageDirectory)

    $originLibraryRoot = Get-ArduinoLibraryRootFromSketchOrigin -SketchSourceDirectory $SketchDirectory -LibrariesRoots $librariesRoots
    if (-not [string]::IsNullOrWhiteSpace($originLibraryRoot)) {
        $resolvedOriginLibraryRoot = (Resolve-Path -LiteralPath $originLibraryRoot).Path
        if (-not $resolvedLibraryRoots.Contains($resolvedOriginLibraryRoot)) {
            $resolvedLibraryRoots.Add($resolvedOriginLibraryRoot) | Out-Null
        }
    }

    $globalIncludeDirectories = Get-ArduinoBridgeGlobalIncludeDirectories -StageDirectory $StageDirectory
    foreach ($includeName in $sketchIncludeNames) {
        if (Test-ArduinoSystemIncludeName -IncludeName $includeName) {
            continue
        }
        if (Test-IncludeResolvedInDirectories -IncludeName $includeName -SearchDirectories $globalIncludeDirectories) {
            continue
        }

        $libraryRoot = Find-ArduinoLibraryRootByHeader -LibrariesRoots $librariesRoots -IncludeName $includeName
        if (-not [string]::IsNullOrWhiteSpace($libraryRoot)) {
            $resolvedLibraryRoot = (Resolve-Path -LiteralPath $libraryRoot).Path
            if (-not $resolvedLibraryRoots.Contains($resolvedLibraryRoot)) {
                $resolvedLibraryRoots.Add($resolvedLibraryRoot) | Out-Null
            }
        }
    }

    $resolvedLibraryRoots = Resolve-ArduinoLibraryDependencyClosure `
        -SeedLibraryRoots $resolvedLibraryRoots.ToArray() `
        -LibrariesRoots $librariesRoots `
        -StageDirectory $StageDirectory

    $stagedLibrariesDirectory = Join-Path $StageDirectory "libraries"
    $libraryReports = @()
    foreach ($libraryRoot in $resolvedLibraryRoots) {
        if ([string]::IsNullOrWhiteSpace($libraryRoot)) {
            continue
        }

        $stagedLibraryRoot = Copy-ArduinoLibraryForRunnerStage -LibraryRoot $libraryRoot -DestinationLibrariesDirectory $stagedLibrariesDirectory
        $manifestData = Get-ArduinoLibraryStageManifestData -StagedLibraryRoot $stagedLibraryRoot
        $libraryReports += [PSCustomObject]@{
            name = Get-ArduinoLibraryDisplayName -LibraryRoot $libraryRoot
            version = Get-ArduinoLibraryPropertyValue -LibraryRoot $libraryRoot -PropertyName "version"
            root = $libraryRoot
            staged_root = $stagedLibraryRoot
            include_dirs = @($manifestData.IncludeDirectories)
            source_files = @($manifestData.SourceFiles)
        }
    }

    Write-Output ("[arduino_bridge] libraries={0} direct_includes={1}" -f @($libraryReports).Count, @($sketchIncludeNames).Count)
    foreach ($libraryReport in @($libraryReports)) {
        Write-Output ("[arduino_bridge] lib {0} sources={1} includes={2}" -f $libraryReport.name, @($libraryReport.source_files).Count, @($libraryReport.include_dirs).Count)
    }

    return [PSCustomObject]@{
        search_roots = [string[]]@($librariesRoots)
        direct_includes = [string[]]@($sketchIncludeNames)
        libraries = [object[]]@($libraryReports | Where-Object { $null -ne $_ })
    }
}

function Get-PreprocessedSketch {
    param([string]$BuildPath)

    if ([string]::IsNullOrWhiteSpace($BuildPath)) {
        return $null
    }
    if (-not (Test-Path -LiteralPath $BuildPath)) {
        return $null
    }
    return Get-ChildItem -LiteralPath $BuildPath -Recurse -Filter "*.ino.cpp" |
        Sort-Object FullName |
        Select-Object -First 1
}

function Stage-ArduinoSketch {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [string]$BuildPath
    )

    Clear-GeneratedSketch
    New-Item -ItemType Directory -Force -Path $generatedSketchDir | Out-Null

    $sketchDir = Get-SketchDirectory -Path $Path
    $preprocessed = Get-PreprocessedSketch -BuildPath $BuildPath
    $generatedMain = Join-Path $generatedSketchDir "arduino_sketch.cpp"

    if ($null -ne $preprocessed) {
        Copy-Item -LiteralPath $preprocessed.FullName -Destination $generatedMain -Force
        $mainSource = $preprocessed.FullName
    }
    else {
        $inoFiles = @(Get-ChildItem -LiteralPath $sketchDir -File -Filter "*.ino" |
            Sort-Object @{Expression = { if ($_.BaseName -eq (Split-Path -Leaf $sketchDir)) { 0 } else { 1 } }}, Name)
        if ($inoFiles.Count -eq 0) {
            throw "No .ino files found in sketch directory: $sketchDir"
        }

        $builder = [System.Text.StringBuilder]::new()
        [void]$builder.AppendLine("#include <Arduino.h>")
        foreach ($ino in $inoFiles) {
            [void]$builder.AppendLine("")
            [void]$builder.AppendLine("#line 1 `"$((ConvertTo-LineDirectivePath $ino.FullName))`"")
            [void]$builder.AppendLine([System.IO.File]::ReadAllText($ino.FullName))
        }
        Write-Utf8NoBom -Path $generatedMain -Content $builder.ToString()
        $mainSource = ($inoFiles | ForEach-Object { $_.FullName }) -join ";"
    }

    $extensions = @(".h", ".hh", ".hpp", ".c", ".cc", ".cpp", ".cxx", ".s", ".S")
    $copied = New-Object System.Collections.Generic.List[string]
    foreach ($file in Get-ChildItem -LiteralPath $sketchDir -File -Recurse) {
        if ($file.Extension -notin $extensions) {
            continue
        }
        $relative = Get-RelativePathCompat -BasePath $sketchDir -Path $file.FullName
        $destination = Join-Path $generatedSketchDir $relative
        $destinationDir = Split-Path -Parent $destination
        if ($destinationDir -and -not (Test-Path -LiteralPath $destinationDir)) {
            New-Item -ItemType Directory -Force -Path $destinationDir | Out-Null
        }
        Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
        $copied.Add($relative) | Out-Null
    }

    $libraryStage = Stage-ArduinoLibraries -SketchDirectory $sketchDir -StageDirectory $generatedSketchDir
    Update-ArduinoOneWireIncludeCollision -StageDirectory $generatedSketchDir
    $stagedLibraryReports = @($libraryStage.libraries | Where-Object { $null -ne $_ })
    $manifest = [ordered]@{
        sketch_dir = $sketchDir
        generated_main = $generatedMain
        main_source = $mainSource
        copied_sources = [string[]]@($copied)
        library_search_roots = [string[]]@($libraryStage.search_roots)
        direct_includes = [string[]]@($libraryStage.direct_includes)
        libraries = [object[]]$stagedLibraryReports
    }
    $manifestJson = $manifest | ConvertTo-Json -Depth 4
    Write-Utf8NoBom -Path (Join-Path $generatedSketchDir "manifest.json") -Content ($manifestJson + "`n")
    Write-Output "Staged Arduino sketch: $sketchDir"
}

function Replace-FirstLiteral {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Content,
        [Parameter(Mandatory = $true)]
        [string]$Find,
        [Parameter(Mandatory = $true)]
        [string]$Replace
    )

    $index = $Content.IndexOf($Find, [System.StringComparison]::Ordinal)
    if ($index -lt 0) {
        throw "Could not find linker template anchor: $Find"
    }

    return $Content.Remove($index, $Find.Length).Insert($index, $Replace)
}

function Enable-ArduinoStaticConstructorLinkerPatch {
    if (-not (Test-Path -LiteralPath $linkerTemplatePath)) {
        throw "CSDK linker template was not found: $linkerTemplatePath"
    }

    $content = [System.IO.File]::ReadAllText($linkerTemplatePath)
    if ($content.Contains("__arduino_init_array_start")) {
        return $content
    }

    $patched = Replace-FirstLiteral `
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

    $patched = Replace-FirstLiteral `
        -Content $patched `
        -Find "    .preinit_fun_array :" `
        -Replace ($arduinoSection + "    .preinit_fun_array :")

    Write-Utf8NoBom -Path $linkerTemplatePath -Content $patched
    return $content
}

function Sync-RunnerMemMapOverride {
    if (-not (Test-Path -LiteralPath $runnerMemMapPath)) {
        return
    }

    if (Test-Path -LiteralPath $sdkMemMapPath) {
        $script:sdkMemMapOriginal = [System.IO.File]::ReadAllText($sdkMemMapPath)
    }

    Copy-Item -LiteralPath $runnerMemMapPath -Destination $sdkMemMapPath -Force
    $script:sdkMemMapSynced = $true
}

$buildMutex = [System.Threading.Mutex]::new($false, "Global\AIR780EXX_ArduinoCore_AIR780EPM_RunnerBuild")
$buildMutexAcquired = $false
try {
$buildMutexAcquired = $buildMutex.WaitOne([System.TimeSpan]::FromMinutes(20))
if (-not $buildMutexAcquired) {
    throw "Timed out waiting for AIR780EPM runner build lock."
}

if ([string]::IsNullOrWhiteSpace($SketchPath)) {
    Clear-GeneratedSketch
}
else {
    Stage-ArduinoSketch -Path $SketchPath -BuildPath $ArduinoBuildPath
}

Push-Location $runnerFullPath
try {
    if ($staticConstructorsEnabled) {
        $linkerTemplateOriginal = Enable-ArduinoStaticConstructorLinkerPatch
    }

    Sync-RunnerMemMapOverride

    if ($Clean) {
        xmake clean -a
    }

    xmake f --chip_target=$ChipTarget --lspd_mode=$LspdMode --denoise_force=$DenoiseForce --arduino_static_ctors=$staticConstructorsEnabled
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    xmake
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Output "Arduino static constructors: $staticConstructorsEnabled"
}
finally {
    Pop-Location
    if ($null -ne $linkerTemplateOriginal) {
        Write-Utf8NoBom -Path $linkerTemplatePath -Content $linkerTemplateOriginal
    }
    foreach ($generatedFile in $generatedLinkerFiles) {
        if (Test-Path -LiteralPath $generatedFile) {
            Remove-Item -LiteralPath $generatedFile -Force
        }
    }
    if ($sdkMemMapSynced) {
        if ($null -ne $sdkMemMapOriginal) {
            Write-Utf8NoBom -Path $sdkMemMapPath -Content $sdkMemMapOriginal
        }
        elseif (Test-Path -LiteralPath $sdkMemMapPath) {
            Remove-Item -LiteralPath $sdkMemMapPath -Force
        }
    }
}
}
finally {
    if ($buildMutexAcquired) {
        $buildMutex.ReleaseMutex()
    }
    $buildMutex.Dispose()
}
