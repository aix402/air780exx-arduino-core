param(
    [string]$OutputPath,
    [ValidateSet("ec718pm")]
    [string]$ChipTarget = "ec718pm",
    [bool]$LspdMode = $true,
    [bool]$DenoiseForce = $false,
    [bool]$ArduinoStaticConstructors = $true
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$runnerPath = Join-Path $repoRoot "runner\air780epm_runner"
$csdkRoot = Join-Path $repoRoot "external\luatos-soc-2024"
$luatosRoot = Join-Path $repoRoot "external\LuatOS"

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $runnerPath "build\arduino_export_manifest.json"
}

function Get-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return [System.IO.Path]::GetFullPath($Path)
}

function Add-ExistingDirectory {
    param(
        [System.Collections.Generic.List[string]]$List,
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $fullPath = Get-FullPath $Path
    if ((Test-Path -LiteralPath $fullPath) -and -not $List.Contains($fullPath)) {
        $List.Add($fullPath) | Out-Null
    }
}

function Get-XmakeToolchainBinDir {
    $toolchainCache = Join-Path $runnerPath ".xmake\windows\x64\cache\toolchain"
    if (-not (Test-Path -LiteralPath $toolchainCache)) {
        throw "xmake toolchain cache not found: $toolchainCache"
    }

    $content = [System.IO.File]::ReadAllText($toolchainCache)
    $match = [regex]::Match($content, 'bindir\s*=\s*\[\[(.+?\\bin)\]\]')
    if (-not $match.Success) {
        throw "Could not find GNU Arm bindir in xmake toolchain cache: $toolchainCache"
    }

    return $match.Groups[1].Value
}

if (-not (Test-Path -LiteralPath (Join-Path $runnerPath "xmake.lua"))) {
    throw "AIR780EPM runner xmake project was not found: $runnerPath"
}

Push-Location $runnerPath
try {
    xmake f --chip_target=$ChipTarget --lspd_mode=$LspdMode --denoise_force=$DenoiseForce --arduino_static_ctors=$ArduinoStaticConstructors | Write-Output
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}

$toolchainBin = Get-XmakeToolchainBinDir

$includeDirs = [System.Collections.Generic.List[string]]::new()
foreach ($path in @(
    "$luatosRoot\luat\include",
    "$luatosRoot\components\pins\include",
    "$luatosRoot\components\common",
    "$luatosRoot\components\mobile",
    "$luatosRoot\components\printf",
    "$luatosRoot\components\ethernet\common",
    "$luatosRoot\components\mbedtls",
    "$luatosRoot\components\mbedtls\include",
    "$luatosRoot\components\mbedtls\include\mbedtls",
    "$luatosRoot\components\mbedtls\include\psa",
    "$luatosRoot\components\network\adapter",
    "$luatosRoot\components\camera",
    "$luatosRoot\components\wlan",
    "$luatosRoot\components\minmea",
    "$luatosRoot\components\sms\include",
    "$luatosRoot\components\lcd",
    "$luatosRoot\components\u8g2",
    "$luatosRoot\components\multimedia",
    "$luatosRoot\components\io_queue",
    "$runnerPath\inc",
    "$runnerPath",
    "$repoRoot\core\air780epm\cores\air780epm",
    "$repoRoot\core\air780epm\variants\air780epm_dev"
)) {
    Add-ExistingDirectory -List $includeDirs -Path $path
}

$packageIncludeDirs = [System.Collections.Generic.List[string]]::new()
foreach ($path in @(
    "$csdkRoot\PLAT\device\target\board\ec7xx_0h00\common\pkginc",
    "$csdkRoot\PLAT\device\target\board\ec7xx_0h00\common\inc",
    "$runnerPath"
)) {
    Add-ExistingDirectory -List $packageIncludeDirs -Path $path
}

$defines = @(
    "CHIP_EC718",
    "TYPE_EC718M",
    "TYPE_EC718PM",
    "LWIP_NUM_SOCKETS=32",
    "OPEN_CPU_MODE",
    "FEATURE_EXCEPTION_FLASH_DUMP_ENABLE",
    "__USER_CODE__",
    "CORE_IS_AP",
    "SDK_REL_BUILD",
    "RAMCODE_COMPRESS_EN",
    "REL_COMPRESS_EN",
    "ARM_MATH_CM3",
    "FEATURE_LZMA_ENABLE",
    "WDT_FEATURE_ENABLE=1",
    "TRACE_LEVEL=5",
    "SOFTPACK_VERSION=`"`"",
    "HAVE_STRUCT_TIMESPEC",
    "FEATURE_FOTAPAR_ENABLE",
    "__CURRENT_FILE_NAME__=__FILE__",
    "ARDUINO=10819",
    "ARDUINO_ARCH_EC718PM=1",
    "ARDUINO_ARCH_AIR780EPM=1",
    "PSRAM_FEATURE_ENABLE",
    "DHCPD_ENABLE_DEFINE=1",
    "LUAT_BSP_VERSION=`"V2033`"",
    "EC_ASSERT_FLAG",
    "PM_FEATURE_ENABLE",
    "UINILOG_FEATURE_ENABLE",
    "FEATURE_OS_ENABLE",
    "FEATURE_FREERTOS_ENABLE",
    "configUSE_NEWLIB_REENTRANT=1",
    "FEATURE_YRCOMPRESS_ENABLE",
    "FEATURE_CCIO_ENABLE",
    "LWIP_CONFIG_FILE=`"lwip_config_cat.h`"",
    "LFS_NAME_MAX=63",
    "LFS_DEBUG_TRACE",
    "FEATURE_UART_HELP_DUMP_ENABLE",
    "HTTPS_WITH_CA",
    "FEATURE_HTTPC_ENABLE",
    "RTE_USB_EN=1",
    "RTE_ONE_UART_AT=0",
    "RTE_TWO_UART_AT=0",
    "LUAT_USE_NETWORK",
    "LUAT_USE_LWIP",
    "__USE_SDK_LWIP__",
    "LUAT_USE_DNS",
    "__PRINT_ALIGNED_32BIT__",
    "_REENT_SMALL",
    "_REENT_GLOBAL_ATEXIT",
    "LWIP_INCLUDED_POLARSSL_MD5=1",
    "LUAT_EC7XX_CSDK",
    "LUAT_USE_STD_STRING",
    "LUAT_LOG_NO_NEWLINE",
    "FEATURE_PS_SMS_AT_ENABLE",
    "DEBUG_LOG_HEADER_FILE=`"debug_log_ap.h`"",
    "sprintf=sprintf_",
    "snprintf=snprintf_",
    "vsnprintf=vsnprintf_",
    "LUAT_USE_FS_VFS",
    "MBEDTLS_CONFIG_FILE=`"mbedtls_ec7xx_config.h`"",
    "__USER_MAP_CONF_FILE__=`"mem_map_7xx.h`""
)

if ($ArduinoStaticConstructors) {
    $defines += "ARDUINO_ENABLE_STATIC_CONSTRUCTORS=1"
}

$commonFlags = @(
    "-g3",
    "-mcpu=cortex-m3",
    "-mthumb",
    "-nostartfiles",
    "-mapcs-frame",
    "-ffunction-sections",
    "-fdata-sections",
    "-fno-isolate-erroneous-paths-dereference",
    "-freorder-blocks-algorithm=stc",
    "-mslow-flash-data",
    "-Werror=maybe-uninitialized",
    "-Werror=unused-value",
    "-Werror=array-bounds",
    "-Werror=return-type",
    "-Werror=overflow",
    "-Werror=empty-body",
    "-Wno-unused-parameter",
    "-Wno-unused-but-set-variable",
    "-Wno-sign-compare",
    "-Wno-unused-variable",
    "-Wno-unused-function",
    "-Wno-type-limits"
)

$cOnlyFlags = @(
    "-std=gnu11",
    "-Werror=old-style-declaration",
    "-Werror=implicit-int",
    "-Wno-int-conversion",
    "-Wno-discarded-qualifiers",
    "-Wno-pointer-sign",
    "-Wno-incompatible-pointer-types",
    "-Wno-pointer-to-int-cast",
    "-Wno-int-to-pointer-cast"
)

$cppOnlyFlags = @(
    "-std=c++11",
    "-fno-exceptions",
    "-fno-rtti"
)

$linkerDefines = @(
    "FEATURE_OS_ENABLE",
    "FEATURE_FREERTOS_ENABLE",
    "__USER_MAP_CONF_FILE__=`"mem_map_7xx.h`""
)

$linkDirs = @(
    (Get-FullPath "$csdkRoot\PLAT\prebuild\PS\lib\gcc\ec718pm\oc"),
    (Get-FullPath "$csdkRoot\PLAT\prebuild\PLAT\lib\gcc\ec718pm\oc"),
    (Get-FullPath "$csdkRoot\PLAT\libs\ec718pm"),
    (Get-FullPath "$csdkRoot\lib"),
    (Get-FullPath "$csdkRoot\PLAT\device\target\board\ec7xx_0h00\ap\gcc"),
    (Get-FullPath "$runnerPath\build\csdk"),
    (Get-FullPath "$runnerPath\build\air780epm_runner")
)

$linkFlags = @(
    "-mcpu=cortex-m3",
    "-mthumb",
    "--specs=nano.specs",
    "-lm",
    "-Wl,--cref",
    "-Wl,--check-sections",
    "-Wl,--gc-sections",
    "-Wl,--no-undefined",
    "-Wl,--no-print-map-discarded",
    "-Wl,--print-memory-usage",
    "-Wl,--wrap=_malloc_r",
    "-Wl,--wrap=_free_r",
    "-Wl,--wrap=_realloc_r",
    "-Wl,--wrap=clock",
    "-Wl,--wrap=localtime",
    "-Wl,--wrap=gmtime",
    "-Wl,--wrap=time"
)

$manifest = [ordered]@{
    generated_at = (Get-Date).ToString("o")
    repo_root = $repoRoot.Path
    runner_path = (Get-FullPath $runnerPath)
    csdk_root = (Get-FullPath $csdkRoot)
    luatos_root = (Get-FullPath $luatosRoot)
    chip_target = $ChipTarget
    lspd_mode = $LspdMode
    denoise_force = $DenoiseForce
    arduino_static_constructors = $ArduinoStaticConstructors
    lib_ps_plat = "oc"
    lib_fw = "oc"
    toolchain = [ordered]@{
        bin = $toolchainBin
        cc = (Join-Path $toolchainBin "arm-none-eabi-gcc.exe")
        cxx = (Join-Path $toolchainBin "arm-none-eabi-g++.exe")
        ar = (Join-Path $toolchainBin "arm-none-eabi-gcc-ar.exe")
        objcopy = (Join-Path $toolchainBin "arm-none-eabi-objcopy.exe")
        objdump = (Join-Path $toolchainBin "arm-none-eabi-objdump.exe")
        size = (Join-Path $toolchainBin "arm-none-eabi-size.exe")
    }
    defines = $defines
    include_dirs = $includeDirs.ToArray()
    forced_includes = @("air780epm_luat_compat.h")
    common_flags = $commonFlags
    c_flags = $cOnlyFlags
    cpp_flags = $cppOnlyFlags
    asm_flags = @("-mcpu=cortex-m3", "-mthumb")
    link = [ordered]@{
        mode = "xmake-external-arduino-objects"
        linker_defines = $linkerDefines
        linker_script_template = (Get-FullPath "$csdkRoot\PLAT\core\ld\ec7xxxm_0h00_flash.c")
        linker_script_output = (Get-FullPath "$csdkRoot\PLAT\core\ld\ec7xxxm_0h00_flash.ld")
        map_output = (Get-FullPath "$runnerPath\build\air780epm_runner\air780epm_runner_debug.map")
        elf_output = (Get-FullPath "$runnerPath\build\air780epm_runner\air780epm_runner.elf")
        link_dirs = $linkDirs
        link_flags = $linkFlags
        link_groups = [ordered]@{
            vendor_whole_group = @(
                "ps", "psl1", "psif", "psnv", "tcpipmgr", "lwip", "osa", "ccio", "deltapatch2",
                "middleware_ec", "middleware_ec_private", "driver_private", "feat_USBMOD_FEAT_DEFAULT", "usb_private",
                "startup", "core_airm2m", "lzma", "fota", "csdk"
            )
            app_whole = @("air780epm_runner")
        }
        required_arduino_inputs = [ordered]@{
            sketch_objects_glob = "sketch\*.o"
            core_archive = "core\core.a"
        }
        xmake_config = [ordered]@{
            chip_target = $ChipTarget
            lspd_mode = $LspdMode
            denoise_force = $DenoiseForce
            arduino_static_ctors = $ArduinoStaticConstructors
            arduino_external_build = $true
        }
    }
    package = [ordered]@{
        fcelf = (Get-FullPath "$csdkRoot\PLAT\tools\fcelf.exe")
        section_info = (Get-FullPath "$csdkRoot\PLAT\device\target\board\ec7xx_0h00\ap\gcc\sectionInfo_ec718pm.json")
        bootloader_bin = (Get-FullPath "$runnerPath\build\ap_bootloader\ap_bootloader.bin")
        cp_firmware_bin = (Get-FullPath "$csdkRoot\PLAT\prebuild\FW\lib\gcc\ec718pm\oc\cp-demo-flash.bin")
        mem_map = (Get-FullPath "$runnerPath\out\mem_map.txt")
        include_dirs = $packageIncludeDirs.ToArray()
        binpkg_output = (Get-FullPath "$runnerPath\out\air780epm_runner.binpkg")
        soc_output = (Get-FullPath "$runnerPath\out\air780epm_runner_ec718pm.soc")
    }
}

$outputFullPath = Get-FullPath $OutputPath
$outputDirectory = Split-Path -Parent $outputFullPath
if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

$encoding = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($outputFullPath, (($manifest | ConvertTo-Json -Depth 5) + "`n"), $encoding)
Write-Output "Arduino build manifest: $outputFullPath"
