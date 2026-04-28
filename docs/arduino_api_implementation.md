# AIR780EPM Arduino API Implementation

This document records API status by verification level. Compile-enabled means
the API builds through the Arduino CLI bridge and xmake runner; it does not
mean the related hardware behavior has been observed on a board.

## P0 Core Compatibility

| API | Status | Notes |
| --- | --- | --- |
| `setup()` / `loop()` | Hardware-observed | Blink and serial ready logs have run on AIR780EPM. |
| Static constructors | Hardware-observed | Uses Arduino-owned `.arduino_init_array`; `__libc_init_array()` is not used. |
| `delay()` / `millis()` / `micros()` | Compile-enabled | Basic runner timing path is present; timing accuracy validation is pending. |
| `delayMicroseconds()` | Compile-enabled | Calls the CSDK microsecond delay helper; timing accuracy validation is pending. |
| `pinMode()` / `digitalWrite()` / `digitalRead()` | Hardware-observed for LED | Broader GPIO route and pull-mode validation is pending. |
| Math constants and helpers | Compile-enabled | `PI`, `TWO_PI`, `radians()`, `degrees()`, `sq()`, `constrain()`, `min()`, `max()`, `map()`. |
| Bit and byte helpers | Compile-enabled | `lowByte()`, `highByte()`, `bitRead()`, `bitSet()`, `bitClear()`, `bitToggle()`, `bitWrite()`, `bit()`, `_BV()`. |
| Character helpers | Compile-enabled | `WCharacter.h` provides Arduino-style wrappers such as `isDigit()` and `toLowerCase()`. |
| Random helpers | Compile-enabled | `randomSeed()`, `random(max)`, `random(min, max)` still use libc pseudo-random generation for now. The TLS path is separate and now uses `luat_crypto_trng()` directly. |
| `pgmspace.h` | Compile-enabled | Compatibility-only AVR program-space macros and `pgm_read_*` helpers. |
| `Print` | Compile-enabled | `print()`, `println()`, `printf()`, bases, floats, `Printable`. |
| `Stream` | Compile-enabled | Timeout reads, `find()`, `parseInt()`, `parseFloat()`, `readString()`. |
| `String` | Compile-enabled | Expanded to the common Arduino/ESP32 shape used by third-party libraries: numeric constructors and concat, substring/search, trim/case/replace, conversion helpers, byte copy helpers, comparisons, and `StringSumHelper`. |
| `F()` / `PSTR()` / `PROGMEM` | Compile-enabled | Currently compatibility macros only; AIR780EPM does not use AVR program space semantics. |

## Bus APIs

| API | Status | Notes |
| --- | --- | --- |
| `Wire` | Hardware-observed with SHT40 and SCD4x | LuatOS I2C ID 0; default route follows CSDK IOMUX on GPIO14/GPIO15. |
| `Wire1` | Compile-enabled | LuatOS I2C ID 1; hardware validation pending. |
| `TwoWire` buffers | Compile-enabled | Fixed 32-byte TX/RX buffers; `setBufferSize()` is accepted but does not allocate a larger buffer yet. |
| `TwoWire::setPins()` / `begin(sda, scl)` | Compile-enabled | Arguments are accepted for source compatibility; custom hardware remap is not implemented yet. |
| `SPI` | Compile-enabled | LuatOS SPI ID 0; common transfer/write/transaction signatures are present. Hardware SPI validation pending. |
| `SPI1` | Compile-enabled | LuatOS SPI ID 1; common transfer/write/transaction signatures are present. Hardware SPI validation pending. |
| `SPI.begin(sck, miso, mosi, ss)` | Compile-enabled | Arguments are accepted for source compatibility; custom hardware remap is not implemented yet. |

## ADC

| API | Status | Notes |
| --- | --- | --- |
| `analogRead(A0..A3)` | Hardware-observed on `A1`, compile-enabled on `A0/A2/A3` | First AIR780EPM ADC contract uses logical analog aliases `A0..A3`, not GPIO numbers. Default returned width is 12 bits. Module pin mapping is now fixed as `A0/PIN9`, `A1/PIN96`, `A2/PIN77`, `A3/PIN76`. |
| `analogReadMilliVolts(A0..A3)` | Hardware-observed on `A1`, compile-enabled on `A0/A2/A3` | Converts the LuatOS calibrated microvolt result into integer millivolts. `A1` was flashed and log-verified around `2273-2281 mV` on module `PIN96`. The other channels now have published module-pin mapping, but still need runtime voltage validation. |
| `analogReadResolution(bits)` | Compile-enabled | Supports 1-16 returned bits. Default is 12-bit. |

