# AIR780EPM Build and Upload Notes

This document records the current build, flash, and log workflow for the
AIR780EPM Arduino Core bring-up.

## Current Baseline

| Item | Value |
| --- | --- |
| Target module | `AIR780EPM` |
| Chip | `EC718PM` |
| Arduino package | `openluat:ec718pm` |
| Board ID | `air780epm_dev` |
| FQBN | `openluat:ec718pm:air780epm_dev` |
| Core build backend | LuatOS CSDK `xmake` runner |
| Flash tool | `luatos-cli` release build |
| Manual fallback | `F:\hezhou\luatools\Luatools_v3.exe` |

## Source Layout

| Path | Role |
| --- | --- |
| `core\air780epm` | Arduino platform, core, variant, board package files |
| `runner\air780epm_runner` | xmake-native runner app that hosts `setup()` / `loop()` |
| `external\LuatOS` | LuatOS public headers and module reference tree |
| `external\luatos-soc-2024` | AIR780Ex CSDK and xmake build backend |
| `tools\luatos-cli-release` | Preferred command-line flash/log tool |

## Build Artifacts

Default xmake runner output:

```text
runner/air780epm_runner/out/air780epm_runner.binpkg
runner/air780epm_runner/out/air780epm_runner_ec718pm.soc
```

Arduino CLI bridge output for each sketch:

```text
.arduino-cli-work/<SketchName>/<SketchName>.binpkg
.arduino-cli-work/<SketchName>/<SketchName>_ec718pm.soc
```

Generated staged sketch sources and staged third-party libraries:

```text
runner/air780epm_runner/generated
```

## Flash Layout Override

The runner intentionally keeps `runner\air780epm_runner\mem_map_7xx.h`.
`external\luatos-soc-2024\csdk.lua` auto-detects this file and builds with
`__USER_MAP_CONF_FILE__="mem_map_7xx.h"`.

Current AIR780EPM Arduino runner layout:

| Region | Value | Notes |
| --- | --- | --- |
| AP image/package limit | `0x2c5000` / 2836 KiB | Matches the CSDK package AP limit used for this runner |
| FOTA region | `0x347000..0x3b7000` / 448 KiB | 352 KiB usable after the 96 KiB hib backup reservation |
| LittleFS region | `0x3b7000..0x3e1000` / 168 KiB | Kept unchanged |
| FDB/KV region | `0x3e1000..0x3f1000` / 64 KiB | Kept unchanged |

## OTA Budget Implication

As of 2026-04-29, the validated AIR780EPM Arduino runner is already in the
roughly `1.35 MiB` AP-image class once networking, TLS, storage, and validation
support are linked in. With only about `352 KiB` of usable in-flash FOTA
staging space, full-image OTA is not a sensible first target for this core.

The practical first OTA target is diff OTA. The upgrade artifact should be a
`.sota` package generated from an old `.soc` plus a new `.soc`; `.binpkg`
remains the flash/download artifact for normal runner uploads, not the diff-OTA
package input.

Removing the runner `mem_map_7xx.h` is not the preferred way to use the default
layout. It would let SDK feature macros choose the defaults, making Arduino
package builds less reproducible.

## Build Commands

Build the default runner app:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
```

Build the runner app and stage a sketch into the xmake project:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 `
  -SketchPath .\examples\01.Basics\Blink
```

Check the static constructor map after linker changes:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check_static_ctors_map.ps1
```

Recovery build with static constructors disabled:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 `
  -DisableStaticConstructors
```

## Arduino CLI Bridge

Set up the local hardware package mount:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_setup.ps1
```

Compile a sketch through Arduino CLI and the xmake runner:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -Clean
```

Compile the first ADC report sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 `
  -SketchPath .\examples\06.Analog\AnalogReadReport `
  -Clean
```

Upload a sketch through the Arduino CLI bridge:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -ComPort COM3 `
  -Clean
```

The current bridge uses Arduino CLI for FQBN discovery and sketch
preprocessing, then hands the real firmware link/package step to the xmake
runner.

Compile-only regression sweep:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile smoke -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile pinmap_contract -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile sensor_io -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile connectivity
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile storage
```

## Flash and Log

Install the preferred prebuilt `luatos-cli` release:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_luatos_cli_release.ps1
```

Flash the current default runner package:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 -ComPort COM3
```

View binary-decoded logs:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\view_log.ps1
```

Verify expected startup logs:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\verify_log.ps1 -RequirePass
```

## Observed Hardware Path

Hardware-observed baseline on the AIR780EPM dev board:

| Item | Observed status |
| --- | --- |
| USB flashing | Working |
| Flash COM port | `COM3` |
| `+ARDUINO: AIR780EPM,READY` startup log | Observed |
| Static constructor pass log | Observed with `+ARDUINO: CTOR,PASS` |
| Fallback constructor skip log | Observed with `+ARDUINO: CTOR,SKIP` when disabled |
| Blink LED | Observed |

`luatos-cli log view-binary --port auto --probe` has been the preferred log
path during current validation.

## Tooling Cautions

- Close `Luatools_v3.exe` before CLI-driven flash/log work. It can hold the
  port or interfere with log observation.
- Keep the CSDK linker template under script control. `build_core.ps1`
  temporarily patches the linker template for the Arduino-owned static
  constructor section and restores the source afterward.
- The current flash/log path assumes USB-enumerated ports. Do not claim a board
  UART as the primary `Serial` console contract.
- The release package should vendor pinned SDK/LuatOS content. End users should
  not need to clone submodules to install the board package.
