# AIR780EPM Phase 1 Scope

## Decisions

- Repository name: `air780exx-arduino-core`.
- Target module: `AIR780EPM`.
- Chip: `EC718PM`.
- Build backend: LuatOS CSDK xmake, external-project mode.
- SDK and LuatOS are git submodules during development.
- Release packaging should vendor the pinned submodule content into the Boards
  Manager package instead of requiring end users to clone dependencies.
- USB enumerated serial is the preferred log and flash path.
- `luatos-cli` is preferred for command-line flash/log validation.
- `F:\hezhou\luatools\Luatools_v3.exe` remains the manual fallback.

## Phase 1 Includes

- xmake build loop.
- C++ runtime and static constructor bridge.
- `setup()` / `loop()` runner.
- `Serial` output over the module log channel.
- Compile-enabled `Serial1`, `Serial2`, and `Serial3` HardwareSerial objects.
- Compile-enabled Arduino `Print`, `Stream`, and minimal `String`
  compatibility layer.
- Compile-enabled P0 helpers for math constants, bit/byte macros, character
  helpers, `pgmspace.h`, `random()`, `map()`, and `delayMicroseconds()`.
- Compile-enabled `Wire` and `SPI` object/API shapes.
- Compile-enabled `analogRead()`, `analogReadMilliVolts()`, and
  `analogReadResolution()` on logical `A0..A3`.
- Compile-enabled `analogWrite()` PWM API on CSDK default PWM routes.
- `Blink` smoke sketch.
- Arduino CLI sketch injection for repository examples.

## Phase 1 Excludes

- Network APIs.
- Hardware UART TX/RX validation.
- Hardware I2C/SPI validation and custom bus pin remap.
- Filesystem and NVM.
- Sleep and wakeup.
- OTA.
- Full Arduino CLI library bridge.
- Business code from existing product projects.

## Current Build Baseline

The xmake runner builds on Windows with:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
```

Current output:

```text
runner/air780epm_runner/out/air780epm_runner.binpkg
runner/air780epm_runner/out/air780epm_runner_ec718pm.soc
```

The default build enables the Arduino-owned static constructor bridge. It
temporarily patches the CSDK linker template during the build, restores the
submodule source afterward, and collects only Arduino/C++ `.init_array` entries
into `.arduino_init_array`:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\check_static_ctors_map.ps1
```

A recovery build can disable constructor execution if the hardware needs to be
returned to the minimal `setup()` / `loop()` baseline:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -DisableStaticConstructors
```

## First Hardware Validation

Verified on AIR780EPM over USB, using COM3 for flashing:

- `air780epm_runner.binpkg` can be flashed successfully.
- Serial log shows `+ARDUINO: AIR780EPM,READY`.
- The LED blinks at a regular cadence.
- `luatos-cli log view-binary --port auto --probe` decodes EC718 logs and
  auto-detects COM3 as the binary log port.
- Arduino CLI can compile and upload `examples\01.Basics\Blink` through FQBN
  `openluat:ec718pm:air780epm_dev`.
- Arduino IDE can compile, upload, and run the official Arduino `Blink` example
  on AIR780EPM.
- Arduino CLI can compile `examples\02.Serial\SerialApiCompile`, covering
  `Print`, `Stream`, `String`, `Printable`, numeric base output, float output,
  `F()`, and `printf()`.
- Arduino CLI can compile, upload, and run `examples\02.Serial\Uart1Probe` on
  AIR780EPM. `Serial1` TX/RX was hardware-observed on GPIO18/GPIO19 using an
  external `COM8` adapter, with periodic TX ticks and echoed RX payloads.
- Arduino CLI can compile `examples\00.Core\CoreApiP0Compile`, covering P0
  constants, type aliases, bit helpers, character helpers, `pgmspace.h`,
  `random()`, `map()`, and common math macros.
- Arduino CLI can compile `examples\03.Bus\BusApiP2Compile`, covering common
  `Wire`, `Wire1`, `SPI`, and `SPI1` API shapes without touching the bus at
  runtime.
- Arduino CLI can compile, upload, and run `examples\03.Bus\SHT40Wire` on
  AIR780EPM with an SHT40 connected to `Wire`/I2C0 GPIO14/GPIO15. Runtime logs
  showed repeated `+ARDUINO: SHT40,PASS` readings around 25.3 C and 31% RH.
- Arduino CLI can compile, upload, and run the sketchbook example
  `SparkFun_SCD4x_Arduino_Library\examples\Example1_BasicReadings` on
  AIR780EPM. CO2 readings were observed on hardware through `Wire`/I2C0.
- Arduino CLI can compile `examples\04.PWM\PwmApiCompile`, covering
  `analogWrite()`, `analogWriteResolution()`, and frequency helpers.
- Arduino CLI can compile `examples\06.Analog\AnalogReadReport`, covering the
  first ADC contract on logical `A0..A3`, `analogReadMilliVolts()`, and
  `analogReadResolution()`.
- `examples\06.Analog\AnalogReadReport` was flashed and log-verified on
  hardware. The `A1 -> ADC1` path reported stable readings around
  `2273-2281 mV` on module `PIN96`. The first-batch module-pin mapping is now
  fixed as `A0/PIN9`, `A1/PIN96`, `A2/PIN77`, and `A3/PIN76`. Runtime voltage
  validation is still pending for `A0`, `A2`, and `A3`.
- Arduino CLI can compile, upload, and run
  `examples\04.PWM\PwmLedProbe33` on AIR780EPM. Visible LED brightness changes
  were observed on `PIN_PWM4/GPIO33/PIN26`.
- Arduino CLI can compile, upload, and run
  `examples\05.Display\St7796HwIfProbe` on AIR780EPM dev board v1.2. Runtime
  logs showed `+ARDUINO: LCD,INIT,0` and repeating color-stage messages on the
  dedicated LCD hardware interface with power enabled by `GPIO28`. Full-screen
  color cycling was hardware-observed on the panel.

Static constructor bridge validation passed on hardware:

```text
+ARDUINO: AIR780EPM,READY
+ARDUINO: CTOR,PASS
+ARDUINO: BLINK,HIGH
```

## Arduino CLI Bridge Status

Local setup:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_setup.ps1
```