## PWM

| API | Status | Notes |
| --- | --- | --- |
| `analogWrite(pin, value)` | Hardware-observed on `PIN_PWM4` | Uses GPIO-numbered default PWM pins only: `PIN_PWM0`, `PIN_PWM1`, `PIN_PWM2`, `PIN_PWM4`. Visible LED brightness changes were observed on `GPIO33/PIN26`. |
| `analogWriteResolution(bits)` | Compile-enabled | Supports 1-16 bits; default is 8-bit Arduino behavior. |
| `analogWriteFrequency(pin, hz)` | Compile-enabled | Sets one PWM channel frequency before or during output. Valid LuatOS range is 1 Hz to 13 MHz. |
| `analogWriteFrequency(hz)` / `analogWriteFreq(hz)` | Compile-enabled | Sets all default PWM channel frequencies. |

## Servo

| API | Status | Notes |
| --- | --- | --- |
| `Servo::attach()` / `detach()` | Compile-enabled | Uses the AIR780EPM PWM layer at 50 Hz instead of a separate timer backend. Only PWM-capable default GPIO routes are accepted. |
| `Servo::write()` / `writeMicroseconds()` / `readMicroseconds()` | Compile-enabled | Keeps servo-owned PWM channels separate from generic `analogWrite()` frequency changes. |
| `examples\07.Servo\ServoPulseReport` | Compile-enabled | Ready for board-level validation on a known PWM-capable GPIO such as `PIN_PWM4/GPIO33`. |

## Network And Modem

