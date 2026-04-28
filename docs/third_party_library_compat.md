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
```

## Current Matrix

| Case | Library | Level | Notes |
| --- | --- | --- | --- |
| `arduinojson_runtime_smoke` | `ArduinoJson` | Hardware-observed | Exercises `String`, `Print`, `F()`, JSON parse, mutation, and serialization without external hardware. |
| `ntpclient_report` | `NTPClient` | Hardware-observed | Uses `WiFiUDP` compatibility alias over the cellular UDP layer. |
| `pubsubclient_mqtts_ca_smoke` | `PubSubClient` | Hardware-observed | Uses `CellularClientSecure::setCACert()` and MQTTS loopback. |
| `mqttclient_256dpi_smoke` | `MQTT` by 256dpi | Hardware-observed | Uses plain `CellularClient` and MQTT loopback on `broker.emqx.io:1883`. |
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
```

`ArduinoJsonRuntimeSmoke` was also flashed to AIR780EPM on `COM3`; binary log
capture at `921600` observed repeated
`+ARDUINO: ARDUINOJSON_RUNTIME,PASS,LEN,62,...` lines.

## Policy

- Do not add third-party library cases to the default `smoke` regression unless
  the library is vendored or guaranteed to be installed.
- Use the library matrix to discover missing APIs, then patch the core API
  surface, not the third-party library.
- Runtime examples must not bake in product/business logic; they should only
  validate the Arduino compatibility contract.
