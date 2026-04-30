# CSDK Prebuilt Static Library Research

Date: 2026-04-28
Branch: `codex/csdk-prebuilt-a-experiment`
Workspace: `F:\AIR780EXX_ArduinoCore_worktrees\csdk-prebuilt-a`

## Goal

Research whether the AIR780EPM build can prebuild the LuatOS/CSDK side into static libraries, then let Arduino CLI perform the standard Arduino compile flow for sketch, Arduino core, and third-party libraries, and finally link those Arduino objects/libraries with the CSDK artifacts to produce flashable `.binpkg` and `.soc` firmware.

This is feasibility research only. It must not change the mainline workspace `F:\AIR780EXX_ArduinoCore`, and phase 1 should validate only the Blink minimum path.

## Current Baseline

The current Arduino CLI platform is a bridge, not a real Arduino compiler integration:

- `core/air780epm/platform.txt` uses placeholder compile/archive recipes.
- `recipe.c.o.pattern.windows`, `recipe.cpp.o.pattern.windows`, `recipe.S.o.pattern.windows`, and `recipe.ar.pattern.windows` call `arduino_cli_recipe.ps1 touch-file`.
- `recipe.c.combine.pattern.windows` calls `arduino_cli_recipe.ps1 combine-xmake-build`.
- `scripts/arduino_cli_recipe.ps1` delegates the actual build to `scripts/build_core.ps1`.
- `scripts/build_core.ps1` stages the preprocessed sketch and discovered third-party libraries into `runner/air780epm_runner/generated`, then xmake compiles sketch, runner, Arduino core, LuatOS/CSDK, bootloader, links ELF, and packages output.

The runner target already produces the useful split artifacts:

- `runner/air780epm_runner/build/csdk/libcsdk.a`
- `runner/air780epm_runner/build/air780epm_runner/libair780epm_runner.a`
- `runner/air780epm_runner/build/bootloader_libdriver/libdriver.a`
- `runner/air780epm_runner/build/ap_bootloader/ap_bootloader.bin`
- `runner/air780epm_runner/out/air780epm_runner.binpkg`
- `runner/air780epm_runner/out/air780epm_runner_ec718pm.soc`