| API | Status | Notes |
| --- | --- | --- |
| `IPAddress`, `Client`, `UDP` compatibility surface | Compile-enabled | Core headers are now exposed through `Arduino.h` for normal sketch and library use. |
| `AIR780EPMModem` status path | Hardware-observed | `NetworkStatusReport` was flashed and runtime-verified on AIR780EPM with SIM inserted. The board reached `REGISTERED=1`, `READY=1`, `HAS_IPV4=1`, and reported IPv4 `10.203.85.90`. `waitForNetwork()` now waits for registration, while `activatePDP()` remains the data-netif/IP readiness wait. |
| `AIR780EPMModem` identity path | Hardware-observed | `Modem.getIdentity()` was fixed on AIR780EPM by replacing the blocking MSISDN and SIM-slot sync paths with the LuatOS-safe `soc_mobile_get_sim_number()` and `soc_mobile_get_sim_id()` helpers. Hardware logs now show `IDENTITY,GET,1` plus IMEI / IMSI / ICCID / SIMID, with IoT SIM phone number correctly reported as empty. |
| `AIR780EPMClient` | Hardware-observed | `TcpHttpGet` was flashed and runtime-verified on AIR780EPM. The board reached `HTTP/1.1 200 OK` from `example.com:80`. |
| `AIR780EPMTLSClient` | Hardware-observed | `TlsHttpGet` was flashed and runtime-verified on AIR780EPM. The board reached `HTTP/1.1 200 OK` from `example.com:443` with `CellularClientSecure::setInsecure()`. Root cause for the earlier `-52` failure was not DNS, but `mbedtls_ctr_drbg_seed()` running under the EC718PM `mbedtls_ec7xx_config.h` profile where default entropy sources are disabled. The fix was to remove that unused DRBG seeding path and back the TLS RNG callback with `luat_crypto_trng()` directly. |
| `CellularClientSecure::setCACert()` + `PubSubClient` | Hardware-observed | `MqttsPubSubClientCaSmoke` was flashed and runtime-verified on AIR780EPM. The board connected to `airtest.openluat.com:8888`, subscribed, published, received its loopback payload through the `PubSubClient` callback, and printed `+ARDUINO: MQTTS_PUBSUB_CA,PASS`. |
| `CellularClient` + `MQTT by 256dpi` | Hardware-observed | `Mqtt256dpiSmoke` was flashed and runtime-verified on AIR780EPM with the third-party `MQTT` library by 256dpi. The board connected to the public plain MQTT broker `broker.emqx.io:1883`, subscribed, published, received its loopback payload through the 256dpi callback, and printed `+ARDUINO: MQTT_256DPI,PASS`. |
| `AIR780EPMUDP` | Hardware-observed | `UdpNtpReport` was flashed and runtime-verified on AIR780EPM. The board received a 48-byte NTP packet from `pool.ntp.org` and reported a valid epoch. |
| `configTime()` / `configTzTime()` / `getLocalTime()` | Hardware-observed | First Arduino-compatible time helpers are now present. `configTime()` stores fixed-offset NTP config, `configTzTime()` currently supports fixed-offset POSIX TZ strings such as `CST-8`, and `getLocalTime()` first accepts an already-valid system time / NITZ result, then falls back to the core's built-in UDP NTP request path if needed. |
| `WiFiClient`, `WiFiClientSecure`, `WiFiUdp` aliases | Compile-enabled | Compatibility aliases only; they map to cellular-backed AIR780EPM client classes, not real Wi-Fi hardware. |
| `validation_sketches\NetworkApiP1Report` | Compile-enabled | P1 network API surface compile gate. |
| `examples\08.Network\NetworkStatusReport` | Hardware-observed | First SIM/network status smoke is now validated on hardware. |
| `examples\08.Network\ModemInfoReport` | Hardware-observed | After separating `waitForNetwork()` from `activatePDP()`, hardware runtime logs now show `RUNTIME,WAIT_OK,1`, `STATUS_OK,1`, `REGISTERED,1`, `NET_READY,1`, `HAS_IPV4,1`, and IPv4 `10.45.98.99`. That proves the sketch now reaches the post-wait status path and enters `loop()`. |
| `examples\08.Network\TcpHttpGet` | Hardware-observed | Runtime logs now show `TCP_HTTP,CONNECT,1`, `STATUS_LINE,HTTP/1.1 200 OK`, and `PASS`. |
| `examples\08.Network\TlsHttpGet` | Hardware-observed | Runtime logs now show `TLS_HTTP,CONNECT,1`, `STATUS_LINE,HTTP/1.1 200 OK`, `TLSERR,0`, and `PASS`. |
| `examples\08.Network\UdpNtpReport` | Hardware-observed | Runtime logs now show `UDP_NTP,PACKET,48`, a real remote IPv4/port, a valid epoch, and `PASS`. |
| `examples\08.Network\NetworkTimeReport` | Hardware-observed | Runtime logs now show `NET_TIME,EPOCH,<valid>`, `NET_TIME,LOCAL,2026-04-28 16:42:01`, and `NET_TIME,PASS` on AIR780EPM. `Modem.getTimeStatus()` now reports the same post-sync epoch path seen by `time()` / `getLocalTime()`. |
| `validation_sketches\MqttsPubSubClientCaSmoke` | Hardware-observed | Runtime logs now show `CONNECT,1`, `SUBSCRIBE,1`, `PUBLISH,1`, an `RX` callback with the loopback payload, and `PASS` using the third-party `PubSubClient` library. |
| `validation_sketches\Mqtt256dpiSmoke` | Hardware-observed | Runtime logs now show `CONNECT,1`, `SUBSCRIBE,1`, `PUBLISH,1`, an `RX` callback with the loopback payload, and `PASS` using the third-party `MQTT` library by 256dpi against `broker.emqx.io:1883`. |
| `validation_sketches\NTPClientReport` | Hardware-observed | Third-party `NTPClient` over the `WiFiUDP` alias is hardware-observed through network readiness, NTP update, valid epoch, `TIME_SET,1`, and `PASS`. |

## NVM And File System

