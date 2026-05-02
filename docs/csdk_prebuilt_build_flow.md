# CSDK Prebuilt Arduino Build Flow

This document describes the experimental AIR780EPM build flow where LuatOS/CSDK is prebuilt into static libraries, while Arduino CLI still performs the standard Arduino compile steps for sketches, the Arduino core, and third-party libraries.

## Two Build Modes

There are two separate modes:

1. Maintainer prebuild/export mode.
2. Arduino user compile mode.

Maintainer mode needs the full CSDK/LuatOS source trees and xmake. Arduino user compile mode should use the exported prebuilt distribution package and should not need xmake or the full SDK source tree.

## Maintainer Prebuild Mode

Inputs:

- Full `external\luatos-soc-2024`.
- Full `external\LuatOS`.
- `runner\air780epm_runner`.
- xmake.
- GNU Arm Embedded toolchain.

The prebuild step builds the stable CSDK side:

```text
LuatOS/CSDK source + runner
        |
        | xmake / scripts\prebuild_csdk.ps1
        v
runner\air780epm_runner\build\csdk\libcsdk.a
runner\air780epm_runner\build\air780epm_runner\libair780epm_runner.a
runner\air780epm_runner\build\ap_bootloader\ap_bootloader.bin
runner\air780epm_runner\build\arduino_export_manifest.json
```

The export step then builds the distribution package:

```text
prebuilt .a + headers + vendor libs + tools + firmware + toolchain
        |
        | scripts\export_csdk_prebuilt_distribution.ps1
        v
dist\csdk-prebuilt-air780epm
```

The current distribution keeps the source-like path structure:

```text
dist\csdk-prebuilt-air780epm\
  external\luatos-soc-2024\
  external\LuatOS\
  runner\air780epm_runner\
  core\air780epm\
  abi\linker\air780epm_flash.ld
  abi\package\mem_map.txt
  toolchain\gnu-rm\
  arduino_export_manifest.json
```

The `external` directories inside `dist` are not full source trees. They are a packaged subset: required headers, vendor static libraries, package tools, firmware inputs, and metadata. The user-side distribution consumes the generated linker script and memory-map text under `abi` instead of regenerating them from the SDK linker template.
SDK memory-map headers such as `mem_map.h` and `mem_map_csdk_*.h` are excluded from the distribution header copy; the fixed package memory map is `abi\package\mem_map.txt`.
The SDK NVM header `osanvm.h` is also excluded; Arduino `EEPROM` and `Preferences` call the runner-side `arduino_nvm_io` ABI instead.
CSDK network-private headers used by TCP/UDP/TLS/modem internals are excluded as well; Arduino-facing network classes compile against `arduino_tcp_io`, `arduino_udp_io`, `arduino_tls_io`, and `arduino_modem_io`.

## Distribution Package Contents

The package currently includes:

- Required CSDK/LuatOS/runner/core/variant headers.
- `libcsdk.a`.
- `libair780epm_runner.a`.
- Vendor/prebuilt static libraries used by the final link.
- `ap_bootloader.bin`.
- `cp-demo-flash.bin`.
- `fcelf.exe`.
- `sectionInfo_ec718pm.json`.
- Preprocessed linker script `abi\linker\air780epm_flash.ld`.
- Preprocessed package memory map `abi\package\mem_map.txt`.
- `tools\pack`.
- `comdb.txt`.
- GNU Arm Embedded toolchain under `toolchain\gnu-rm`.
- `arduino_export_manifest.json`.

The manifest is the contract consumed by the Arduino recipes. It records compiler paths, flags, include directories, link directories, link groups, package inputs, and output rules.

## Boards Manager Package Shape

The release candidate is split into three Arduino package-index artifacts:

- `air780:air780`: the Arduino platform archive.
- `air780:air780epm-csdk`: the CSDK/runner ABI tool archive.
- `air780:gnu-rm`: the GNU Arm Embedded toolchain archive.
- `air780:luatos-cli`: the Windows flashing/log tool archive.

The platform archive contains:

- `platform.txt` and `boards.txt`.
- `cores\air780epm`.
- `variants\air780epm_dev`.
- platform-local recipe/upload/helper scripts under `tools`.
- bundled platform examples under `examples`.
- bundled AIR780EPM helper libraries under `libraries`.