Blink baseline command verified in this worktree:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\01.Basics\Blink -Clean
```

Result: build succeeded. Output included `air780epm_runner.binpkg` and `air780epm_runner_ec718pm.soc`.

## CSDK/Xmake Findings

Primary files read:

- `runner/air780epm_runner/xmake.lua`
- `external/luatos-soc-2024/csdk.lua`
- `external/luatos-soc-2024/project/project.lua`
- `external/luatos-soc-2024/bootloader/bootloader.lua`
- `scripts/build_core.ps1`
- `core/air780epm/platform.txt`

Important xmake structure:

- `description_common()` configures the GNU Arm Embedded toolchain, target chip macros, common C/C++/ASM/link flags, and common include paths.
- `description_csdk()` adds CSDK/LuatOS-specific macros, warning flags, wrap linker flags, and LuatOS include paths.
- `target("csdk")` is already `set_kind("static")` and emits `libcsdk.a`.
- `target(project_name)` in `runner/air780epm_runner/xmake.lua` is also `set_kind("static")` and currently includes runner sources, Arduino runtime sources, staged sketch sources, staged library sources, and `core/air780epm/cores/air780epm/*.cpp`.
- `target(project_name .. ".elf")` in `project/project.lua` performs final AP ELF link and depends on `ap_bootloader.elf`, `csdk`, and `project_name`.
- `after_build` in `project/project.lua` produces `.bin`, `.binpkg`, `.soc`, `.map`, `mem_map.txt`, and `comdb.txt`.

For AIR780EPM with the current command:

```powershell
xmake f --chip_target=ec718pm --lspd_mode=true --denoise_force=false --arduino_static_ctors=true
```

xmake resolved:

- `chip_target=ec718pm`
- `lib_ps_plat=oc`
- `lib_fw=oc`
- linker script: `external/luatos-soc-2024/PLAT/core/ld/ec7xxxm_0h00_flash.ld`
- bootloader linker script: `external/luatos-soc-2024/PLAT/core/ld/ec718xm/ec7xx_0h00_flash_bl.ld`
- toolchain: `arm-none-eabi-gcc/g++` from xmake package `gnu_rm ec7xx`, GCC Arm 10-2020-q4

## Parameters That Must Be Exported From Xmake

Arduino CLI cannot safely guess these. A prebuild/export step should emit a machine-readable manifest, for example JSON, consumed by the Arduino recipes.

Toolchain:

- Absolute paths for `arm-none-eabi-gcc`, `arm-none-eabi-g++`, `arm-none-eabi-ar`, `arm-none-eabi-objcopy`, `arm-none-eabi-objdump`, `arm-none-eabi-size`.
- Toolchain version/package identity for reproducibility.

Compile flags required by Arduino CLI:

- CPU/ABI flags: `-mcpu=cortex-m3`, `-mthumb`, `-mapcs-frame`.
- Section flags: `-ffunction-sections`, `-fdata-sections`.
- Runtime/startup flags: `-nostartfiles`.
- C/C++ language levels should match xmake: GNU C11 and C++11.
- Forced include from runner: `-include air780epm_luat_compat.h`.
- AIR780EPM runner defines: `ARDUINO=10819`, `ARDUINO_ARCH_EC718PM=1`, `ARDUINO_ARCH_AIR780EPM=1`.
- Static constructor define when enabled: `ARDUINO_ENABLE_STATIC_CONSTRUCTORS=1`.
- Chip/config defines resolved by xmake: at least `CHIP_EC718`, `TYPE_EC718M`, `TYPE_EC718PM`, `LWIP_NUM_SOCKETS=32`, `OPEN_CPU_MODE`, `PSRAM_FEATURE_ENABLE`, and common CSDK defines needed by Arduino headers that include LuatOS/CSDK headers.
- Include dirs for Arduino-facing compilation: runner `inc`, Arduino core, variant, CSDK common include dirs, LuatOS component dirs used by public Arduino headers, and any generated/staged third-party library include dirs.

Link inputs required by Arduino CLI/final linker:

- Arduino CLI-produced sketch object/archive.
- Arduino CLI-produced core archive.
- Arduino CLI-produced third-party library archives.
- Prebuilt CSDK archive: `build/csdk/libcsdk.a`.
- Runner/static glue archive after removing Arduino-owned sources from xmake: `build/air780epm_runner/libair780epm_runner.a`.
- Bootloader binary for packaging: `build/ap_bootloader/ap_bootloader.bin`.
- Vendor/prebuilt library search dirs:
  - `external/luatos-soc-2024/PLAT/prebuild/PS/lib/gcc/ec718pm/oc`
  - `external/luatos-soc-2024/PLAT/prebuild/PLAT/lib/gcc/ec718pm/oc`
  - `external/luatos-soc-2024/PLAT/libs/ec718pm`
  - `external/luatos-soc-2024/lib`
  - `external/luatos-soc-2024/PLAT/device/target/board/ec7xx_0h00/ap/gcc`
- Link groups used by CSDK:
  - `ps`, `psl1`, `psif`, `psnv`, `tcpipmgr`, `lwip`, `osa`, `ccio`, `deltapatch2`
  - `middleware_ec`, `middleware_ec_private`, `driver_private`, `feat_USBMOD_FEAT_DEFAULT`, `usb_private`
  - `startup`, `core_airm2m`, `lzma`, `fota`, `csdk`
  - the runner/Arduino application archive, currently `air780epm_runner`, with whole-archive semantics
- Linker flags:
  - `-mcpu=cortex-m3`, `-mthumb`, `--specs=nano.specs`, `-lm`
  - `-Wl,--cref`, `-Wl,--check-sections`, `-Wl,--gc-sections`, `-Wl,--no-undefined`, `-Wl,--no-print-map-discarded`, `-Wl,--print-memory-usage`
  - wrap flags: `_malloc_r`, `_free_r`, `_realloc_r`, `clock`, `localtime`, `gmtime`, `time`
  - linker script path and map path

Post-link/package steps required outside xmake if Arduino CLI owns final link:

- Generate/preprocess `ec7xxxm_0h00_flash.ld` from `ec7xxxm_0h00_flash.c` using the same defines and include dirs.
- Generate `mem_map.txt` from `mem_map.h`.
- Run `arm-none-eabi-objcopy -O binary`.
- Run `fcelf -C` for AP binary compression/section processing.
- Run `fcelf -M` to package bootloader AP binary, AP binary, and CP firmware into `.binpkg`.
- Build `.soc` from `tools/pack/info.json`, `.binpkg`, `.elf`, `.map`, `comdb.txt`, and `mem_map.txt`.

## Arduino CLI Recipes That Need Real Implementations

`core/air780epm/platform.txt` needs a separate experimental platform or guarded recipe set. Do not replace the current bridge until Blink is proven.

Recipes to change for the experiment:

- `recipe.preproc.includes.windows`: use real `arm-none-eabi-g++ -E -CC` style preprocessing so library discovery sees normal include behavior.
- `recipe.preproc.macros.windows`: use real macro preprocessing, not `preprocess-copy`.
- `recipe.c.o.pattern.windows`: compile C sources with exported CSDK/Arduino compile flags.
- `recipe.cpp.o.pattern.windows`: compile C++ and `.ino.cpp` with exported flags.
- `recipe.S.o.pattern.windows`: compile assembly with exported `-mcpu=cortex-m3 -mthumb` and include/define set.
- `recipe.ar.pattern.windows`: use `arm-none-eabi-ar rcs` to produce Arduino core archive.
- `recipe.c.combine.pattern.windows`: replace `combine-xmake-build` with a real final link/package recipe, or call a helper that consumes the xmake export manifest plus Arduino build path.
- `recipe.size.pattern.windows`: call `arm-none-eabi-size` on the real ELF instead of returning zeroes.
- `recipe.output.tmp_file`/`recipe.output.save_file`: keep `.binpkg` output names after package step succeeds.

## Recommended Phase 1 Split

For Blink only, keep the split conservative:

1. Add an xmake export/prebuild mode that builds only CSDK-side stable artifacts and writes a manifest.
2. Remove sketch, third-party libraries, and `core/air780epm/cores/air780epm/*.cpp` from the xmake runner target in the experimental path.
3. Keep runner glue sources in xmake or move them to a separate `libair780epm_runner.a`; they provide LuatOS task entry and C wrappers used by the Arduino core.
4. Let Arduino CLI compile:
   - Blink-generated `.ino.cpp`
   - `core/air780epm/cores/air780epm/*.cpp`
   - third-party libraries, though Blink has none
5. Final link should use Arduino-produced archives plus xmake-produced `libcsdk.a`, runner glue archive, vendor/prebuilt libs, generated linker script, and xmake-equivalent link groups.
6. Packaging can initially stay in a PowerShell helper invoked by `recipe.c.combine.pattern.windows`, as long as final link is no longer delegated to xmake.

## Feasibility Verdict

Feasible, with one important condition: final link and packaging must exactly mirror `external/luatos-soc-2024/project/project.lua`. The CSDK static library already exists, but the current successful firmware is not only `libcsdk.a`; it also depends on bootloader output, vendor prebuilt library directories, link group ordering/whole-archive behavior, generated linker scripts, mem map preprocessing, and `fcelf` packaging.

The minimum Blink experiment should focus on reproducing the final `air780epm_runner.elf` link from Arduino CLI-built sketch/core archives plus xmake-exported CSDK artifacts. If that works, `.binpkg` and `.soc` are mechanical follow-up steps using the existing xmake packaging logic translated into a script.

## Phase 1 Blink Experiment Result

Status: Blink minimum path passed in this experimental worktree.

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -Clean -CliVerbose `
  -ArduinoCliPath "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
```

What this verified:

- Arduino CLI invoked real compile recipes for `Blink.ino.cpp`.
- Arduino CLI invoked real compile/archive recipes for `core/air780epm/cores/air780epm/*.cpp`.
- Arduino CLI produced `.arduino-cli-work\Blink\sketch\Blink.ino.cpp.o`.
- Arduino CLI produced `.arduino-cli-work\Blink\core\core.a`.
- xmake external mode linked those Arduino CLI outputs into `air780epm_runner.elf`.
- `fcelf` packaging completed and Arduino CLI output contains:
  - `.arduino-cli-work\Blink\Blink.ino.binpkg`
  - `.arduino-cli-work\Blink\Blink.ino_ec718pm.soc`

Map evidence:

- `runner/air780epm_runner/build/air780epm_runner/air780epm_runner_debug.map` contains `LOAD ...\.arduino-cli-work\Blink\sketch\Blink.ino.cpp.o`.
- The same map contains `LOAD ...\.arduino-cli-work\Blink\core\core.a`.
- The same map contains `.load_apos`, `Load$$LOAD_APOS$$Base`, and `Image$$LOAD_APOS$$Length`, so the AP package section expected by `sectionInfo_ec718pm.json` is present.

Important fix discovered during the experiment:

- The CSDK linker script is preprocessed from the final ELF target, not only from the CSDK/static-library target.
- If `FEATURE_FREERTOS_ENABLE` is not visible on `target(project_name .. ".elf")`, FreeRTOS `.sect_freertos_*` inputs become orphan sections and `fcelf -C` fails with `Find section load_apos failed`.
- The experimental runner ELF target now explicitly adds `FEATURE_OS_ENABLE` and `FEATURE_FREERTOS_ENABLE` so `.load_apos` is emitted.

Current split is still experimental:

- Arduino CLI owns sketch/core compilation and core archiving.
- `scripts/link_arduino_with_csdk.ps1` now owns the experimental combine boundary. It reads `arduino_export_manifest.json`, validates Arduino CLI outputs, writes `.arduino-cli-work\Blink\arduino_csdklib_link_inputs.json`, then invokes xmake external mode.
- xmake still owns CSDK/runner static archive build, final ELF link, and packaging.
- Final link currently injects Arduino CLI sketch object and `core.a` through external-mode xmake flags; a later phase should replace that helper internals with a direct linker invocation or cleaner xmake target/linkgroup integration.

PowerShell note:

- The experimental Windows recipes now call `pwsh`, because this machine has PowerShell 7 installed.
- Under `pwsh`, macro arguments with embedded quotes should be passed directly, for example `-DLWIP_CONFIG_FILE="lwip_config_cat.h"`. The previous Windows PowerShell-oriented `\"` escaping broke CSDK header inclusion under PowerShell 7.

New manifest/helper artifacts:

- `runner/air780epm_runner/build/arduino_export_manifest.json` exports compile flags plus link/package metadata.
- `.arduino-cli-work\Blink\arduino_csdklib_link_inputs.json` records the exact Arduino objects/archive handed into the CSDK link step.
- The helper also copies `Blink.ino.elf` and `Blink.ino.map` into the Arduino build directory in addition to `.binpkg` and `.soc`.

## Third-Party Library Link Probe Result

Status: Arduino CLI third-party library discovery, compilation, and final CSDK link passed in this experimental worktree.

Probe files:

- `libraries/Air780EpmLinkProbe`
- `examples/99.Experimental/ThirdPartyLinkProbe`

Command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\99.Experimental\ThirdPartyLinkProbe `
  -Clean -CliVerbose `
  -ArduinoCliPath "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
```

What this verified:

- Arduino CLI resolver found `Air780EpmLinkProbe.h`.
- Arduino CLI compiled `libraries\Air780EpmLinkProbe\src\Air780EpmLinkProbe.cpp`.
- The final CSDK/xmake external link consumed `.arduino-cli-work\ThirdPartyLinkProbe\libraries\Air780EpmLinkProbe\Air780EpmLinkProbe.cpp.o`.
- The symbol `air780epmLinkProbeValue(unsigned long)` resolved from the Arduino CLI-produced library object.
- Arduino CLI output contains:
  - `.arduino-cli-work\ThirdPartyLinkProbe\ThirdPartyLinkProbe.ino.binpkg`
  - `.arduino-cli-work\ThirdPartyLinkProbe\ThirdPartyLinkProbe.ino_ec718pm.soc`

Map evidence:

- `.arduino-cli-work\ThirdPartyLinkProbe\ThirdPartyLinkProbe.ino.map` contains `LOAD ...\sketch\ThirdPartyLinkProbe.ino.cpp.o`.
- The same map contains `LOAD ...\libraries\Air780EpmLinkProbe\Air780EpmLinkProbe.cpp.o`.
- The same map contains `LOAD ...\core\core.a`.
- The same map contains `air780epmLinkProbeValue(unsigned long)` at the library object.

Important fix discovered during this experiment:

- Arduino CLI library objects are emitted under `.arduino-cli-work\<sketch>\libraries\...`, not under `sketch` or `core`.
- The xmake external final-link target must include `libraries/**/*.o`.
- The combine helper should record these inputs in `arduino_csdklib_link_inputs.json` as `library_objects`; it should also reserve `library_archives` for Arduino CLI library archives if a later recipe/package layout emits `.a` files.

## Direct Link And Package Experiment Result

Status: direct final link and direct package passed for both Blink and the third-party library probe.

New experimental helper:

- `scripts/export_arduino_direct_link.ps1`

Purpose:

- Read `runner\air780epm_runner\build\arduino_export_manifest.json`.
- Read Arduino CLI outputs under `.arduino-cli-work\<sketch>`.
- Generate a direct GCC response file under `.arduino-cli-work\<sketch>\direct-link`.
- Preprocess a private linker script into the direct-link output directory instead of relying on xmake's temporary linker-script side effect.
- Directly call `arm-none-eabi-g++` to produce the final ELF.
- Directly run `arm-none-eabi-objcopy`, `fcelf -C`, `fcelf -M`, and `7z` to produce `.bin`, `.binpkg`, and `.soc`.

Commands verified:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\export_arduino_direct_link.ps1 `
  -BuildPath .\.arduino-cli-work\Blink `
  -ProjectName Blink.ino `
  -Package