| API | Status | Notes |
| --- | --- | --- |
| `EEPROM` | Hardware-observed | `EepromPreferencesReport` was flashed and runtime-verified. A second flash/run observed `EEPROM` counter progression from `PREV=0` to `NEXT=1`, indicating data persisted across reflashing. |
| `Preferences` | Hardware-observed | The same validation observed `Preferences` counter progression from `PREV=1` to `NEXT=2` and preserved label `air780epm`. |
| `LittleFS.begin()` / `format()` / `totalBytes()` / `usedBytes()` | Hardware-observed | AIR780EPM runner LFS adapter is working on hardware. `LittleFSReport` reported total `172032` bytes and successful runtime open/read/write flow. |
| `FS` path operations | Hardware-observed | `exists/remove/rename/mkdir/rmdir/open/readdir` were runtime-verified through `LittleFSReport`. |
| `examples\09.NVM\EepromPreferencesReport` | Hardware-observed | Runtime logs confirmed write/commit and persisted counters across reflashing. Pure reset-only or power-cycle-only validation can still be added later. |
| `examples\10.FileSystem\LittleFSReport` | Hardware-observed | Runtime logs confirmed `MKDIR/OPEN_WRITE/EXISTS/RENAME/OPEN_READ/OPEN_ROOT/REMOVE/RMDIR` all returned `OK`, with payload `air780epm-littlefs`. |

## Display Bring-Up

| Capability | Status | Notes |
| --- | --- | --- |
| `AIR780EPM_LuatOS.h` bridge header | Compile-verified | Exposes low-level LuatOS/CSDK display headers to sketches that need board bring-up access. This is not a stable Arduino display API. |
| `examples\05.Display\St7796HwIfProbe` | Hardware-observed | AIR780EPM dev board v1.2 probe for ST7796 on the dedicated `LUAT_LCD_HW_ID_0` path with LCD power enabled by `GPIO28`. Runtime logs showed `+ARDUINO: LCD,INIT,0` and repeating `LCD,STAGE,RED/GREEN/BLUE/YELLOW/WHITE/BLACK`, and the panel was observed cycling full-screen colors on hardware. |

## Serial

| Object | Status | Notes |
| --- | --- | --- |
| `Serial` | Hardware-observed output | USB/log channel; does not claim UART0. |
| `Serial1` | Hardware-observed TX/RX | LuatOS UART ID 1. Default route is RX GPIO18/PIN17, TX GPIO19/PIN18. Verified with external `COM8` adapter using `Uart1Probe`. |
| `Serial2` | Compile-enabled | LuatOS UART ID 2. Hardware TX/RX validation pending. |
| `Serial3` | Compile-enabled | LuatOS UART ID 3. Hardware TX/RX validation pending. |

## Third-Party Library Bridge

| Capability | Status | Notes |
| --- | --- | --- |
| Sketchbook library discovery | Compile-verified | Detects sketch `#include` lines and searches Arduino sketchbook `libraries`. |
| Library source staging | Compile-verified | Copies root, `src`, `utility`, and `util` C/C++/assembly sources into the xmake runner. |
| Header include paths | Compile-verified | Adds staged header directories from libraries into xmake. |
| Direct library dependency closure | Compile-verified | Walks library source `#include` lines to stage dependent sketchbook libraries. |
| Header-only libraries | Compile-verified | ArduinoJson `JsonGeneratorExample` compiles. |
| Source libraries | Hardware-observed | SparkFun SCD4x `Example1_BasicReadings` compiles and reports CO2 data on AIR780EPM over `Wire`/I2C0. |
| Compatibility matrix script | Compile-verified | `scripts\validate_library_compat.ps1` runs selected third-party probes and reports missing local libraries as `SKIP`. |
| ArduinoJson runtime smoke | Hardware-observed | `validation_sketches\ArduinoJsonRuntimeSmoke` compiles with local ArduinoJson `7.4.3`; board logs on `COM3` at `921600` show repeated `+ARDUINO: ARDUINOJSON_RUNTIME,PASS,LEN,62,...`. |

