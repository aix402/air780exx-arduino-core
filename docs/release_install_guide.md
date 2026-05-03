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

The gate regenerates the platform and `luatos-cli` archives, generates the
draft package index, installs the package into an isolated Arduino15-like data
directory, verifies the installed platform shape, compiles installed-package
`Blink` with Arduino CLI's default build path, then compiles
`ComplexLibraryProbe` with a temporary sketchbook library to keep third-party
library coverage out of the published example menu.

Also verify the generated package index references the intended archive files
and that each archive size and SHA256 checksum matches the file in
`dist\releases`.