```

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\export_arduino_direct_link.ps1 `
  -BuildPath .\.arduino-cli-work\ThirdPartyLinkProbe `
  -ProjectName ThirdPartyLinkProbe.ino `
  -Package
```

Direct-link outputs verified:

- `.arduino-cli-work\Blink\direct-link\Blink.ino.elf`
- `.arduino-cli-work\Blink\direct-link\Blink.ino.binpkg`
- `.arduino-cli-work\Blink\direct-link\Blink.ino_ec718pm.soc`
- `.arduino-cli-work\ThirdPartyLinkProbe\direct-link\ThirdPartyLinkProbe.ino.elf`
- `.arduino-cli-work\ThirdPartyLinkProbe\direct-link\ThirdPartyLinkProbe.ino.binpkg`
- `.arduino-cli-work\ThirdPartyLinkProbe\direct-link\ThirdPartyLinkProbe.ino_ec718pm.soc`

Important fixes discovered during this experiment:

- The AP linker script must be generated explicitly for a direct-link flow. The xmake flow generated `ec7xxxm_0h00_flash.ld` in `before_link`; the direct helper now preprocesses `ec7xxxm_0h00_flash.c` to a private `.ld` file in `.arduino-cli-work\<sketch>\direct-link`.
- `fcelf -M` requires a `mem_map.txt` that preserves macro definitions such as `AP_FLASH_LOAD_ADDR`. The direct helper must preprocess `mem_map.h` with `-dD`, not only `-E -P`.
- `fcelf -C` also expects a same-basename `.size` file next to the AP binary output. The direct helper now writes it from `arm-none-eabi-objdump -h` plus `arm-none-eabi-size` output before calling `fcelf -C`.
- `.soc` packaging is a normal `7z` archive over the pack directory after updating `tools\pack\info.json` so `rom.file` points at the generated `.binpkg`.

Updated feasibility conclusion:

- The first-stage architecture no longer needs xmake to own final ELF link/package.
- xmake is still useful as the CSDK/runner prebuild step that emits `libcsdk.a`, `libair780epm_runner.a`, `ap_bootloader.bin`, and the manifest.
- Arduino CLI can own sketch, core, and third-party library compilation; the final combine can be a manifest-driven PowerShell helper instead of an xmake external target.

## Arduino CLI Combine Recipe Direct-Link Result

Status: Arduino CLI combine now uses manifest-driven direct link/package in this experimental worktree.

Implementation:

- `scripts/build_core.ps1` has a `-PrebuildOnly` mode that configures xmake and builds only:
  - `csdk`
  - `ap_bootloader.elf`
  - `air780epm_runner`
- `scripts/link_arduino_with_csdk.ps1` now writes `arduino_csdklib_link_inputs.json` with `link_mode = direct-manifest-link-package`.
- The same helper then calls `build_core.ps1 -PrebuildOnly` to refresh CSDK/runner/bootloader artifacts.
- It finally calls `scripts/export_arduino_direct_link.ps1 -Package` and copies direct-link outputs back to the Arduino build root.

Arduino CLI commands verified after this switch:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -Clean -CliVerbose `
  -ArduinoCliPath "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