The tool archives must use a single short top-level directory in the zip. Arduino CLI strips that directory while installing the tool. The CSDK tool archive currently uses `air780epm-csdk`; the GNU Arm tool archive uses `gnu-rm`; the flash tool archive uses `luatos-cli`.

The package-index draft is generated with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\generate_package_index_draft.ps1 `
  -BaseUrl https://example.com/air780/arduino/releases
```

Before publishing, replace the draft base URL with the real release URL and upload all archives listed by the generated index.

## Arduino CLI Compile Mode

Arduino CLI reads `core\air780epm\platform.txt`.

The Windows recipes call:

```text
scripts\arduino_cli_recipe.ps1
```

The compile recipes use the manifest-selected GNU Arm tools. When testing the distribution package, set:

```powershell
$env:AIR780EPM_ARDUINO_MANIFEST_PATH = (Resolve-Path .\dist\csdk-prebuilt-air780epm\arduino_export_manifest.json).Path
```

Without this environment variable, the recipes use the worktree manifest:

```text
runner\air780epm_runner\build\arduino_export_manifest.json
```

## Sketch Compile

Arduino CLI preprocesses `.ino` files into `.ino.cpp`, then invokes the C++ compile recipe:

```text
sketch.ino
    |
    | Arduino CLI preprocessing
    v
sketch.ino.cpp
    |
    | arm-none-eabi-g++
    v
.arduino-cli-work\<Sketch>\sketch\sketch.ino.cpp.o
```

## Arduino Core Compile

Arduino CLI compiles the Arduino core sources:

```text
core\air780epm\cores\air780epm\*.cpp
    |
    | arm-none-eabi-g++
    v
.arduino-cli-work\<Sketch>\core\*.o
    |
    | arm-none-eabi-gcc-ar
    v
.arduino-cli-work\<Sketch>\core\core.a
```

The Arduino core remains source-compiled by Arduino CLI. This is why the distribution still needs the headers required by the core.

## Third-Party Library Compile

When a sketch includes a library, Arduino CLI performs normal library discovery and compilation:

```text
libraries\<Library>\src\*.c / *.cpp
    |
    | arm-none-eabi-gcc / arm-none-eabi-g++
    v
.arduino-cli-work\<Sketch>\libraries\**\*.o
```

This has been verified with:

- `ComplexLibraryProbe`, covering mixed C/C++ and nested `src\detail` sources.
- `ArduinoJsonProbe`, covering a real header-only Arduino library.

## Combine Stage

Arduino CLI then runs:

```text
recipe.c.combine.pattern.windows
```

The recipe calls:

```text
arduino_cli_recipe.ps1 combine-csdk-prebuilt
```

That invokes:

```text
scripts\link_arduino_with_csdk.ps1
```

The combine step collects Arduino CLI outputs:

```text
sketch\*.o
core\core.a
libraries\**\*.o
libraries\**\*.a, if any
```

It then reads the manifest to find the CSDK-side inputs:

```text
libcsdk.a
libair780epm_runner.a
vendor *.a
preprocessed linker script
preprocessed mem_map.txt
fcelf.exe
ap_bootloader.bin
cp-demo-flash.bin
tools\pack
```

If the manifest has `distribution_package=true`, the combine step only reuses packaged artifacts. It refuses to refresh CSDK prebuild artifacts through xmake.

## Direct Link Stage

The combine step calls:

```text
scripts\export_arduino_direct_link.ps1 -Package
```

This script:

1. Uses the manifest linker script.
2. In source-worktree mode, patches and preprocesses the SDK linker template.
3. In distribution mode, copies the exported `abi\linker\air780epm_flash.ld`.
4. Builds a GCC response file.
5. Calls `arm-none-eabi-g++` for the final ELF link.

Distribution mode does not require `ec7xxxm_0h00_flash.c` for user-side linking. If the SDK linker template or memory-map input changes, the maintainer must rerun prebuild/export and publish a new distribution package.

The important link order is:

```text
vendor whole-group libraries, including libcsdk.a
sketch objects
third-party library objects
third-party library archives
Arduino core archive
libair780epm_runner.a
libm
```

The output is:

```text
.arduino-cli-work\<Sketch>\direct-link\<Project>.elf
.arduino-cli-work\<Sketch>\direct-link\<Project>.map
```

The verifier asserts this order because the CSDK/vendor archive ordering is sensitive.

## Package Stage

After linking, the direct package flow produces flashable firmware:

```text
<Project>.elf
    |
    | arm-none-eabi-objcopy -O binary
    v
<Project>.unzip.bin
    |
    | fcelf -C
    v
<Project>.bin
```

