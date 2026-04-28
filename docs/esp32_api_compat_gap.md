# AIR780EPM vs ESP32 Arduino Compatibility Gaps

This document records the current compatibility target against common ESP32
Arduino library expectations. The goal is source compatibility where practical,
not a claim that AIR780EPM behaves like an ESP32 at every API edge.

## Working or Intentionally Matched

| Surface | Current status | Notes |
| --- | --- | --- |
| `Arduino.h` P0 helpers | Compile-enabled | math, bit/byte, character, `pgmspace.h`, `random()`, `map()` |
| `Print` / `Stream` / minimal `String` | Compile-enabled | enough for current in-repo examples and current library probes |
| `HardwareSerial` objects | Compile-enabled | `Serial1`, `Serial2`, `Serial3` exist; `Serial` is USB/log, not UART0 |
| `Wire` / `Wire1` | Compile-enabled | common method shapes present |
| `SPI` / `SPI1` | Compile-enabled | common transaction and transfer shapes present |
| `analogWrite()` family | Compile-enabled | LuatOS PWM-backed, not ESP32 LEDC-backed |

## Accepted but Not Yet ESP32-Equivalent

| API shape | Current AIR780EPM behavior | Gap |
| --- | --- | --- |
| `Wire.begin(sda, scl)` | Arguments are accepted | No custom hardware remap yet |
| `TwoWire::setPins()` | Accepted for compatibility | No custom remap yet |
| `SPI.begin(sck, miso, mosi, ss)` | Arguments are accepted | No custom hardware remap yet |
| `TwoWire::setBufferSize()` | Accepted | Buffer size is not dynamically expanded yet |
| `Serial` | USB/log output path | Does not emulate ESP32 UART0 semantics |
| `analogRead()` / `analogReadMilliVolts()` | Compile-enabled on logical `A0..A3` | No GPIO-number analog mapping, no public range/attenuation API yet |
| `analogWriteFrequency()` | Board-specific helper | Not an ESP32 `ledc*` API clone |

## Not Yet Implemented

| Area | Notes |
| --- | --- |
| Filesystem | `FS`, `LittleFS`, `SPIFFS`, `SD` not implemented |
| NVM | `EEPROM` and `Preferences` not implemented |
| Sleep | no Arduino sleep/wakeup API yet |
| OTA | no `Update` or URL OTA contract yet |
| Network | no `Client`, `UDP`, `WiFiClient`, `WiFiUDP`, or modem-backed Arduino network layer yet |
| Interrupt helpers | `attachInterrupt()` family not implemented yet |
| Touch / DAC / LEDC extras | not implemented and not yet promised |

## Explicit Non-Goals for Now

| ESP32 expectation | AIR780EPM position |
| --- | --- |
| Real Wi-Fi stack | AIR780EPM is a cellular module; future compatibility aliases must document that clearly |
| Full ESP-IDF feature parity | Out of scope |
| Full Arduino builder parity | The current bridge stages source libraries but does not implement every Arduino builder rule |
| GPIO-number plus board-remap magic | The current contract keeps digital pins as GPIO numbers and avoids silent route changes |

## AIR780EPM-Specific Contracts Libraries Must Tolerate

1. `Serial` is the board log channel, not a UART0 clone.
2. Public digital pins use GPIO numbering.
3. Some duplicated module routes are intentionally not auto-remapped by the
   Arduino layer.
4. The build backend is an xmake runner under an Arduino CLI bridge, not the
   same linker/runtime environment as ESP32 Arduino.

## What to Use Instead of Blind ESP32 Assumptions

| If a library expects | Prefer on AIR780EPM |
| --- | --- |
| UART0 console | `Serial` for logs, `Serial1`+ for board UART tests |
| ESP32 `ledc*` | `analogWrite()` and AIR780EPM PWM aliases |
| Wi-Fi network objects | wait for modem-backed Arduino network layer |
| Custom `Wire`/`SPI` remap | current default CSDK routes only |
| ADC by GPIO number | use logical `A0..A3`; GPIO-number analog mapping is not promised yet |