```

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\99.Experimental\ThirdPartyLinkProbe `
  -Clean -CliVerbose `
  -ArduinoCliPath "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
```

Outputs verified at the Arduino build root:

- `.arduino-cli-work\Blink\Blink.ino.binpkg`
- `.arduino-cli-work\Blink\Blink.ino_ec718pm.soc`
- `.arduino-cli-work\ThirdPartyLinkProbe\ThirdPartyLinkProbe.ino.binpkg`
- `.arduino-cli-work\ThirdPartyLinkProbe\ThirdPartyLinkProbe.ino_ec718pm.soc`

Map evidence after the recipe switch:

- `.arduino-cli-work\ThirdPartyLinkProbe\ThirdPartyLinkProbe.ino.map` contains `LOAD ...\sketch\ThirdPartyLinkProbe.ino.cpp.o`.
- The same map contains `LOAD ...\libraries\Air780EpmLinkProbe\Air780EpmLinkProbe.cpp.o`.
- The same map contains `LOAD ...\core\core.a`.
- The same map resolves `air780epmLinkProbeValue(unsigned long)` from the library object.

## Complex Third-Party Library Layout Probe Result

Status: complex Arduino library layout passed in this experimental worktree.

Probe files:

- `libraries/Air780EpmComplexLibProbe`
- `examples/99.Experimental/ComplexLibraryProbe`

What this verified:

- Arduino CLI discovered a library from a sketch include.
- Arduino CLI compiled `src\Air780EpmComplexLibProbe.cpp`.
- Arduino CLI compiled `src\ComplexProbeC.c`.
- Arduino CLI recursively compiled `src\detail\ComplexDetail.cpp`.
- The direct-link combine helper recorded all three library objects in `arduino_csdklib_link_inputs.json`.
- The final map loaded all three library objects and resolved:
  - `air780epmComplexProbeValue(unsigned long)`
  - `air780epm_complex_probe_c_step`
  - `air780epmComplexDetailValue(unsigned long)`
- Arduino CLI output contains:
  - `.arduino-cli-work\ComplexLibraryProbe\ComplexLibraryProbe.ino.binpkg`
  - `.arduino-cli-work\ComplexLibraryProbe\ComplexLibraryProbe.ino_ec718pm.soc`

Important recipe fixes discovered during this experiment:

- The synthetic Arduino library resolver probe must treat standard compiler headers such as `stdint.h` as built-in headers. Otherwise C library sources fail during dependency discovery before real compilation.
- The same probe must search the current source file directory for quoted includes, matching normal compiler behavior. Otherwise nested library sources such as `src\detail\ComplexDetail.cpp` falsely report adjacent headers as missing.

## Real Third-Party Library Probe Result

Status: real Arduino library manager library passed in this experimental worktree.

Library:

- `ArduinoJson@7.4.3`
- Installed with Arduino CLI into the worktree user directory: `libraries\ArduinoJson`
- The external library directory is ignored by git as `libraries/ArduinoJson/`; the committed artifact is the verification sketch and documentation, not the vendored library source.

Probe file:

- `examples/99.Experimental/ArduinoJsonProbe`

Command verified:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\99.Experimental\ArduinoJsonProbe `
  -Clean -CliVerbose
```

What this verified:

- Arduino CLI resolver found `ArduinoJson.h` from `ArduinoJson@7.4.3`.
- ArduinoJson's template/header-only implementation compiled inside the sketch object.
- The final direct-link/package flow produced:
  - `.arduino-cli-work\ArduinoJsonProbe\ArduinoJsonProbe.ino.binpkg`
  - `.arduino-cli-work\ArduinoJsonProbe\ArduinoJsonProbe.ino_ec718pm.soc`
- `arduino_csdklib_link_inputs.json` correctly recorded no `library_objects` for this header-only library.
- The final map contains ArduinoJson symbols in `.arduino-cli-work\ArduinoJsonProbe\sketch\ArduinoJsonProbe.ino.cpp.o`.

Reproduction note:

- If `libraries\ArduinoJson` is absent, run:

```powershell
.\tools\arduino-cli-release\arduino-cli.exe `
  lib install ArduinoJson `
  --config-file .\.arduino-cli-config\arduino-cli.yaml
```

## Project-Local Arduino CLI/Data Isolation

