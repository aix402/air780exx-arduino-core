# AIR780EPM Arduino Package Install Guide

This guide describes the first Windows release-candidate package shape.

## Scope

- Host OS: Windows.
- Board package: `air780:air780`.
- Board: `AIR780EPM Dev Board`.
- Tool packages installed by Arduino Boards Manager:
  - `air780:air780epm-csdk`
  - `air780:gnu-rm`
  - `air780:luatos-cli`

The normal Arduino build flow compiles sketches, the Arduino core, and Arduino
libraries with Arduino CLI/IDE recipes, then links against the prebuilt CSDK and
runner static libraries.

## Install In Arduino IDE

1. Open Arduino IDE preferences.
2. Add the published package index URL to Additional Boards Manager URLs.
3. Open Boards Manager.
4. Search for `AIR780`.
5. Install `AIR780 Arduino Core`.
6. Select `AIR780EPM Dev Board`.

The package index URL must point to the published
`package_air780_index.json`. The local
`package_air780_index.draft.json` generated during release testing
uses a temporary `127.0.0.1` URL and is not a publishable URL.

Current public package index:

```text
https://github.com/aix402/air780exx-arduino-core/releases/download/v0.2.0/package_air780_index.json
```

## Proxy Notes

If Arduino CLI/IDE can open Arduino's built-in indexes but fails to fetch the
AIR780 package index or GitHub release assets with `wsarecv` / connection-reset
errors, route Arduino CLI through the local proxy in the CLI config:

```yaml
network:
  proxy: http://127.0.0.1:7897
```

This avoids changing the system WinHTTP proxy and does not require administrator
permissions. Adjust the port to match the local proxy service.

## Compile

Open `File > Examples > AIR780EPM Dev Board Examples > AIR780 >
01.Basics > Blink`, then click Verify.

Arduino IDE may also list IDE-bundled or user-installed libraries, such as
Ethernet or ArduinoJson, under the current board's examples section when those
libraries declare broad architecture compatibility. Those entries are not
bundled by the AIR780 package.

The package manager installation should provide the CSDK ABI package, GNU Arm
toolchain, and `luatos-cli`. Users should not need xmake, LuatOS source, or
`luatos-soc-2024` source for normal sketch compilation.

## Upload

For a normally running board, select the board serial port in Arduino IDE and
click Upload. On the current AIR780EPM development board this is commonly
`COM3`, but the exact COM number is assigned by Windows.

The upload recipe calls:

```powershell
luatos-cli flash run --soc <firmware.soc> --port <selected-port>
```

## Boot Mode Recovery Upload

If the firmware is not running and the normal command/log port is unavailable,
enter EC718 Boot/download mode manually:

1. Hold BOOT.
2. Press RESET or power-cycle the board.
3. Release BOOT.
4. Wait for Windows to enumerate the EC718 download port.

For command-line recovery, use `auto` as the upload port:

```powershell
arduino-cli compile `
  -b air780:air780:air780epm_dev `
  --upload `
  -p auto `
  <sketch-path>
```

This maps to `luatos-cli flash run --port auto`. In the verified release smoke,
`luatos-cli` detected the EC718 download-mode port as `COM7` and completed a
full flash.

## Release Gate

Before publishing a package index, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_package_index_install.ps1
```

The gate regenerates the no-toolchain CSDK ABI archive, platform archive, GNU
Arm toolchain archive, and `luatos-cli` archive, generates the draft package
index, installs the package into an isolated Arduino15-like data directory,
verifies the installed platform shape, verifies the CSDK tool does not contain a
bundled `toolchain` directory, compiles installed-package `Blink` with Arduino
CLI's default build path, installs `PubSubClient` through the Arduino library
index, compiles the installed-package `MqttsLoopback` example, then compiles
`ComplexLibraryProbe` with a temporary sketchbook library to keep experimental
third-party library coverage out of the published example menu.

Also verify the generated package index references the intended archive files
and that each archive size and SHA256 checksum matches the file in
`dist\releases`.

The `v0.1.1` public GitHub release fixes the `air780epm-csdk` tool archive so
it no longer bundles the GNU Arm toolchain. The CSDK tool archive is about
39.82 MiB instead of about 234.48 MiB, and the package uses the separate
`air780:gnu-rm@10.2.1-ec718` tool dependency.

The `v0.1.1` public GitHub release has passed:

- local package-index install smoke from generated `v0.1.1` assets;
- installed CSDK tool shape verification: `toolchain.source=external-tool` and
  no bundled `toolchain` directory;
- installed-package `AIR780 > 01.Basics > Blink` compile;
- published package index download and SHA256 verification;
- published CSDK tool asset HEAD check showing `41750759` bytes.
- Arduino IDE Boards Manager upgrade from installed `0.1.0` to `0.1.1`;
- Arduino IDE `AIR780 > 01.Basics > Blink` compile after upgrade;
- Arduino IDE upload to AIR780EPM hardware after upgrade;
- Blink runtime verification on AIR780EPM hardware after upgrade.

After merging the prebuilt release work into `main`, the main branch also
passed a final hardware smoke on AIR780EPM: `scripts\arduino_cli_upload.ps1`
compiled `examples\01.Basics\Blink`, reset the running module from `COM3` into
download mode, flashed through the detected `COM7` download port with
`luatos-cli`, and the board LED blinked at the expected cadence.

The earlier `v0.1.0` public GitHub release passed:

- online package-index install from the GitHub Release URL using
  `network.proxy`;
- installed-package `AIR780 > 01.Basics > Blink` compile.

The earlier `v0.1.0-rc5` public GitHub release candidate passed:

- local package-index install smoke from a generated `127.0.0.1` index;
- online package-index install from the GitHub Release URL using
  `network.proxy`;
- installed-package `AIR780 > 01.Basics > Blink` compile;
- Arduino IDE download and install;
- Arduino IDE `AIR780 > 01.Basics > Blink` compile;
- Arduino IDE upload to AIR780EPM hardware;
- Blink runtime verification on AIR780EPM hardware.

The earlier `v0.1.0-rc4` candidate also passed Arduino IDE install,
`AIR780 > 01.Basics > Blink` compile, and upload on AIR780EPM hardware.