Arduino IDE 2.x setup:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_ide_setup.ps1
```

The IDE setup script backs up `C:\Users\cu80u\.arduinoIDE\arduino-cli.yaml` and
sets its sketchbook `directories.user` to this repository, so the IDE can
discover `hardware\openluat\ec718pm`.

If the IDE still reports `Platform 'aix402:ec718pm' not found`, the IDE is using
an old cached board selection. The repository examples include `sketch.yaml`
profiles bound to `openluat:ec718pm:air780epm_dev`; reopen the sketch or select
`AIR780EPM Dev Board` again.

Compile:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\01.Basics\Blink -Clean
```

Upload:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\01.Basics\Blink -ComPort COM3
```

Validation:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\verify_log.ps1 -RequirePass
```

Arduino IDE validation:

- Board: `AIR780EPM Dev Board`
- FQBN: `openluat:ec718pm:air780epm_dev`
- Sketch: official Arduino `Blink` example
- Result: compile, upload, and runtime blink verified on AIR780EPM.

The bridge uses Arduino CLI for board discovery, preprocessing, and IDE
integration, then stages the generated `.ino.cpp` into
`runner\air780epm_runner\generated` and lets the LuatOS CSDK xmake runner own
the real firmware link/package step.

`arduino_cli_setup.ps1` defaults to the system Arduino data directory when
`library_index.json` is already present, avoiding the slow first-run library
index download. Use `-IsolatedData` only when a clean repo-local Arduino CLI data
directory is required.

## Constructor Smoke Incident

Calling `__libc_init_array()` from `air780epm_call_static_constructors()` caused
the downloaded firmware to stop enumerating USB serial ports on hardware.
Manually walking `__init_array_start` / `__init_array_end` is also unsafe in the
current link because those symbols resolve from newlib rather than an
Arduino-owned bounded section. The working bridge instead creates
`.arduino_init_array` and walks only `__arduino_init_array_start` to
`__arduino_init_array_end`.

Recovery output when building with `-DisableStaticConstructors`:

```text
+ARDUINO: AIR780EPM,READY
+ARDUINO: CTOR,SKIP
```

Known follow-up items:

- Do not call `__libc_init_array()` on AIR780EPM; it caused a no-enumeration boot
  failure.
- Always run `scripts/check_static_ctors_map.ps1` after linker changes.
- Install `luatos-cli` from its GitHub release instead of building the Rust
  source locally:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_luatos_cli_release.ps1
```

`scripts/upload_core.ps1` uses the release executable first and passes the
generated `.soc` file to `luatos-cli flash run`.
- `scripts/view_log.ps1` and `scripts/verify_log.ps1` use `luatos-cli` binary
  SOC log mode by default.
- `tools\arduino-cli-release\arduino-cli.exe` is optional; the bridge can use an
  existing Arduino CLI path, but the current repo-local release is already
  installed and does not need to be downloaded again.
- `scripts/run_regression_matrix.ps1` now provides the first minimal compile
  profiles: `smoke`, `pinmap_contract`, and `sensor_io`.
- `RTE_Device.h` is copied from the existing validated product project as a
  temporary EC718 board configuration seed. It needs to be reduced to an
  AIR780EPM-specific variant before release.