Status: project-local Arduino CLI and project-local Arduino data directories passed for Blink and ArduinoJsonProbe in this experimental worktree.

Default isolation:

- CLI executable: `tools\arduino-cli-release\arduino-cli.exe`
- Arduino CLI config: `.arduino-cli-config\arduino-cli.yaml`
- Arduino CLI package data: `.arduino-cli-data`
- Arduino CLI downloads/cache: `.arduino-cli-downloads`
- Arduino CLI user/libraries root: the worktree root

Why this matters:

- Sharing the Arduino CLI executable is normally safe because it is just a process.
- Sharing `C:\Users\cu80u\AppData\Local\Arduino15` with Arduino IDE is riskier because the IDE daemon and CLI can both update package indexes, builtin tools, and installed platform data.
- This experiment now defaults to project-local data/download directories, so it avoids competing with Arduino IDE or the mainline workspace for Arduino package state.
- `scripts\arduino_cli_setup.ps1 -UseSystemData` remains as an explicit escape hatch if a later compatibility test needs the system Arduino15 directory.

Verified CLI version:

```powershell
.\tools\arduino-cli-release\arduino-cli.exe version
```

Result:

```text
arduino-cli Version: 1.4.1 Commit: e39419312 Date: 2026-01-19T16:12:56Z
```

Commands verified with isolated defaults:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -Clean -CliVerbose
```

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\99.Experimental\ArduinoJsonProbe `
  -Clean -CliVerbose
```

Evidence:

- Setup reported `Data: F:\AIR780EXX_ArduinoCore_worktrees\csdk-prebuilt-a\.arduino-cli-data`.
- Arduino CLI used builtin `ctags` from `.arduino-cli-data\packages\builtin\tools\ctags\5.8-arduino11`.
- ArduinoJson resolved from the worktree user library directory: `F:\AIR780EXX_ArduinoCore_worktrees\csdk-prebuilt-a\libraries\ArduinoJson`.
- Final direct package outputs were produced for:
  - `.arduino-cli-work\Blink\Blink.ino.binpkg`
  - `.arduino-cli-work\Blink\Blink.ino_ec718pm.soc`
  - `.arduino-cli-work\ArduinoJsonProbe\ArduinoJsonProbe.ino.binpkg`
  - `.arduino-cli-work\ArduinoJsonProbe\ArduinoJsonProbe.ino_ec718pm.soc`

## Reproducible Flow Verification Script

Status: a single verification script now captures the current experimental acceptance path.

Software-only acceptance script:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_csdk_prebuilt_arduino_flow.ps1
```

Coverage:

- Confirms the project-local Arduino CLI executable is available.
- Compiles and packages Blink through the standard Arduino CLI flow.
- Compiles and packages `ComplexLibraryProbe`, verifying mixed C/C++ third-party library objects.
- Compiles and packages `ArduinoJsonProbe`, verifying a real Arduino library manager library.
- Checks final Arduino build-root outputs: `.elf`, `.map`, `.binpkg`, and `.soc`.
- Checks direct-link manifests for sketch objects, `core.a`, `libair780epm_runner.a`, `libcsdk.a`, and expected library objects.
- Checks final maps for sketch/core/library evidence and selected probe symbols.
- Confirms `.arduino-cli-config\arduino-cli.yaml` points at repo-local `.arduino-cli-data` and `.arduino-cli-downloads`.

Phase-1 umbrella acceptance script:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1
```

Use this as the default local regression command for the experiment. It runs the software-only verifier above.

Full distribution acceptance:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 `
  -IncludeDistribution
```

This runs the default software verifier, then exports and verifies the bundled CSDK prebuilt distribution package.

Optional hardware acceptance on COM3:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 `
  -FlashComPort COM3
```

This first runs the software verifier, then flashes Blink using the generated `.binpkg` and `.soc`, and finally runs `scripts\verify_log.ps1 -RequirePass`. Use this only when no other session is using the board port.

Optional distribution hardware acceptance on COM3:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 `
  -IncludeDistribution `
  -FlashDistribution `
  -FlashComPort COM3
```

This verifies the distribution package, flashes the distribution-built `ComplexLibraryProbe`, and checks the third-party library runtime PASS log.

## Hardware Flash Result

Status: Blink `.soc` produced by the Arduino CLI + direct CSDK link flow flashed successfully with `luatos-cli` and passed runtime log verification on hardware.

Command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\Blink\Blink.ino.binpkg `
  -SocFile .\.arduino-cli-work\Blink\Blink.ino_ec718pm.soc
```

Initial flash result:

- `luatos-cli 1.8.0` was used from the worktree-local `tools\luatos-cli-release\luatos-cli.exe`.
- Initial command port was `COM3`.
- The module rebooted into download mode as `COM20`.
- `ap_bootloader`, `Blink.ino`, and `cp-demo-flash` were flashed.
- Final tool output: `EC718 flash completed successfully`.

Initial post-flash log status:

- `scripts\verify_log.ps1 -ComPort auto -RequirePass` did not find a supported log device.
- `scripts\verify_log.ps1 -ComPort COM3 -RequirePass` failed because `COM3` was no longer present.
- After waiting, Windows still showed no AIR780EPM COM port.
- Blink runtime effect still needs visual LED confirmation or follow-up USB serial enumeration investigation.

Follow-up investigation:

- Hardware observation after the first luatos-cli flash: no normal three-COM-port enumeration and no Blink LED.
- Comparing ELF section layouts showed the direct-link ELF was missing `.arduino_init_array`, while the xmake-linked runner ELF contained it.
- The direct-link path generated its private linker script from the unpatched CSDK template, so the Arduino static-constructor section inserted by `build_core.ps1` for xmake was not present.
- `scripts\export_arduino_direct_link.ps1` now writes a private patched linker template into the direct-link output directory before preprocessing the final `.ld`.
- The regenerated Blink ELF/map now contains `.arduino_init_array`, `__arduino_init_array_start`, and `__arduino_init_array_end`.
- `scripts\verify_csdk_prebuilt_arduino_flow.ps1` now asserts these symbols are present before accepting a firmware build.

Second follow-up investigation:

- After the static-constructor fix, the board still failed to boot correctly because the AP entry name in the `.binpkg` was derived from the sketch project name.
- The CSDK/xmake package flow names the AP payload `ap`; the direct package path must preserve that name for the `AP_PKGIMG_LNA` entry.
- `scripts\export_arduino_direct_link.ps1` now copies the final AP binary to `ap.bin` before `fcelf -M`.
- `scripts\verify_csdk_prebuilt_arduino_flow.ps1` now parses `.binpkg` metadata and asserts that entries include `ap_bootloader`, `ap`, and `cp-demo-flash`, and that the sketch project name is not used as a flash entry.

Final hardware verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\Blink\Blink.ino.binpkg `
  -SocFile .\.arduino-cli-work\Blink\Blink.ino_ec718pm.soc
