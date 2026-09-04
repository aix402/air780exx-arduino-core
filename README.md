# AIR780 Arduino Core

Arduino Core porting workspace for OpenLuat AIR780Ex modules.

The first supported target is `AIR780EPM`, based on the EC718PM chip. This
workspace uses the LuatOS CSDK xmake flow instead of the older SDK-internal
Makefile flow used by the ML307N-EC prototype.

## Repository Name

Suggested GitHub repository name:

```text
air780exx-arduino-core
```

## Layout

```text
core/air780epm/              Arduino platform files and minimal core
runner/air780epm_runner/     xmake CSDK runner that hosts setup()/loop()
examples/                    Arduino CLI compatible examples
libraries/                   small public helper libraries and probes
validation_sketches/         compile/runtime regression sketches
scripts/                     build, upload, and Arduino CLI bridge scripts
docs/                        public docs, release notes, and maintainer notes
tools/luatos-cli/            command-line flash/log tool submodule
```

## Public Repo Boundary

This workspace is organized so the public Arduino repository contains the
Arduino-facing source and build automation without embedding the full vendor
SDK:

- the public source boundary is `core/`, `runner/`, `examples/`, `libraries/`,
  `validation_sketches/`, `scripts/`, and `docs/`
- the LuatOS CSDK source tree is a maintainer-only dependency and can live
  outside the public repo in a sibling `deps/` directory
- ordinary users should install the published Boards Manager package, which
  contains the required prebuilt ABI files and tools
- maintainers need `deps/LuatOS/` and `deps/luatos-soc-2024/` only when
  rebuilding the SDK-backed artifacts
- release binaries should be published through GitHub Releases, not committed
  into git

Maintainer scripts support Windows PowerShell 5.1 and PowerShell 7. The
examples below use the Windows-included `powershell` command.

See [docs/maintainer_notes.md](docs/maintainer_notes.md) for the suggested
local layout and dependency policy.

## License

The original code in this repository is released under the MIT License. See
[LICENSE](LICENSE). Third-party components retain their original licenses; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the component list and
license locations.

## Project Docs

- [Maintainer notes](docs/maintainer_notes.md)
- [Public release checklist](docs/public_release_checklist.md)
- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)

## Phase 1 Scope

- xmake build loop for `AIR780EPM`
- C++ runtime basics and validated Arduino-owned static constructor bridge
- Arduino `setup()` / `loop()`
- `Serial` log output
- compile-enabled `Serial1`, `Serial2`, and `Serial3` HardwareSerial objects
- compile-enabled Arduino `Print`, `Stream`, and minimal `String`
- compile-enabled P0 compatibility helpers: math constants, bit/byte helpers,
  character helpers, `pgmspace.h`, `random()`, `map()`, and `delayMicroseconds()`
- compile-enabled `Wire` and `SPI` object/API shapes
- compile-enabled `analogRead()`, `analogReadMilliVolts()`, and
  `analogReadResolution()` on logical `A0..A3`
- compile-enabled `analogWrite()` PWM on the default LuatOS PWM routes
- initial third-party Arduino library bridge through Arduino CLI discovery and
  xmake runner staging
- `delay()`, `millis()`, `micros()`
- minimal GPIO for `Blink`

Hardware UART TX/RX validation, network, filesystem, sleep, OTA, and broad
third-party library compatibility are out of scope for the first phase.

## Pin Numbering

Arduino digital pins use AIR780EPM GPIO numbers, not module physical pin
numbers. For example, `pinMode(27, OUTPUT)` operates `GPIO27`; on the current
AIR780EPM dev board this is the built-in LED route.

The underlying OpenLuat hardware table still uses module physical pin numbers.
Digital GPIO follows the CSDK default IOMUX route; duplicate routes such as
GPIO18/GPIO19 are not changed by the Arduino Core unless a future explicit
remap API is used. The current pin contract and hardware validation backlog are
tracked in `docs/pinmap.md`.

