# AIR780EPM Third-party Library Compatibility

This document records third-party Arduino library probes used to force the
core API surface into a practical shape. A passing compile means the AIR780EPM
Arduino CLI bridge and xmake runner can stage and build that library; runtime
claims are listed only after board logs or hardware behavior have been observed.

## Script Entry

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Clean -ContinueOnError
```

The script returns `PASS`, `SKIP`, or `FAIL` per case. `SKIP` means the sketch
or local sketchbook library is not installed on this machine.

Useful focused runs:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Case arduinojson_runtime_smoke -Clean
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Case "arduinojson_runtime_smoke,ntpclient_report,pubsubclient_mqtts_ca_smoke,mqttclient_256dpi_smoke" -Clean -ContinueOnError
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Case "onewire_basic_compile,dallas_temperature_compile,u8g2_ssd1306_compile,adafruit_ssd1306_compile,rtclib_compile,arduino_httpclient_compile,arduino_mqttclient_compile" -Clean -ContinueOnError
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Case "arduino_httpclient_runtime_smoke,arduino_mqttclient_runtime_smoke" -Clean -ContinueOnError
```

## Current Matrix

| Case | Library | Level | Notes |
| --- | --- | --- | --- |
| `arduinojson_runtime_smoke` | `ArduinoJson` | Hardware-observed | Exercises `String`, `Print`, `F()`, JSON parse, mutation, and serialization without external hardware. |
| `ntpclient_report` | `NTPClient` | Hardware-observed | Uses `WiFiUDP` compatibility alias over the cellular UDP layer. |
| `pubsubclient_mqtts_ca_smoke` | `PubSubClient` | Hardware-observed | Uses `CellularClientSecure::setCACert()` and MQTTS loopback. |
| `mqttclient_256dpi_smoke` | `MQTT` by 256dpi | Hardware-observed | Uses plain `CellularClient` and MQTT loopback on `broker.emqx.io:1883`. |
| `onewire_basic_compile` | `OneWire` | Compile-only | Exercises object creation, search API, and GPIO timing helper calls. The bridge rewrites staged `OneWire.h` includes to avoid the CSDK header name collision. |
| `dallas_temperature_compile` | `DallasTemperature` + `OneWire` | Compile-only | Exercises the common DS18B20 dependency chain without requiring a sensor response. |
| `u8g2_ssd1306_compile` | `U8g2` | Compile-only | Exercises a common SSD1306 I2C constructor and font path. |
| `adafruit_ssd1306_compile` | `Adafruit SSD1306` + `Adafruit GFX Library` + `Adafruit BusIO` | Compile-only | Exercises the Adafruit display dependency chain. The runner exports `ARDUINO=10819` and AIR780EPM architecture macros at target level so library source units see the Arduino 1.x path. |
| `rtclib_compile` | `RTClib` + `Adafruit BusIO` | Compile-only | Exercises `DateTime`, `TimeSpan`, and common RTC wrapper types without touching hardware. |
| `arduino_httpclient_compile` | `ArduinoHttpClient` | Compile-only | Exercises an HTTP wrapper on top of `CellularClient`. |
| `arduino_httpclient_runtime_smoke` | `ArduinoHttpClient` | Hardware-observed | Runtime-verified on AIR780EPM through `example.com:80` GET, `STATUS_CODE,200`, non-empty body, and `+ARDUINO: ARDUINO_HTTPCLIENT,PASS`. |
| `arduino_mqttclient_compile` | `ArduinoMqttClient` | Compile-only | Exercises the official Arduino MQTT client wrapper on top of `CellularClient`. |
| `arduino_mqttclient_runtime_smoke` | `ArduinoMqttClient` | Hardware-observed | Runtime-verified on AIR780EPM through plain MQTT loopback on `broker.emqx.io:1883`, including connect, subscribe, publish, RX, and `+ARDUINO: ARDUINO_MQTTCLIENT,PASS`. |
| `sparkfun_scd4x_basic` | `SparkFun SCD4x Arduino Library` | Hardware-observed | Third-party I2C sensor example; previously observed CO2 data on AIR780EPM. |
| `sht40_basic` | `SHT40` | Hardware-observed | Third-party I2C sensor example; previously observed temperature and humidity output. |
| `sensirion_sht4x_example_usage` | `Sensirion I2C SHT4x` + `Sensirion Core` | Hardware-observed | Exercises a dependency chain on `Wire`. |

## Current Result

This pass verified the core `String` expansion against the third-party network
cases and the new ArduinoJson sketch:

```text
[library_compat] arduinojson_runtime_smoke            PASS  compile-plus-runtime-verified
[library_compat] ntpclient_report                     PASS  compile-plus-runtime-verified
[library_compat] pubsubclient_mqtts_ca_smoke          PASS  compile-plus-runtime-verified
[library_compat] mqttclient_256dpi_smoke              PASS  compile-plus-runtime-verified
[library_compat] onewire_basic_compile                PASS  compile-only
[library_compat] dallas_temperature_compile           PASS  compile-only
[library_compat] u8g2_ssd1306_compile                 PASS  compile-only
[library_compat] adafruit_ssd1306_compile             PASS  compile-only
[library_compat] rtclib_compile                       PASS  compile-only
[library_compat] arduino_httpclient_compile           PASS  compile-only
[library_compat] arduino_mqttclient_compile           PASS  compile-only
[library_compat] arduino_httpclient_runtime_smoke     PASS  compile-plus-runtime-verified
[library_compat] arduino_mqttclient_runtime_smoke     PASS  compile-plus-runtime-verified
```

`ArduinoJsonRuntimeSmoke` was also flashed to AIR780EPM on `COM3`; binary log
capture at `921600` observed repeated
`+ARDUINO: ARDUINOJSON_RUNTIME,PASS,LEN,62,...` lines.

The newly added compile-only cases were validated through
`scripts\validate_library_compat.ps1` on 2026-04-28. The dedicated
`ArduinoHttpClientRuntimeSmoke` and `ArduinoMqttClientSmoke` sketches were then
flashed to AIR780EPM on 2026-04-29, with binary logs decoded from `COM3` at
`921600` through `luatos-cli log view-binary --probe`.

## Policy

- Do not add third-party library cases to the default `smoke` regression unless
  the library is vendored or guaranteed to be installed.
- Use the library matrix to discover missing APIs, then patch the core API
  surface, not the third-party library.
- Runtime examples must not bake in product/business logic; they should only
  validate the Arduino compatibility contract.
