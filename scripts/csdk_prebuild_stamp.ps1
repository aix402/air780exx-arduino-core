$ErrorActionPreference = "Stop"

function Get-CsdkPrebuildStampPath {
    param([Parameter(Mandatory = $true)]$Manifest)
    return (Join-Path $Manifest.runner_path "build\arduino_prebuild_stamp.json")
}

function Get-RelativePathForStamp {
    param(
        [Parameter(Mandatory = $true)][string]$BasePath,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $baseFull = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $baseFull.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $baseFull += [System.IO.Path]::DirectorySeparatorChar
    }
    $pathFull = [System.IO.Path]::GetFullPath($Path)
    if ($pathFull.StartsWith($baseFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $pathFull.Substring($baseFull.Length).Replace("\", "/")
    }
    return $pathFull.Replace("\", "/")
}

function Get-StampFileHash {
    param([Parameter(Mandatory = $true)][string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-CsdkPrebuildInputFiles {
    param([Parameter(Mandatory = $true)]$Manifest)

    $files = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

    $explicitFiles = @(
        (Join-Path $Manifest.runner_path "xmake.lua"),
        (Join-Path $Manifest.runner_path "mem_map_7xx.h"),
        (Join-Path $Manifest.csdk_root "csdk.lua"),
        (Join-Path $Manifest.csdk_root "project\project.lua"),
        (Join-Path $Manifest.csdk_root "bootloader\bootloader.lua"),
        (Join-Path $Manifest.csdk_root "PLAT\core\ld\ec7xxxm_0h00_flash.c"),
        (Join-Path $Manifest.csdk_root "PLAT\device\target\board\ec7xx_0h00\common\inc\mem_map.h"),
        (Join-Path $Manifest.repo_root "scripts\build_core.ps1"),
        (Join-Path $Manifest.repo_root "scripts\export_arduino_build_manifest.ps1")
    )

    foreach ($file in $explicitFiles) {
        if (Test-Path -LiteralPath $file -PathType Leaf) {
            [void]$files.Add(([System.IO.Path]::GetFullPath($file)))
        }
    }

    $directories = @(
        (Join-Path $Manifest.runner_path "src"),
        (Join-Path $Manifest.runner_path "inc"),
        (Join-Path $Manifest.csdk_root "interface\src"),
        (Join-Path $Manifest.csdk_root "interface\include"),
        (Join-Path $Manifest.luatos_root "luat\include"),
        (Join-Path $Manifest.luatos_root "components\common"),
        (Join-Path $Manifest.luatos_root "components\mobile"),
        (Join-Path $Manifest.luatos_root "components\printf")
    )

    foreach ($directory in $directories) {
        if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
            continue
        }
        Get-ChildItem -LiteralPath $directory -Recurse -File | Where-Object {
            $_.Extension -in @(".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".S", ".s", ".lua")
        } | ForEach-Object {
            [void]$files.Add($_.FullName)
        }
    }

    return @($files)
}

function Get-CsdkPrebuildArtifactPaths {
    param([Parameter(Mandatory = $true)]$Manifest)

    return @(
        (Join-Path $Manifest.runner_path "build\air780epm_runner\libair780epm_runner.a"),
        (Join-Path $Manifest.runner_path "build\csdk\libcsdk.a"),
        $Manifest.package.bootloader_bin
    )
}

function New-CsdkPrebuildFingerprint {
    param([Parameter(Mandatory = $true)]$Manifest)

    $inputFiles = @(Get-CsdkPrebuildInputFiles -Manifest $Manifest)
    $fileEntries = @($inputFiles | ForEach-Object {
        [ordered]@{
            path = (Get-RelativePathForStamp -BasePath $Manifest.repo_root -Path $_)
            sha256 = (Get-StampFileHash -Path $_)
        }
    })

    $payload = [ordered]@{
        schema = 1
        chip_target = $Manifest.chip_target
        lspd_mode = [bool]$Manifest.lspd_mode
        denoise_force = [bool]$Manifest.denoise_force
        arduino_static_constructors = [bool]$Manifest.arduino_static_constructors
        files = $fileEntries
    }
    $json = $payload | ConvertTo-Json -Depth 6 -Compress
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $hash = $sha.ComputeHash($bytes)
    }
    finally {
        $sha.Dispose()
    }

    return [PSCustomObject]@{
        hash = ([System.BitConverter]::ToString($hash).Replace("-", "").ToLowerInvariant())
        file_count = $fileEntries.Count
        files = $fileEntries
    }
}

function Get-CsdkPrebuildArtifactHashes {
    param([Parameter(Mandatory = $true)]$Manifest)

    return @(Get-CsdkPrebuildArtifactPaths -Manifest $Manifest | ForEach-Object {
        if (-not (Test-Path -LiteralPath $_ -PathType Leaf)) {
            return
        }
        [ordered]@{
            path = (Get-RelativePathForStamp -BasePath $Manifest.repo_root -Path $_)
            sha256 = (Get-StampFileHash -Path $_)
        }
    })
}

function Write-CsdkPrebuildStamp {
    param([Parameter(Mandatory = $true)]$Manifest)

    $stampPath = Get-CsdkPrebuildStampPath -Manifest $Manifest
    $stampDir = Split-Path -Parent $stampPath
    if ($stampDir -and -not (Test-Path -LiteralPath $stampDir)) {
        New-Item -ItemType Directory -Force -Path $stampDir | Out-Null
    }

    $fingerprint = New-CsdkPrebuildFingerprint -Manifest $Manifest
    $stamp = [ordered]@{
        schema = 1
        generated_at = (Get-Date).ToString("o")
        input_fingerprint = $fingerprint.hash
        input_file_count = $fingerprint.file_count
        config = [ordered]@{
            chip_target = $Manifest.chip_target
            lspd_mode = [bool]$Manifest.lspd_mode
            denoise_force = [bool]$Manifest.denoise_force
            arduino_static_constructors = [bool]$Manifest.arduino_static_constructors
        }
        artifacts = @(Get-CsdkPrebuildArtifactHashes -Manifest $Manifest)
    }

    $encoding = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($stampPath, (($stamp | ConvertTo-Json -Depth 6) + "`n"), $encoding)
    Write-Output "CSDK prebuild stamp: $stampPath"
}

function Test-CsdkPrebuildStamp {
    param([Parameter(Mandatory = $true)]$Manifest)

    $stampPath = Get-CsdkPrebuildStampPath -Manifest $Manifest
    if (-not (Test-Path -LiteralPath $stampPath -PathType Leaf)) {
        return [PSCustomObject]@{ Valid = $false; Reason = "prebuild stamp is missing" }
    }

    $stamp = Get-Content -Raw -LiteralPath $stampPath | ConvertFrom-Json
    if ([int]$stamp.schema -ne 1) {
        return [PSCustomObject]@{ Valid = $false; Reason = "prebuild stamp schema is unsupported" }
    }

    $fingerprint = New-CsdkPrebuildFingerprint -Manifest $Manifest
    if ([string]$stamp.input_fingerprint -ne [string]$fingerprint.hash) {
        return [PSCustomObject]@{ Valid = $false; Reason = "prebuild input fingerprint changed" }
    }

    $currentArtifacts = @(Get-CsdkPrebuildArtifactHashes -Manifest $Manifest)
    $expectedArtifacts = @($stamp.artifacts)
    if ($currentArtifacts.Count -ne $expectedArtifacts.Count) {
        return [PSCustomObject]@{ Valid = $false; Reason = "prebuild artifact set changed" }
    }

    foreach ($expected in $expectedArtifacts) {
        $current = @($currentArtifacts | Where-Object { $_.path -eq $expected.path }) | Select-Object -First 1
        if ($null -eq $current) {
            return [PSCustomObject]@{ Valid = $false; Reason = "prebuild artifact is missing: $($expected.path)" }
        }
        if ([string]$current.sha256 -ne [string]$expected.sha256) {
            return [PSCustomObject]@{ Valid = $false; Reason = "prebuild artifact hash changed: $($expected.path)" }
        }
    }

    return [PSCustomObject]@{ Valid = $true; Reason = "prebuild stamp matches" }
}