The AP binary is copied to `ap.bin` before packaging. This is required because the flash package AP entry must be named `ap`.

Then:

```text
ap_bootloader.bin
ap.bin
cp-demo-flash.bin
mem_map.txt
    |
    | fcelf -M
    v
<Project>.binpkg
```

Finally, `tools\pack` and 7-Zip build the `.soc` archive:

```text
binpkg + elf + map + comdb + mem_map
    |
    | 7z
    v
<Project>_ec718pm.soc
```

The final outputs are copied back to the Arduino build root:

```text
.arduino-cli-work\<Sketch>\<Project>.elf
.arduino-cli-work\<Sketch>\<Project>.map
.arduino-cli-work\<Sketch>\<Project>.binpkg
.arduino-cli-work\<Sketch>\<Project>_ec718pm.soc
```

## Flash Stage

Flash uses:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\<Sketch>\<Project>.binpkg `
  -SocFile .\.arduino-cli-work\<Sketch>\<Project>_ec718pm.soc
```

In Boards Manager installs, the upload recipe passes the package-index installed
`air780:luatos-cli` tool directory to `upload_core.ps1`. For development
worktrees, `upload_core.ps1` still falls back to `tools\luatos-cli-release`
when that installed tool path is not provided.

For normal uploads, pass the currently selected Arduino serial port, for
example `COM3`. If the board is not running normally and no command/log port is
available, manually enter Boot/download mode and pass `auto` as the port:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort auto `
  -SocFile .\.arduino-cli-work\<Sketch>\<Project>_ec718pm.soc
```

This maps directly to `luatos-cli flash run --port auto`, which detects the
EC718 download-mode port such as `COM7`.

Normal AIR780EPM flashing sequence:

```text
COM3 running command/log port
    |
    | reboot to download mode
    v
COM20 download port
    |
    | flash ap_bootloader / ap / cp-demo-flash
    v
reset
    |
    v
COM3 / COM4 / COM5 return
```

## Verified Acceptance Commands

Default phase-1 acceptance:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1
```

Distribution acceptance:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 -IncludeDistribution
```

Package the verified distribution for release:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\package_csdk_prebuilt_distribution.ps1 -Clean
```

The package script writes three release artifacts under `dist\releases`:

- `csdk-prebuilt-air780epm-<version>.zip`
- `csdk-prebuilt-air780epm-<version>.zip.sha256`
- `csdk-prebuilt-air780epm-<version>.manifest.json`

By default `<version>` is derived from the current Git branch and short commit.
Pass `-Version <name>` when cutting a named release.

Package-index install acceptance:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_package_index_install.ps1 -Clean
```

This starts a local HTTP server for `dist\releases`, generates a package-index draft, installs `air780:air780` into an isolated Arduino15-like data directory under `%LOCALAPPDATA%\Arduino15-air780-smoke`, and compiles both `Blink` and `ComplexLibraryProbe`.

The verification intentionally uses a path shaped like a normal Windows Arduino15 install. A much deeper worktree-local data path can make GNU Arm Embedded 10.2.1 fail to find its C++ multilib header `bits\c++config.h`; the Arduino15-like smoke path has been verified to avoid that failure.

Distribution hardware acceptance:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 `
  -IncludeDistribution `
  -FlashDistribution `
  -FlashComPort COM3
```

## Current Verification Status

Verified in this experiment:

- Blink software build through Arduino CLI and direct CSDK link.
- Blink hardware flash and runtime log.
- `ComplexLibraryProbe` software build with third-party C/C++ library objects.
- `ComplexLibraryProbe` hardware flash and runtime log.
- `ArduinoJsonProbe` software build with a real header-only Arduino library.
- Distribution package build with bundled toolchain.
- Distribution-built `ComplexLibraryProbe` hardware flash and runtime log.
- Boards Manager-style package-index install into an isolated Arduino15-like directory.
- Package-index-installed `Blink` and `ComplexLibraryProbe` compile/package outputs.

## Key Constraints

- The AP package entry must be named `ap`.
- The final link order must preserve the verified vendor/core/runner ordering.
- The linker script must include `.arduino_init_array` and its start/end symbols.
- CSDK prebuild refresh still requires the full CSDK/LuatOS source trees and xmake.
- Arduino compile/link/package from an already exported distribution package should not require xmake or full SDK sources.