```

Observed final flash result:

- Initial command port was `COM3`.
- The module rebooted into download mode as `COM20`.
- The flash entries were `ap_bootloader`, `ap`, and `cp-demo-flash`.
- Final tool output: `EC718 flash completed successfully`.
- After reset, Windows enumerated the normal `COM3`, `COM4`, and `COM5` ports again.

Runtime verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_log.ps1 `
  -ComPort COM3 `
  -Duration 20 `
  -RequirePass
```

Observed runtime log:

- `+ARDUINO: CTOR,PASS`
- `+ARDUINO: AIR780EPM,READY`
- Repeated `+ARDUINO: BLINK,HIGH` and `+ARDUINO: BLINK,LOW`
- Final script output: `LOG_VERIFY: PASS`

## Third-Party Library Hardware Flash Result

Status: `ComplexLibraryProbe` flashed successfully and passed runtime log verification on hardware.

Why this probe matters:

- The sketch calls a third-party Arduino library discovered by Arduino CLI.
- The library includes a top-level C++ source file, a C source file, and a nested `src\detail` C++ source file.
- The runtime value depends on all three translation units being compiled by Arduino CLI and linked into the direct CSDK firmware.

Command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\99.Experimental\ComplexLibraryProbe `
  -BuildPath .\.arduino-cli-work\ComplexLibraryProbe `
  -Clean

pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\ComplexLibraryProbe\ComplexLibraryProbe.ino.binpkg `
  -SocFile .\.arduino-cli-work\ComplexLibraryProbe\ComplexLibraryProbe.ino_ec718pm.soc
```

Observed flash result:

- Initial command port was `COM3`.
- The module rebooted into download mode as `COM20`.
- The flash entries were `ap_bootloader`, `ap`, and `cp-demo-flash`.
- Final tool output: `EC718 flash completed successfully`.
- After reset, Windows enumerated `COM3`, `COM4`, and `COM5`.

Runtime verification:

```powershell
.\scripts\verify_log.ps1 `
  -ComPort COM3 `
  -Duration 20 `
  -RequirePass `
  -PassRegex @("\+ARDUINO: CTOR,PASS", "\+ARDUINO: COMPLEX_LIB_PROBE,VALUE,494780", "\+ARDUINO: COMPLEX_LIB_PROBE,PASS") `
  -FailRegex @("ASSERT", "PANIC", "FATAL", "\+ARDUINO: COMPLEX_LIB_PROBE,FAIL")
```

Observed runtime log:

- `+ARDUINO: CTOR,PASS`
- `+ARDUINO: COMPLEX_LIB_PROBE,VALUE,494780`
- `+ARDUINO: COMPLEX_LIB_PROBE,PASS`
- Final script output: `LOG_VERIFY: PASS`

## Prebuilt Distribution Shape Experiment

Status: a generated CSDK prebuilt distribution package can drive Arduino CLI compile/link/package without using the original CSDK/LuatOS source paths in the final manifest.

New scripts:

- `scripts\export_csdk_prebuilt_distribution.ps1`
- `scripts\verify_csdk_prebuilt_distribution.ps1`

Distribution output:

- `dist\csdk-prebuilt-air780epm`
- `dist\csdk-prebuilt-air780epm\arduino_export_manifest.json`

The distribution exporter copies the files currently required by Arduino compile, direct link, and package:

- Arduino-facing include directories from the CSDK, LuatOS, runner, core, and variant.
- Vendor/prebuilt link directories from `PLAT\prebuild`, `PLAT\libs`, CSDK `lib`, and board GCC output.
- `runner\air780epm_runner\build\csdk\libcsdk.a`.
- `runner\air780epm_runner\build\air780epm_runner\libair780epm_runner.a`.
- `runner\air780epm_runner\build\ap_bootloader\ap_bootloader.bin`.
- `cp-demo-flash.bin`.
- `fcelf.exe`, `sectionInfo_ec718pm.json`, `tools\pack`, and `comdb.txt`.
- Generated ABI inputs `abi\linker\air780epm_flash.ld` and `abi\package\mem_map.txt`, so user-side distribution builds do not regenerate them from the SDK linker template.
- SDK memory-map headers such as `mem_map.h` and `mem_map_csdk_*.h` are excluded from the distribution header copy after the Arduino core stopped directly including `common_api.h` for serial/time/analog paths.
- The SDK NVM header `osanvm.h` is excluded after `EEPROM` and `Preferences` moved through the runner-side `arduino_nvm_io` wrapper.
- CSDK network-private headers used only by runner-side TCP/UDP/TLS/modem wrappers, such as `networkmgr.h`, `psdial.h`, `ps_lib_api.h`, `lwip_config_cat.h`, `cmsis_os2.h`, `cmips.h`, and `osasys.h`, are excluded from the user distribution.
- The GNU Arm Embedded toolchain resolved by xmake, copied into `toolchain\gnu-rm` and referenced by the distribution manifest.

The exported manifest is marked with `distribution_package=true`. When `scripts\link_arduino_with_csdk.ps1` sees this flag, it only validates and reuses the packaged artifacts; it refuses to refresh prebuild artifacts through xmake. This models the eventual binary-distribution behavior.

Recipe override for the experiment:

```powershell
$env:AIR780EPM_ARDUINO_MANIFEST_PATH = (Resolve-Path .\dist\csdk-prebuilt-air780epm\arduino_export_manifest.json).Path
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -BuildPath .\.arduino-cli-work\BlinkDistPackage `
  -Clean
Remove-Item Env:\AIR780EPM_ARDUINO_MANIFEST_PATH
```

Repeatable verification command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_csdk_prebuilt_distribution.ps1 -Clean
```

What this verified:

- The package exporter rewrites manifest paths from the worktree root to `dist\csdk-prebuilt-air780epm`.
- The package exporter rewrites manifest toolchain paths from the xmake package cache to `dist\csdk-prebuilt-air780epm\toolchain\gnu-rm`.
- Blink compiles, links, and packages using the distribution manifest.
- `ComplexLibraryProbe` compiles, links, and packages using the distribution manifest.
- The direct-link response files contain `dist/csdk-prebuilt-air780epm`, proving the final link uses packaged paths.
- `ComplexLibraryProbeDistPackage` flashed successfully and passed runtime log verification on AIR780EPM hardware.

Distribution hardware verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\ComplexLibraryProbeDistPackage\ComplexLibraryProbe.ino.binpkg `
  -SocFile .\.arduino-cli-work\ComplexLibraryProbeDistPackage\ComplexLibraryProbe.ino_ec718pm.soc

.\scripts\verify_log.ps1 `
  -ComPort COM3 `
  -Duration 20 `
  -RequirePass `
  -PassRegex @("\+ARDUINO: CTOR,PASS", "\+ARDUINO: COMPLEX_LIB_PROBE,VALUE,494780", "\+ARDUINO: COMPLEX_LIB_PROBE,PASS") `
  -FailRegex @("ASSERT", "PANIC", "FATAL", "\+ARDUINO: COMPLEX_LIB_PROBE,FAIL")
```

Observed result:

- Flash used `COM3` and auto-entered download mode on `COM20`.
- Package entries were `ap_bootloader`, `ap`, and `cp-demo-flash`.
- Device re-enumerated `COM3`, `COM4`, and `COM5` after reset.
- Runtime log contained `+ARDUINO: COMPLEX_LIB_PROBE,VALUE,494780` and `+ARDUINO: COMPLEX_LIB_PROBE,PASS`.
- Final script output: `LOG_VERIFY: PASS`.

Current limitations:

- The package is slimmer than the first conservative export, but still not proven minimal. The exporter now copies header-like files from include directories and only the required `lib*.a` files from manifest link groups.
- Before bundling the toolchain, the verified export size was about 118 MB, down from the first conservative export of about 423 MB.
- After bundling the full GNU Arm toolchain, the verified export size is about 813 MB. This proves independence from the xmake toolchain cache, but the toolchain itself still needs a later slimming/pass-through strategy.
- The slimmed package was also flashed with `ComplexLibraryProbeDistPackage` and passed the same runtime log verification on hardware.
- Arduino core and variant sources still come from this Arduino core repository; that is expected because Arduino CLI owns core compilation.
- This proves the original full CSDK/LuatOS source trees are not required for the tested Arduino compile/link/package path after the distribution package is generated. A build machine still needs the source trees when regenerating `libcsdk.a`, `libair780epm_runner.a`, or bootloader artifacts.

## CSDK Prebuild Reuse Policy

Status: Arduino CLI combine now reuses existing CSDK prebuild artifacts by default.

Meaning of the Arduino combine stage:

- Arduino CLI first compiles sketch sources into object files.
- It compiles Arduino core sources and archives them into `core.a`.
- It compiles discovered third-party library sources.
- The combine recipe is the final Arduino CLI build stage that combines those Arduino outputs with non-Arduino link inputs. In this experiment, that stage invokes `scripts\link_arduino_with_csdk.ps1`, which links Arduino objects/archives with `libcsdk.a`, `libair780epm_runner.a`, vendor libraries, and packaging inputs to produce `.elf`, `.binpkg`, and `.soc`.

Default behavior:

- `scripts\link_arduino_with_csdk.ps1` checks whether the required CSDK-side artifacts already exist:
  - `runner\air780epm_runner\build\csdk\libcsdk.a`
  - `runner\air780epm_runner\build\air780epm_runner\libair780epm_runner.a`
  - `runner\air780epm_runner\build\ap_bootloader\ap_bootloader.bin`
  - required packaging tools/templates from the manifest
- It also validates `runner\air780epm_runner\build\arduino_prebuild_stamp.json`.
- If required files exist and the stamp matches, the combine stage reuses them and skips `build_core.ps1 -PrebuildOnly`.
- If any required artifact is missing, the stamp is missing, the input fingerprint changed, or an artifact hash changed, combine refreshes the prebuild automatically.

Fingerprint/stamp helper:

- `scripts\csdk_prebuild_stamp.ps1`

Stamp contents:

- xmake/board configuration fields used by the manifest.
- SHA-256 fingerprint over the selected prebuild inputs.
- SHA-256 hashes for generated artifacts:
  - `libair780epm_runner.a`
  - `libcsdk.a`
  - `ap_bootloader.bin`

Initial input fingerprint coverage:

- runner xmake/config files and `runner\air780epm_runner\src`, `inc`, `mem_map_7xx.h`
- key CSDK xmake files: `csdk.lua`, `project\project.lua`, `bootloader\bootloader.lua`
- linker/mem-map templates used by the direct link/package path
- `scripts\build_core.ps1` and `scripts\export_arduino_build_manifest.ps1`
- selected CSDK/LuatOS public/interface source/header directories used by the Arduino-facing build

Explicit refresh:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\prebuild_csdk.ps1
```

Clean refresh:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\prebuild_csdk.ps1 -Clean
```

One-off forced refresh during Arduino CLI compile:

```powershell
$env:AIR780EPM_REFRESH_CSDK_PREBUILD = "1"
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -Clean
Remove-Item Env:\AIR780EPM_REFRESH_CSDK_PREBUILD
```

## Next Integration Boundary

## OTA and Sleep Dist Hardware Validation

Status: OTA and sleep APIs have been ported into the CSDK prebuilt flow and validated on hardware through the distribution package.

Implementation boundary:

- `AIR780EPMOTA.{h,cpp}` remains an Arduino core API wrapper over `arduino_ota_io.h`.
- `arduino_ota_io.c` lives in `libair780epm_runner.a` and owns the CSDK/LuatOS HTTP/FOTA calls.
- `AIR780EPMSleep.{h,cpp}` remains an Arduino core API wrapper over `arduino_sleep_io.h`.
- `arduino_sleep_io.c` lives in `libair780epm_runner.a` and owns private sleep headers such as `slpman.h`, `platform_define.h`, and `luat_pm.h`.
- This keeps the Arduino CLI core compile path from directly including the private sleep SDK headers.

Software acceptance:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 -IncludeDistribution
```

Observed result:

- Arduino CLI direct-link flow passed for Blink, ComplexLibraryProbe, ArduinoJsonProbe, OtaApiReport, and SleepReport.
- Distribution-package compile passed for Blink, ComplexLibraryProbe, OtaApiReport, and SleepReport.

