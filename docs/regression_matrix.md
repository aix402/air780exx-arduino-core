# AIR780EPM Regression Matrix

This document records the minimum sketch matrix that should be rerun as the
core grows. The immediate goal is a small, repeatable gate that matches the
current xmake-native Arduino bridge.

## Profile Summary

| Profile | Goal | Current automation target |
| --- | --- | --- |
| `smoke` | Keep the basic build and runtime path intact | compile gate first, optional upload/log gate |
| `pinmap_contract` | Freeze public pin aliases and resource-boundary output | compile gate first, optional runtime report |
| `sensor_io` | Keep real board I/O examples working | compile gate first, selective hardware rerun |
| `connectivity` | Keep modem and socket-facing API bring-up stable | compile gate first, then SIM-on-board rerun |
| `storage` | Keep NVM and LittleFS surfaces stable | compile gate first, then reboot/persistence rerun |

## Sketch Matrix

| Sketch | Path | Level target | Notes |
| --- | --- | --- | --- |
| Blink | `examples\01.Basics\Blink` | `L3` | Base ready log and LED loop |
| Core API P0 compile | `examples\00.Core\CoreApiP0Compile` | `L2` | P0 compatibility surface |
| Serial API compile | `examples\02.Serial\SerialApiCompile` | `L2` | `Print` / `Stream` / `String` shape |
| Bus API P2 compile | `examples\03.Bus\BusApiP2Compile` | `L2` | `Wire`, `Wire1`, `SPI`, `SPI1` shapes |
| PWM API compile | `examples\04.PWM\PwmApiCompile` | `L2` | `analogWrite()` and frequency helpers |
| UART1 probe | `examples\02.Serial\Uart1Probe` | `L4` | External TX/RX observation |
| SHT40 `Wire` probe | `examples\03.Bus\SHT40Wire` | `L4` | I2C0 hardware-observed |
| PWM LED probe | `examples\04.PWM\PwmLedProbe33` | `L4` | Visible LED brightness change |
| Servo pulse report | `examples\07.Servo\ServoPulseReport` | `L2` -> `L4` | First servo-on-PWM validation path |
| LCD HWIF probe | `examples\05.Display\St7796HwIfProbe` | `L4` | AIR780EPM dev board v1.2 only |
| Pin report | `validation_sketches\PinReport` | `L2` -> `L4` | Public alias report |
| Pin capabilities | `validation_sketches\PinCapabilities` | `L2` -> `L4` | PWM / overlap / sensitivity report |
| Resource boundary report | `validation_sketches\ResourceBoundaryP4Report` | `L2` -> `L4` | `Verified / Available / Sensitive` report |
| Analog read report | `examples\06.Analog\AnalogReadReport` | `L2` | First logical `A0..A3` ADC contract; `A1/PIN96` is hardware-observed, `A0/PIN9`, `A2/PIN77`, and `A3/PIN76` still need runtime validation |
| Network API P1 report | `validation_sketches\NetworkApiP1Report` | `L2` | Compile gate for `IPAddress` / `Client` / UDP / modem-facing API surface |
| Network status report | `examples\08.Network\NetworkStatusReport` | `L4` | SIM / registration / PDP state smoke is hardware-observed |
| Modem info report | `examples\08.Network\ModemInfoReport` | `L4` | Hardware runtime summary now shows `WAIT_OK=1`, `REGISTERED=1`, `NET_READY=1`, `HAS_IPV4=1`, and a real IPv4, which proves the sketch completed its wait/status path and entered `loop()` |
| TCP HTTP GET | `examples\08.Network\TcpHttpGet` | `L4` | Hardware runtime now reaches `HTTP/1.1 200 OK` over plain TCP |
| TLS HTTP GET | `examples\08.Network\TlsHttpGet` | `L4` | Hardware runtime now reaches `HTTP/1.1 200 OK` over TLS with `TLSERR=0` |
| MQTTS PubSubClient CA smoke | `validation_sketches\MqttsPubSubClientCaSmoke` | `L4` | Third-party `PubSubClient` over `CellularClientSecure::setCACert()` is hardware-observed through connect / subscribe / publish / RX loopback |
| MQTT 256dpi smoke | `validation_sketches\Mqtt256dpiSmoke` | `L4` | Third-party `MQTT` by 256dpi over plain `CellularClient` is hardware-observed through connect / subscribe / publish / RX loopback on `broker.emqx.io:1883` |
| UDP NTP report | `examples\08.Network\UdpNtpReport` | `L4` | Hardware runtime now receives a 48-byte NTP packet and valid epoch |
| NTPClient report | `validation_sketches\NTPClientReport` | `L4` | Third-party `NTPClient` over the `WiFiUDP` compatibility alias is hardware-observed through update, valid epoch, and `PASS` |
| Network time report | `examples\08.Network\NetworkTimeReport` | `L4` | Hardware runtime now reaches a valid epoch plus formatted local time through `configTime()` / `getLocalTime()` |
| EEPROM / Preferences report | `examples\09.NVM\EepromPreferencesReport` | `L4` -> `L5` | Reflash-persistent counter and key-value smoke is hardware-observed; pure reset / power-cycle still pending |
| LittleFS report | `examples\10.FileSystem\LittleFSReport` | `L4` | File create / read / rename / list / cleanup smoke is hardware-observed |