Analog input uses a separate first-batch contract: `A0..A3` are logical ADC
channel tokens, not GPIO numbers. The core does not currently promise
`analogRead(GPIOx)` behavior. The current channel-to-module-pin mapping is:
`A0 -> ADC0 -> PIN9`, `A1 -> ADC1 -> PIN96`, `A2 -> ADC2 -> PIN77`,
`A3 -> ADC3 -> PIN76`.

## Serial Ports

`Serial` is the USB/log output channel and does not claim UART0. `Serial1`,
`Serial2`, and `Serial3` are compile-enabled HardwareSerial objects backed by
LuatOS UART IDs 1, 2, and 3. Their default pin routes follow `docs/pinmap.md`.
`Serial1` TX/RX has been hardware-observed on GPIO18/GPIO19; `Serial2` and
`Serial3` are still pending hardware validation.

`HardwareSerial` now derives from Arduino-style `Stream`, with common `Print`
helpers available on all serial objects. The current compatibility layer covers
basic `print()`, `println()`, `printf()`, numeric base formatting, float
formatting, `Printable`, `Stream` timeout reads, and a minimal heap-backed
`String`. This has been validated as compile-enabled through Arduino CLI.
Hardware observation now exists for the USB/log `Serial` path and `Serial1`
TX/RX; `Serial2` and `Serial3` are still pending.

`Wire` maps to LuatOS I2C ID 0 and `Wire1` maps to LuatOS I2C ID 1. `SPI` maps
to LuatOS SPI ID 0 and `SPI1` maps to LuatOS SPI ID 1. Their common Arduino
method shapes are compile-enabled. Custom pin arguments are accepted for source
compatibility, but hardware remap behavior has not been validated or promised.
`Wire` has been hardware-observed on I2C0 GPIO14/GPIO15 with both the in-repo
SHT40 sketch and the SparkFun SCD4x third-party library example.

PWM is available on the default LuatOS PWM channels. Use `PIN_PWM0`,
`PIN_PWM1`, `PIN_PWM2`, or `PIN_PWM4` with `analogWrite()`. `PIN_PWM4`
has been hardware-observed on GPIO33/PIN26 with a visible LED brightness probe;
the remaining PWM routes and frequency measurement are still pending.

ADC is compile-enabled on logical `A0..A3`. `analogRead()` defaults to a
12-bit returned value and `analogReadMilliVolts()` converts the LuatOS
calibrated microvolt result into Arduino-style millivolts. The `A1 -> ADC1`
path has been hardware-observed around `2273-2281 mV` on module physical
`PIN96`. The other first-batch analog aliases are now documented as
`A0/PIN9`, `A2/PIN77`, and `A3/PIN76`, but those three channels are still
waiting for runtime voltage validation. GPIO-numbered analog mapping is still
not promised.

For board bring-up, the core also exposes a thin low-level LuatOS bridge header
`AIR780EPM_LuatOS.h`. This is intended for platform validation sketches such as
the ST7796 LCD probe; it is not yet a stable high-level Arduino display API.

## Build

Build the default runner:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
```

The default build enables the Arduino static constructor bridge and should log
`+ARDUINO: CTOR,PASS`. A recovery build can disable constructors if needed:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -DisableStaticConstructors
```

Install the prebuilt `luatos-cli` release for command-line flashing:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_luatos_cli_release.ps1
```

Flash and view decoded EC718 binary logs:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 -ComPort COM3
powershell -ExecutionPolicy Bypass -File .\scripts\view_log.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\verify_log.ps1 -RequirePass
```

Check the generated map file before hardware validation:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check_static_ctors_map.ps1
```

## Arduino CLI Bridge

Set up the local platform mount:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_setup.ps1
```

This creates the local FQBN:

```text
air780:air780:air780epm_dev
```

The default setup uses the project-local Arduino CLI executable and
project-local data/cache directories:

- `tools\arduino-cli-release\arduino-cli.exe`
- `.arduino-cli-config\arduino-cli.yaml`
- `.arduino-cli-data`
- `.arduino-cli-downloads`

This avoids competing with Arduino IDE or another worktree for the shared
Arduino15 package state.

