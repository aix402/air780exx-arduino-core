# AIR780EPM Resource Catalog

This document records the current board-resource contract in three buckets:

- `Verified`: hardware-observed on AIR780EPM
- `Available`: compile-enabled or structurally wired, but not yet hardware-observed
- `Sensitive`: visible to the porting effort, but not promised as ordinary
  Arduino resources

## Verified

| Resource | Arduino surface | Default route | Status | Notes |
| --- | --- | --- | --- | --- |
| USB log channel | `Serial` | USB enumerated port | Hardware-observed | Primary log path for bring-up and validation |
| Built-in LED | `LED_BUILTIN` | GPIO27 / module PIN16 | Hardware-observed | Used by `Blink` |
| UART1 | `Serial1` | RX GPIO18 / PIN17, TX GPIO19 / PIN18 | Hardware-observed | Verified with external USB-UART on `COM8` |
| I2C0 | `Wire` | SCL GPIO14 / PIN58, SDA GPIO15 / PIN57 | Hardware-observed | Verified with SHT40 and SparkFun SCD4x |
| PWM4 | `analogWrite(PIN_PWM4, ...)` | GPIO33 / PIN26 | Hardware-observed | Visible LED brightness changes observed |
| ADC1 | `A1` | LuatOS ADC ID 1 / module PIN96 | Hardware-observed | Flashed and log-verified through `AnalogReadReport`; observed stable readings around `2273-2281 mV` |
| LCD HWIF | `AIR780EPM_LuatOS.h` probe only | `LUAT_LCD_HW_ID_0` | Hardware-observed | AIR780EPM dev board v1.2, power GPIO28, reset GPIO36 |

## Available

| Resource | Arduino surface | Default route | Status | Notes |
| --- | --- | --- | --- | --- |
| UART2 | `Serial2` | RX GPIO12 / PIN28, TX GPIO13 / PIN29 | Compile-enabled | Runtime validation pending |
| UART3 | `Serial3` | RX GPIO14 / PIN58, TX GPIO15 / PIN57 | Compile-enabled | Shares route pressure with `Wire` |
| I2C1 | `Wire1` | LuatOS I2C1 default route | Compile-enabled | Route details are documented, runtime validation pending |
| SPI0 | `SPI` | SCK GPIO11, MISO GPIO10, MOSI GPIO9, SS GPIO8 | Compile-enabled | Runtime validation pending |
| SPI1 | `SPI1` | LuatOS SPI1 default route | Compile-enabled | Runtime validation pending |
| PWM0 | `PIN_PWM0` | GPIO1 / PIN22 | Compile-enabled | Runtime validation pending |
| PWM1 | `PIN_PWM1` | GPIO24 / PIN20 | Compile-enabled | Runtime validation pending |
| PWM2 | `PIN_PWM2` | GPIO31 / PIN32 | Compile-enabled | Runtime validation pending |
| ADC0 | `A0` | LuatOS ADC ID 0 / module PIN9 | Compile-enabled | Module pin mapping confirmed; runtime voltage validation pending |
| ADC2 | `A2` | LuatOS ADC ID 2 / module PIN77 | Compile-enabled | Module pin mapping confirmed; runtime voltage validation pending |
| ADC3 | `A3` | LuatOS ADC ID 3 / module PIN76 | Compile-enabled | Module pin mapping confirmed; runtime voltage validation pending |
| CPU temperature ADC | future board-specific API | internal channel | Not exposed yet | Keep out of first-batch Arduino contract |
| VBAT ADC | future board-specific API | internal channel | Not exposed yet | Keep out of first-batch Arduino contract |

## Sensitive

| Resource | Why sensitive | Current handling |
| --- | --- | --- |
| GPIO0 / `USB_BOOT` | Boot/download behavior risk | Kept numerically visible, not first-batch hardware-verified |
| GPIO18 / GPIO19 | Default digital route overlaps UART1 pins; alternate route overlaps I2C1 | Follow CSDK default route and document overlap |
| GPIO14 / GPIO15 | Shared by `Wire` and `Serial3` default routes | Documented overlap, no automatic remap |
| GPIO28 / GPIO36 | Current LCD accessory-board control path | Board-specific probe only, not generic display API |
| Physical `PIN62` / `PIN63` / `PIN64` | Duplicate pad exposure, not independent first-batch digital pins | Not published as separate Arduino pins |
| `PWR_KEY`, `VBUS`, `WAKEUP0`, `WAKEUP1`, `WAKEUP2`, `USIM_DET` | Power or wake semantics, not ordinary GPIO | Track as board/power resources, not generic digital pins |
| Download / log serial topology | Flash and logs share USB-enumerated resources | Prefer CLI-driven USB flow; do not hard-code generic UART0 assumptions |

## Route Rules

1. Arduino digital pin numbers use GPIO numbers.
2. First-batch Arduino resources follow the CSDK default route unless an
   explicit remap API is later designed and validated.
3. Resource documentation must separate `compile-enabled` from
   `hardware-observed`.
4. A resource can be visible in validation sketches before it is promised as a
   general Arduino API.

## Current Follow-Up List

- Complete runtime voltage validation for `A0`, `A2`, and `A3` on module
  `PIN9`, `PIN77`, and `PIN76`.
- Validate `Wire1`, `SPI`, `SPI1`, `Serial2`, and `Serial3`.
- Confirm which sensitive pins should stay compile-visible and which should
  eventually warn or reject at runtime.
- Reduce the temporary board configuration seeded from the existing validated
  product project into an AIR780EPM-specific release contract.