## Current Compile Checks

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\00.Core\CoreApiP0Compile -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\01.Basics\Blink -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\02.Serial\SerialEcho -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\02.Serial\HardwareSerialPorts -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\02.Serial\Uart1Probe -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\02.Serial\SerialApiCompile -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\03.Bus\BusApiP2Compile -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\03.Bus\SHT40Wire -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\04.PWM\PwmApiCompile -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\04.PWM\PwmLedProbe33 -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\05.Display\St7796HwIfProbe -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\06.Analog\AnalogReadReport -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\PinReport -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\PinCapabilities -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\ResourceBoundaryP4Report -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile smoke -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile pinmap_contract -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile sensor_io -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\check_static_ctors_map.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\07.Servo\ServoPulseReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\validation_sketches\NetworkApiP1Report
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\08.Network\NetworkStatusReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\08.Network\ModemInfoReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\08.Network\TcpHttpGet
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\08.Network\TlsHttpGet
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\validation_sketches\MqttsPubSubClientCaSmoke
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\validation_sketches\Mqtt256dpiSmoke
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\08.Network\UdpNtpReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\validation_sketches\NTPClientReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\08.Network\NetworkTimeReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\09.NVM\EepromPreferencesReport
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -SketchPath .\examples\10.FileSystem\LittleFSReport
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\ArduinoJsonRuntimeSmoke -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Case "arduinojson_runtime_smoke,ntpclient_report,pubsubclient_mqtts_ca_smoke,mqttclient_256dpi_smoke" -Clean -ContinueOnError

& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" --config-file "$env:USERPROFILE\.arduinoIDE\arduino-cli.yaml" compile -b openluat:ec718pm:air780epm_dev "$env:USERPROFILE\Documents\Arduino\libraries\SparkFun_SCD4x_Arduino_Library\examples\Example1_BasicReadings" --build-path ".\.arduino-ide-work\SCD4xBasic" --clean
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" --config-file "$env:USERPROFILE\.arduinoIDE\arduino-cli.yaml" compile -b openluat:ec718pm:air780epm_dev "$env:USERPROFILE\Documents\Arduino\libraries\ArduinoJson\examples\JsonGeneratorExample" --build-path ".\.arduino-ide-work\ArduinoJsonGenerator" --clean
```

## Known Gaps

- `Serial2` and `Serial3` hardware TX/RX have not been validated yet.
- Third-party Arduino library discovery and dependency staging are compile-
  verified for normal source libraries and header-only libraries, but do not yet
  implement full Arduino builder behavior such as architecture filtering,
  precompiled library archives, or platform-specific library recipes.
- `String` now covers the common Arduino/ESP32 methods needed by current
  probes, but ESP32-specific move/copy internals are not a public compatibility
  promise yet.
- AVR flash-string behavior is source-compatible only; strings are ordinary
  memory on this platform.
- `random()` is pseudo-random only until a hardware RNG-backed policy is
  designed.
- I2C1, SPI, and custom pin remap are not implemented as hardware-validated
  Arduino behavior yet. Use the CSDK default routes
  until route selection is explicitly designed and hardware-validated.
- ADC currently uses logical `A0..A3` channels only. GPIO-number analog inputs
  and public range selection are still pending.
- Normal ADC channels return microvolt-scale calibrated values from the LuatOS
  layer in this build configuration; the Arduino shim now converts those to
  millivolts for `analogReadMilliVolts()`.
- ST7796 validation currently uses the low-level LuatOS LCD hardware interface
  through `AIR780EPM_LuatOS.h`; a real Arduino display class and third-party
  graphics-library contract are still future work.
- I2C zero-byte address probes are not implemented yet, so an Arduino
  `I2CScanner` style sketch is not a validated runtime test at this stage.
- PWM duty-cycle changes have been hardware-observed on `PIN_PWM4/GPIO33`,
  but the other PWM routes and frequency changes have not been measured yet.
- `Servo` is compile-enabled on top of PWM, but no AIR780EPM servo pulse output
  has been hardware-observed yet.
- TCP, TLS, UDP, first-pass network time helpers, PubSubClient-backed CA MQTTS,
  and MQTT by 256dpi over plain TCP are now hardware-observed. Other MQTT
  libraries still need their own compatibility smokes.
- `configTzTime()` is intentionally first-pass only. Fixed-offset POSIX TZ
  strings are supported; full DST rule parsing is not.
- Fresh-boot `luatos-cli log view-binary --probe` capture can miss one-shot
  lines around the early modem/PDP transition. When validating setup completion,
  also inspect follow-on loop summaries or later no-probe captures.
- `EEPROM` / `Preferences` persistence has been observed across reflashing, but
  pure reset-only and power-cycle-only validation is still pending.