The current release candidate uses the CSDK prebuilt static-library flow:
Arduino CLI compiles sketches, the Arduino core, and third-party libraries, then
the combine step links those outputs with `libcsdk.a` and
`libair780epm_runner.a`. Ordinary users of a generated distribution package do
not need XMake. Maintainers only need XMake plus the CSDK/LuatOS trees when
refreshing the prebuilt `.a` artifacts or rebuilding the distribution package.

Detailed build flow:

- [CSDK prebuilt build flow](docs/csdk_prebuilt_build_flow.md)
- [Maintainer notes](docs/maintainer_notes.md)

Phase-1 software acceptance:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1
```

Distribution package acceptance:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_phase1_csdk_prebuilt_experiment.ps1 -IncludeDistribution
```

Create a release archive from the verified distribution package:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package_csdk_prebuilt_distribution.ps1 -Clean
```

This writes the `.zip`, `.zip.sha256`, and `.manifest.json` files under
`dist\releases`.

Compile and upload the Blink sketch:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\01.Basics\Blink -Clean
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\01.Basics\Blink -ComPort COM3
```

Compile the serial API compatibility sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\02.Serial\SerialApiCompile -Clean
```

Compile and upload the `Serial1` hardware probe:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\02.Serial\Uart1Probe -ComPort COM3 -Clean
```

Compile the P0 core API compatibility sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\00.Core\CoreApiP0Compile -Clean
```

Compile the bus API compatibility sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\03.Bus\BusApiP2Compile -Clean
```

Compile the SHT40 I2C runtime sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\03.Bus\SHT40Wire -Clean
```

Compile the PWM API compatibility sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\04.PWM\PwmApiCompile -Clean
```

Compile the ADC report sketch:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\06.Analog\AnalogReadReport -Clean
```

Compile and upload the visible PWM LED probe on `GPIO33/PIN26`:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\04.PWM\PwmLedProbe33 -ComPort COM3 -Clean
```

Compile and upload the AIR780EPM v1.2 ST7796 hardware-interface probe:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\05.Display\St7796HwIfProbe -ComPort COM3 -Clean
```

This probe uses the dedicated LuatOS LCD hardware interface `LUAT_LCD_HW_ID_0`,
drives LCD power through `GPIO28`, and assumes the AIR780EPM dev board v1.2
screen wiring. Runtime logs should show `+ARDUINO: LCD,INIT,0` followed by
`LCD,STAGE,...`; full-screen color cycling has been hardware-observed on the
AIR780EPM v1.2 board.

The CLI bridge stages the Arduino-preprocessed `.ino.cpp` into the xmake runner
and emits `.binpkg` plus `.soc` artifacts under `.arduino-cli-work\<SketchName>`.

Compile-only regression profiles:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile smoke -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile pinmap_contract -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile sensor_io -Clean
```

Third-party Arduino libraries installed in the sketchbook `libraries`
directory are discovered from sketch `#include` lines and staged into the xmake
runner under `runner/air780epm_runner/generated/libraries`. The current bridge
copies library sources from the library root, `src`, `utility`, and `util`
layouts, then adds staged headers and C/C++/assembly sources to xmake. It also
walks direct library `#include` dependencies found in those staged sources.

This is a source-compatibility bridge, not yet a full Arduino builder clone:
`library.properties` architecture filtering, precompiled library archives, and
platform-specific library recipes are not implemented. Current third-party
checks include compile-verified ArduinoJson and hardware-observed SparkFun
SCD4x basic readings on AIR780EPM.

For Arduino IDE 2.x discovery, back up and point the IDE sketchbook to this
repository:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_ide_setup.ps1
```

Restart the IDE, then select `AIR780EPM Dev Board`.

If the IDE reports `Platform 'aix402:ec718pm' not found`, it is using an old
cached board selection. Select `AIR780EPM Dev Board` again, or reopen the sketch
after running `scripts\arduino_ide_setup.ps1`.

The Arduino IDE path has been validated with the official Arduino `Blink`
example: compile, upload, and runtime blink all work on AIR780EPM.
