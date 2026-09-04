# AIR780EPM vs ESP32 Arduino Compatibility Gaps

This document records the current compatibility target against common ESP32
Arduino library expectations. The goal is source compatibility where practical,
not a claim that AIR780EPM behaves like an ESP32 at every API edge.

## Working or Intentionally Matched

| Surface | Current status | Notes |
| --- | --- | --- |
| `Arduino.h` P0 helpers | Compile-enabled | math, bit/byte, character, `pgmspace.h`, `random()`, `map()` |
| `Print` / `Stream` / expanded `String` | Compile-enabled | common Arduino/ESP32 `String` methods now cover current in-repo examples and library probes |
| `HardwareSerial` objects | Compile-enabled | `Serial1`, `Serial2`, `Serial3` exist; `Serial` is USB/log, not UART0 |
| `Wire` / `Wire1` | Compile-enabled | common method shapes present |
| `SPI` / `SPI1` | Compile-enabled | common transaction and transfer shapes present |
| `analogWrite()` family | Compile-enabled | LuatOS PWM-backed, not ESP32 LEDC-backed |
| `Client` / `UDP` / WiFi compatibility aliases | Compile-enabled and partially hardware-observed | cellular-backed `WiFiClient`, `WiFiClientSecure`, and `WiFiUDP` aliases exist for common libraries; they are not real Wi-Fi |
| `EEPROM`, `Preferences`, `FS`, `LittleFS` | Hardware-observed | AIR780EPM storage smoke has passed; this is not ESP32 partition parity |

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
| AIR780EPM URL OTA facade | Hardware-observed, failure-first only | A board-specific `AIR780EPMOTA` state machine now exists, but it is not ESP32 `Update.h`. On 2026-04-29 the no-URL baseline plus failure-path runtime were verified on AIR780EPM; real `.sota` stage/apply is still pending |
| AIR780EPM sleep facade | Compile-enabled only | `AIR780EPMSleep` is a board-specific primitive, not ESP32 `esp_sleep_*`. Wake pads are PMU IDs, and `deepSleep()` means EC718PM `SLP2` |

## Not Yet Implemented

| Area | Notes |
| --- | --- |
| Interrupt helpers | `attachInterrupt()` family not implemented yet |
| SD / SPIFFS aliases | not implemented and not yet promised |
| Touch / DAC / LEDC extras | not implemented and not yet promised |

## Explicit Non-Goals for Now

| ESP32 expectation | AIR780EPM position |
| --- | --- |
| Real Wi-Fi stack | AIR780EPM is a cellular module; future compatibility aliases must document that clearly |
| Full ESP-IDF feature parity | Out of scope |
| ESP32 `Update.h` OTA contract | Not a first-pass target; AIR780EPM OTA will start with a narrower URL/diff OTA facade |
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
| Wi-Fi network objects | cellular-backed compatibility aliases when a library only needs `Client` / `UDP` contracts |
| Custom `Wire`/`SPI` remap | current default CSDK routes only |
| ADC by GPIO number | use logical `A0..A3`; GPIO-number analog mapping is not promised yet |
| ESP32 `esp_sleep_*` | `AIR780EPMSleep` for board-specific sleep primitives; wake pads are PMU IDs, not Arduino GPIOs |