## Current Automation Contract

The minimal runner script is `scripts\run_regression_matrix.ps1`.

Initial automation scope:

- always support compile-only regression
- optionally support upload plus log verification when hardware is connected
- keep sketch selection explicit and small
- fail fast by default

## Recommended Profile Contents

### `smoke`

- `examples\01.Basics\Blink`
- `examples\00.Core\CoreApiP0Compile`
- `examples\02.Serial\SerialApiCompile`
- `examples\03.Bus\BusApiP2Compile`
- `examples\04.PWM\PwmApiCompile`

### `pinmap_contract`

- `validation_sketches\PinReport`
- `validation_sketches\PinCapabilities`
- `validation_sketches\ResourceBoundaryP4Report`

### `sensor_io`

- `examples\03.Bus\SHT40Wire`
- `examples\06.Analog\AnalogReadReport`

### `connectivity`

- `validation_sketches\NetworkApiP1Report`
- `examples\08.Network\NetworkStatusReport`
- `examples\08.Network\ModemInfoReport`
- `examples\08.Network\TcpHttpGet`
- `examples\08.Network\TlsHttpGet`
- `validation_sketches\MqttsPubSubClientCaSmoke`
- `validation_sketches\Mqtt256dpiSmoke`
- `examples\08.Network\UdpNtpReport`
- `validation_sketches\NTPClientReport`
- `examples\08.Network\NetworkTimeReport`

### `storage`

- `examples\09.NVM\EepromPreferencesReport`
- `examples\10.FileSystem\LittleFSReport`

## Hardware Rerun Rules

- Any change to pin aliases, variant files, or bus routing must rerun
  `pinmap_contract`.
- Any change to `HardwareSerial`, `Wire`, or `SPI` must rerun `smoke` and the
  relevant hardware sketch when the board is connected.
- Any change to `wiring_analog.cpp` must rerun `smoke` and the ADC report once
  it exists.
- Any change to the PWM ownership model or `Servo.cpp` must rerun `smoke` and
  `examples\07.Servo\ServoPulseReport`.
- Any change to modem, TCP, TLS, MQTT, or UDP runner code must rerun
  `connectivity`.
- Any change to `EEPROM`, `Preferences`, `FS`, `LittleFS`, or the AIR780EPM
  runner storage adapter must rerun `storage`.
- Any linker, runner, or upload-script change must rerun `Blink` upload/log
  verification before broader claims are updated.