Sleep hardware validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\SleepReportDistPackage\SleepReport.ino.binpkg `
  -SocFile .\.arduino-cli-work\SleepReportDistPackage\SleepReport.ino_ec718pm.soc

$pass = @(
  "\+ARDUINO: SLEEP,READY",
  "\+ARDUINO: SLEEP,LIGHT,RETURNED",
  "\+ARDUINO: SLEEP,PASS"
)
$fail = @("ASSERT", "PANIC", "FATAL", "\+ARDUINO: SLEEP,.*FAIL")
.\scripts\verify_log.ps1 -ComPort COM3 -Duration 30 -RequirePass -PassRegex $pass -FailRegex $fail
```

Observed result:

- Flash used `COM3` and auto-entered download mode on `COM20`.
- Device re-enumerated `COM3`, `COM4`, and `COM5` after reset.
- Runtime log contained `+ARDUINO: SLEEP,READY`, `+ARDUINO: SLEEP,PAD0,CONFIG,OK`, `+ARDUINO: SLEEP,LIGHT,RETURNED`, and `+ARDUINO: SLEEP,PASS`.
- No ASSERT/PANIC/FATAL was observed.
- Final script output: `LOG_VERIFY: PASS`.

OTA failure-path hardware validation:

```powershell
$env:AIR780EPM_ARDUINO_MANIFEST_PATH = (Resolve-Path .\dist\csdk-prebuilt-air780epm\arduino_export_manifest.json)
try {
  pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
    -SketchPath .\validation_sketches\OtaFailureValidation `
    -BuildPath .\.arduino-cli-work\OtaFailureValidationDistPackage `
    -Clean
}
finally {
  Remove-Item Env:\AIR780EPM_ARDUINO_MANIFEST_PATH -ErrorAction SilentlyContinue
}

pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 `
  -ComPort COM3 `
  -PackageFile .\.arduino-cli-work\OtaFailureValidationDistPackage\OtaFailureValidation.ino.binpkg `
  -SocFile .\.arduino-cli-work\OtaFailureValidationDistPackage\OtaFailureValidation.ino_ec718pm.soc
```

Observed result:

- Flash completed successfully through `COM3` and download-mode `COM20`.
- Runtime log validated:
  - empty URL: `ERR,-1`
  - bad URL scheme: `ERR,-1`
  - auth argument mismatch: `ERR,-1`
  - no-network guard before registration: `ERR,-3`
  - invalid host download failure: final `STATE,ERROR,ERR,-4`
  - clear after error succeeded
  - `example.com` body was rejected by FOTA verification: final `STATE,ERROR,ERR,-5`
- No ASSERT/PANIC/FATAL was observed.
- No `STATE,STAGED` was observed in the failure-path run.
- Final script output: `LOG_VERIFY: PASS`.

The current experiment has crossed the phase-1 feasibility bar:

- Arduino CLI compiles the sketch.
- Arduino CLI compiles and archives the Arduino core as `core.a`.
- Arduino CLI discovers and compiles third-party libraries, including mixed C/C++ layout and header-only ArduinoJson.
- The combine stage links Arduino-produced outputs with `libcsdk.a`, `libair780epm_runner.a`, vendor libraries, generated linker script, and package inputs.
- The final `.binpkg`/`.soc` flashes and boots on AIR780EPM hardware.

The next engineering step is to keep the proven helper boundary but make the experimental platform recipe cleaner:

- Keep `pwsh` as the Windows recipe shell for this branch.
- Keep project-local Arduino CLI data/cache defaults to avoid fighting Arduino IDE or the mainline workspace.
- Keep `scripts\prebuild_csdk.ps1` and the stamp/fingerprint check as the CSDK refresh boundary.
- Keep `scripts\link_arduino_with_csdk.ps1` as the Arduino `recipe.c.combine.pattern.windows` implementation for now.
- Treat `scripts\export_arduino_direct_link.ps1` as the authoritative direct-link/package implementation until the direct linker arguments are stable enough to inline or simplify.
- Preserve the direct-link order asserted by `scripts\verify_csdk_prebuilt_arduino_flow.ps1`: vendor whole-group libraries, `libcsdk.a`, Arduino core archive, then runner whole-archive.
- Preserve the package entry names asserted by the verifier: `ap_bootloader`, `ap`, and `cp-demo-flash`.

Current recipe boundary:

- `recipe.c.combine.pattern.windows` calls `arduino_cli_recipe.ps1 combine-csdk-prebuilt`.
- `combine-csdk-prebuilt` invokes `scripts\link_arduino_with_csdk.ps1`, which refreshes or reuses CSDK prebuild artifacts and then calls the direct-link/package helper.
- `combine-xmake-build` remains as a compatibility alias only; new recipe work should use `combine-csdk-prebuilt`.
- `recipe.size.pattern.windows` calls `arduino_cli_recipe.ps1 report-size`, which now reports section sizes from the real Arduino build-root ELF with `arm-none-eabi-size -A`.

Do not remove the local LuatOS/CSDK trees yet:

- A build machine still needs them to refresh `libcsdk.a`, `libair780epm_runner.a`, `ap_bootloader.bin`, linker/mem-map templates, package metadata, tools, and vendor libraries.
- A future binary distribution could ship those generated artifacts plus the manifest and tools, but that is a later packaging problem and has not been proven in this worktree.

## Open Risks

- The direct-link helper currently owns explicit link order and whole-archive behavior. If this is moved into raw `platform.txt` recipes later, the same order must be preserved and re-verified.
- Some CSDK warning flags are C-only but currently also reach C++; Arduino recipes should separate C and C++ warning flags to avoid noisy builds.
- Static constructor support depends on patching the generated linker template so `.arduino_init_array` and its start/end symbols exist. The verifier now guards this, but any future linker-script generation change must keep the assertion.
- `mem_map_7xx.h` override is copied into the SDK path during current builds; the experiment should avoid mutating CSDK files and instead pass the runner include directory during linker-script/mem-map preprocessing.
- `LuatOS` and `luatos-soc-2024` submodules plus downloaded prebuilt libs are required for reproducible local validation.
- If Arduino CLI eventually owns final link/package without the helper script, the exported manifest must still include linker-preprocessor defines such as `FEATURE_OS_ENABLE` and `FEATURE_FREERTOS_ENABLE`; missing these can produce a linkable ELF that cannot be packaged by `fcelf`.
