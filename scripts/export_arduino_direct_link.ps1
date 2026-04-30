param(
    [Parameter(Mandatory = $true)]
    [string]$BuildPath,
    [string]$ManifestPath,
    [string]$ProjectName = "air780epm_runner",
    [string]$OutputDirectory,
    [string]$SevenZipPath,
    [switch]$Execute,
    [switch]$Package
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

function Convert-ToGccPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return $Path.Replace("\", "/")
}

function Quote-ResponseArg {
    param([Parameter(Mandatory = $true)][string]$Arg)

    if ($Arg -match '[\s"]') {
        return '"' + ($Arg.Replace('\', '\\').Replace('"', '\"')) + '"'
    }
    return $Arg
}

function Add-Arg {
    param(
        [System.Collections.Generic.List[string]]$Args,
        [Parameter(Mandatory = $true)][string]$Arg
    )
    $Args.Add($Arg) | Out-Null
}

function Add-PathArg {
    param(
        [System.Collections.Generic.List[string]]$Args,
        [Parameter(Mandatory = $true)][string]$Path
    )
    Add-Arg -Args $Args -Arg (Convert-ToGccPath (Get-FullPath $Path))
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

    [System.IO.File]::WriteAllText((Get-FullPath $OutputPath), $content, [System.Text.UTF8Encoding]::new($false))
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
    $preprocessArgs.Add((Get-FullPath $OutputPath)) | Out-Null
    $preprocessArgs.Add((Get-FullPath $InputPath)) | Out-Null

    & $Compiler @preprocessArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function Resolve-SevenZip {
    param([string]$RequestedPath)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        Assert-File -Path $RequestedPath -Description "7-Zip executable"
        return (Get-FullPath $RequestedPath)
    }

    $command = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    foreach ($candidate in @(
        "C:\Program Files\7-Zip\7z.exe",
        "C:\Program Files (x86)\7-Zip\7z.exe"
    )) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }

    throw "7-Zip executable was not found. Pass -SevenZipPath to enable .soc packaging."
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

$buildFullPath = Get-FullPath $BuildPath
$manifestFullPath = Get-FullPath $ManifestPath
Assert-File -Path $manifestFullPath -Description "Arduino/CSDK export manifest"
$manifest = Get-Content -Raw -LiteralPath $manifestFullPath | ConvertFrom-Json
$isDistributionPackage = ($manifest.PSObject.Properties.Name -contains "distribution_package") -and [bool]$manifest.distribution_package

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

$runnerArchive = Find-LinkLibrary -Manifest $manifest -FileName "libair780epm_runner.a"
$csdkArchive = Find-LinkLibrary -Manifest $manifest -FileName "libcsdk.a"
Assert-File -Path $runnerArchive -Description "Runner static archive"
Assert-File -Path $csdkArchive -Description "CSDK static archive"

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $buildFullPath "direct-link"
}
$outputFullDirectory = Get-FullPath $OutputDirectory
if (-not (Test-Path -LiteralPath $outputFullDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputFullDirectory | Out-Null
}

$elfOutput = Join-Path $outputFullDirectory "$ProjectName.elf"
$mapOutput = Join-Path $outputFullDirectory "$ProjectName.map"
$linkerTemplateOutput = Join-Path $outputFullDirectory "$ProjectName.arduino_ctor.c"
$linkerScriptOutput = Join-Path $outputFullDirectory "$ProjectName.ld"
$unzipBinOutput = Join-Path $outputFullDirectory "$ProjectName.unzip.bin"
$binOutput = Join-Path $outputFullDirectory "$ProjectName.bin"
$apBinOutput = Join-Path $outputFullDirectory "ap.bin"
$memMapOutput = Join-Path $outputFullDirectory "mem_map.txt"
$sizeOutput = Join-Path $outputFullDirectory "$ProjectName.size"
$binpkgOutput = Join-Path $outputFullDirectory "$ProjectName.binpkg"
$socOutput = Join-Path $outputFullDirectory "$($ProjectName)_$($manifest.chip_target).soc"
$responsePath = Join-Path $outputFullDirectory "$ProjectName.link.rsp"
$commandManifestPath = Join-Path $outputFullDirectory "$ProjectName.direct_link.json"

$preprocessedLinkerScript = $null
if ($manifest.link.PSObject.Properties.Name -contains "preprocessed_linker_script") {
    $preprocessedLinkerScript = [string]$manifest.link.preprocessed_linker_script
}

if ($isDistributionPackage -and -not [string]::IsNullOrWhiteSpace($preprocessedLinkerScript)) {
    Assert-File -Path $preprocessedLinkerScript -Description "Preprocessed linker script"
    Copy-Item -LiteralPath (Get-FullPath $preprocessedLinkerScript) -Destination $linkerScriptOutput -Force
}
else {
    $linkerIncludeDirs = @(
        $manifest.runner_path,
        (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\pkginc"),
        (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\inc")
    )

    Write-ArduinoStaticConstructorLinkerTemplate `
        -InputPath $manifest.link.linker_script_template `
        -OutputPath $linkerTemplateOutput

    Invoke-PreprocessFile `
        -Compiler $manifest.toolchain.cc `
        -InputPath $linkerTemplateOutput `
        -OutputPath $linkerScriptOutput `
        -Defines @($manifest.defines) `
        -IncludeDirs $linkerIncludeDirs
}

$args = [System.Collections.Generic.List[string]]::new()

foreach ($flag in $manifest.link.link_flags) {
    if ($flag -eq "-lm") {
        continue
    }
    Add-Arg -Args $args -Arg $flag
}

Add-Arg -Args $args -Arg "-T$(Convert-ToGccPath (Get-FullPath $linkerScriptOutput))"
Add-Arg -Args $args -Arg "-Wl,-Map,$(Convert-ToGccPath (Get-FullPath $mapOutput))"
Add-Arg -Args $args -Arg "-o"
Add-PathArg -Args $args -Path $elfOutput

foreach ($linkDir in $manifest.link.link_dirs) {
    Add-Arg -Args $args -Arg "-L$(Convert-ToGccPath (Get-FullPath $linkDir))"
}

Add-Arg -Args $args -Arg "-Wl,--start-group"
Add-Arg -Args $args -Arg "-Wl,--whole-archive"
foreach ($libraryName in $manifest.link.link_groups.vendor_whole_group) {
    Add-Arg -Args $args -Arg "-l$libraryName"
}
Add-Arg -Args $args -Arg "-Wl,--no-whole-archive"
Add-Arg -Args $args -Arg "-Wl,--end-group"

foreach ($objectFile in $sketchObjects) {
    Add-PathArg -Args $args -Path $objectFile.FullName
}
foreach ($objectFile in $libraryObjects) {
    Add-PathArg -Args $args -Path $objectFile.FullName
}

if ($libraryArchives.Count -gt 0) {
    Add-Arg -Args $args -Arg "-Wl,--whole-archive"
    foreach ($archiveFile in $libraryArchives) {
        Add-PathArg -Args $args -Path $archiveFile.FullName
    }
    Add-Arg -Args $args -Arg "-Wl,--no-whole-archive"
}

Add-Arg -Args $args -Arg "-Wl,--whole-archive"
Add-PathArg -Args $args -Path $coreArchive
Add-PathArg -Args $args -Path $runnerArchive
Add-Arg -Args $args -Arg "-Wl,--no-whole-archive"

Add-Arg -Args $args -Arg "-lm"

$encoding = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($responsePath, (($args | ForEach-Object { Quote-ResponseArg $_ }) -join "`n") + "`n", $encoding)

$commandManifest = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    mode = if ($Execute) { "execute" } else { "dry-run" }
    project_name = $ProjectName
    build_path = $buildFullPath
    manifest_path = $manifestFullPath
    response_file = $responsePath
    command = @($manifest.toolchain.cc, "@$responsePath")
    link_order = @(
        "vendor_whole_group",
        "sketch_objects",
        "library_objects",
        "library_archives",
        "core_archive",
        "runner_archive",
        "libm"
    )
    outputs = [ordered]@{
        elf = $elfOutput
        map = $mapOutput
        bin = $binOutput
        binpkg = $binpkgOutput
        soc = $socOutput
        mem_map = $memMapOutput
        size = $sizeOutput
    }
    inputs = [ordered]@{
        sketch_objects = @($sketchObjects | ForEach-Object { $_.FullName })
        library_objects = @($libraryObjects | ForEach-Object { $_.FullName })
        library_archives = @($libraryArchives | ForEach-Object { $_.FullName })
        core_archive = (Get-FullPath $coreArchive)
        runner_archive = (Get-FullPath $runnerArchive)
        csdk_archive = (Get-FullPath $csdkArchive)
        linker_script_template = if ([string]::IsNullOrWhiteSpace([string]$manifest.link.linker_script_template)) { $null } else { (Get-FullPath $manifest.link.linker_script_template) }
        preprocessed_linker_script = if ([string]::IsNullOrWhiteSpace($preprocessedLinkerScript)) { $null } else { (Get-FullPath $preprocessedLinkerScript) }
        patched_linker_script_template = if (Test-Path -LiteralPath $linkerTemplateOutput -PathType Leaf) { (Get-FullPath $linkerTemplateOutput) } else { $null }
        linker_script = (Get-FullPath $linkerScriptOutput)
    }
}
[System.IO.File]::WriteAllText($commandManifestPath, (($commandManifest | ConvertTo-Json -Depth 6) + "`n"), $encoding)

Write-Output "Direct link response: $responsePath"
Write-Output "Direct link manifest: $commandManifestPath"

if ($Execute -or $Package) {
    & $manifest.toolchain.cc "@$responsePath"
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & $manifest.toolchain.size $elfOutput
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

if ($Package) {
    if ($isDistributionPackage -and -not [string]::IsNullOrWhiteSpace([string]$manifest.package.mem_map)) {
        Assert-File -Path $manifest.package.mem_map -Description "Preprocessed memory map"
        Copy-Item -LiteralPath (Get-FullPath $manifest.package.mem_map) -Destination $memMapOutput -Force
    }
    else {
        $packageIncludeDirs = @($manifest.include_dirs)
        if ($manifest.package.PSObject.Properties.Name -contains "include_dirs") {
            $packageIncludeDirs = @($manifest.package.include_dirs)
        }

        Invoke-PreprocessFile `
            -Compiler $manifest.toolchain.cc `
            -InputPath (Join-Path $manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\inc\mem_map.h") `
            -OutputPath $memMapOutput `
            -Defines @($manifest.defines) `
            -IncludeDirs $packageIncludeDirs `
            -KeepDefines
    }

    & $manifest.toolchain.objcopy "-O" "binary" $elfOutput $unzipBinOutput
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $sizeText = [System.Text.StringBuilder]::new()
    [void]$sizeText.Append((& $manifest.toolchain.objdump "-h" $elfOutput | Out-String))
    [void]$sizeText.Append((& $manifest.toolchain.size "-G" $elfOutput | Out-String))
    [void]$sizeText.Append((& $manifest.toolchain.size "-t" "-G" $csdkArchive | Out-String))
    [void]$sizeText.Append((& $manifest.toolchain.size "-t" "-G" $runnerArchive | Out-String))
    [System.IO.File]::WriteAllText($sizeOutput, $sizeText.ToString(), $encoding)

    & $manifest.package.fcelf `
        "-C" `
        "-bin" $unzipBinOutput `
        "-cfg" $manifest.package.section_info `
        "-map" $mapOutput `
        "-out" $binOutput
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    Copy-Item -LiteralPath $binOutput -Destination $apBinOutput -Force

    $productName = "$($manifest.chip_target.ToUpper())_PRD"
    & $manifest.package.fcelf `
        "-M" `
        "-input" $manifest.package.bootloader_bin "-addrname" "BL_PKGIMG_LNA" "-flashsize" "BOOTLOADER_PKGIMG_LIMIT_SIZE" `
        "-input" $apBinOutput "-addrname" "AP_PKGIMG_LNA" "-flashsize" "AP_PKGIMG_LIMIT_SIZE" `
        "-input" $manifest.package.cp_firmware_bin "-addrname" "CP_PKGIMG_LNA" "-flashsize" "CP_PKGIMG_LIMIT_SIZE" `
        "-pkgmode" "1" `
        "-banoldtool" "1" `
        "-productname" $productName `
        "-def" $memMapOutput `
        "-outfile" $binpkgOutput
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $packDir = Join-Path $outputFullDirectory "pack"
    $resolvedOutput = Resolve-Path -LiteralPath $outputFullDirectory
    if (Test-Path -LiteralPath $packDir) {
        $resolvedPack = Resolve-Path -LiteralPath $packDir
        if (-not $resolvedPack.Path.StartsWith($resolvedOutput.Path, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove pack directory outside direct-link output: $($resolvedPack.Path)"
        }
        Remove-Item -LiteralPath $packDir -Recurse -Force
    }
    if ($manifest.package.PSObject.Properties.Name -contains "pack_dir" -and -not [string]::IsNullOrWhiteSpace([string]$manifest.package.pack_dir)) {
        $sourcePackDir = [string]$manifest.package.pack_dir
    }
    else {
        $sourcePackDir = Join-Path $manifest.csdk_root "tools\pack"
    }
    Copy-Item -LiteralPath $sourcePackDir -Destination $packDir -Recurse -Force

    $infoPath = Join-Path $packDir "info.json"
    $info = Get-Content -Raw -LiteralPath $infoPath | ConvertFrom-Json
    $info.rom.file = [System.IO.Path]::GetFileName($binpkgOutput)
    [System.IO.File]::WriteAllText($infoPath, (($info | ConvertTo-Json -Depth 10) + "`n"), $encoding)

    if ($manifest.package.PSObject.Properties.Name -contains "comdb" -and -not [string]::IsNullOrWhiteSpace([string]$manifest.package.comdb)) {
        $comdbSource = [string]$manifest.package.comdb
    }
    else {
        $comdbSource = Join-Path $manifest.csdk_root "PLAT\tools\$($manifest.chip_target)\comdb.txt"
    }
    Assert-File -Path $comdbSource -Description "COMDB file"
    Copy-Item -LiteralPath $binpkgOutput -Destination $packDir -Force
    Copy-Item -LiteralPath $elfOutput -Destination $packDir -Force
    Copy-Item -LiteralPath $mapOutput -Destination $packDir -Force
    Copy-Item -LiteralPath $comdbSource -Destination $packDir -Force
    Copy-Item -LiteralPath $memMapOutput -Destination $packDir -Force

    if (Test-Path -LiteralPath $socOutput -PathType Leaf) {
        Remove-Item -LiteralPath $socOutput -Force
    }
    $sevenZip = Resolve-SevenZip -RequestedPath $SevenZipPath
    & $sevenZip "a" "-t7z" "-mx=9" $socOutput "$packDir\*"
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    Remove-Item -LiteralPath $packDir -Recurse -Force
    Write-Output "Direct package outputs: $binpkgOutput"
    Write-Output "Direct package outputs: $socOutput"
}
