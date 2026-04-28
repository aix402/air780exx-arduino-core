# AIR780EPM Pin Map

## Public Pin Rule

Arduino digital pins use GPIO numbers.

```cpp
pinMode(27, OUTPUT);
digitalWrite(27, HIGH);
```

The example above operates `GPIO27`, not module physical `PIN27`.

Module physical pin numbers remain the hardware reference used internally for
IOMUX decisions and documentation. This keeps Arduino sketches and third-party
libraries compatible while still matching the OpenLuat hardware table.

## Current Validation Status

| Item | Status |
| --- | --- |
| GPIO-number API contract | Compile-only |
| `A0..A3` logical ADC contract | Compile-enabled |
| `A0..A3` module-pin mapping | Pin-table confirmed |
| `A1 -> ADC1` data path | Hardware-observed |
| `LED_BUILTIN = GPIO27` | Hardware-observed before this pinmap cleanup |
| CSDK default digital GPIO route | Compile-only |
| Pull-up / pull-down behavior | Pending hardware validation |

## Default Digital GPIO Routes

Arduino digital GPIO follows the CSDK default route returned by
`luat_pin_get_iomux_info(LUAT_MCU_PERIPHERAL_GPIO, gpio, ...)`. The Arduino Core
does not silently remap duplicate GPIO resources.

| Arduino pin | GPIO | Default module pin | UID/PAD | Alt | Notes |
| --- | --- | --- | --- | --- | --- |
| `16` | GPIO16 | PIN97 | 11 | 4 | CSDK `prv_gpio_iomux` default |
| `17` | GPIO17 | PIN100 | 12 | 4 | CSDK `prv_gpio_iomux` default |
| `18` | GPIO18 | PIN17 | 33 | 0 | Conflicts with UART1_RXD |
| `19` | GPIO19 | PIN18 | 34 | 0 | Conflicts with UART1_TXD |

Other GPIO numbers currently use the CSDK default GPIO-to-PAD mapping.

GPIO18/GPIO19 also exist on module PIN67/PIN66 as alternate routes. Those pins
are the LuatOS-recommended I2C1 SCL/SDA route, but they are not the CSDK default
for plain digital GPIO. A future explicit remap API should be used when a sketch
needs GPIO18/GPIO19 on PIN67/PIN66 instead of the UART1 pins.

`GPIO0` maps to the `USB_BOOT` hardware function in the AIR780EPM table. It is
kept numerically visible for Arduino compatibility, but it is sensitive and not
part of the first hardware-verified GPIO set.

## Analog Inputs

The first AIR780EPM ADC contract is intentionally separate from GPIO numbering.
Use logical analog aliases:

```cpp
int raw = analogRead(A0);
uint32_t mv = analogReadMilliVolts(A0);
```

Do not assume `analogRead(0)` means ADC channel 0. In this core, `0` still
means `GPIO0` for digital numbering.

| Arduino alias | ADC channel | Current route contract | Status | Notes |
| --- | --- | --- | --- | --- |
| `A0` / `PIN_ADC0` | 0 | LuatOS ADC ID 0 / module PIN9 | Compile-enabled | Module pin mapping confirmed from vendor pin table; runtime voltage validation pending |
| `A1` / `PIN_ADC1` | 1 | LuatOS ADC ID 1 / module PIN96 | Hardware-observed | Flashed and log-verified through `AnalogReadReport`; observed stable readings around `2273-2281 mV` on the `A1 -> ADC1 -> PIN96` path |
| `A2` / `PIN_ADC2` | 2 | LuatOS ADC ID 2 / module PIN77 | Compile-enabled | Module pin mapping confirmed from vendor pin table; runtime voltage validation pending |
| `A3` / `PIN_ADC3` | 3 | LuatOS ADC ID 3 / module PIN76 | Compile-enabled | Module pin mapping confirmed from vendor pin table; runtime voltage validation pending |

Current ADC behavior:

- `analogRead()` defaults to a 12-bit returned value.
- `analogReadResolution(bits)` supports `1..16` returned bits.
- `analogReadMilliVolts()` converts the LuatOS calibrated microvolt result to
  integer millivolts.
- The first batch uses the LuatOS wide range by default.
- Public per-channel range selection is not exposed yet.

AIR780EPM LuatOS reference material indicates:

- 4 general ADC channels, `0..3`
- a narrow range around `0-1.5V`
- a wide range around `0-3.3V`

The current Arduino contract now publishes the module physical pin mapping from
the vendor pin table:

- `ADC0 / A0 -> PIN9`
- `ADC1 / A1 -> PIN96`
- `ADC2 / A2 -> PIN77`
- `ADC3 / A3 -> PIN76`

This is intentionally still separate from GPIO numbering. The core does not yet
promise `analogRead(GPIOx)` behavior.

## Board Aliases

| Arduino alias | GPIO number | Peripheral route | Plain digital route |
| --- | --- | --- | --- |
| `LED_BUILTIN` | GPIO27 | PIN16 | PIN16 |
| `SCL` | GPIO14 | PIN58 / I2C0_SCL | PIN58 |
| `SDA` | GPIO15 | PIN57 / I2C0_SDA | PIN57 |
| `SS` | GPIO8 | PIN83 / SPI0_CS | PIN83 |
| `MOSI` | GPIO9 | PIN85 / SPI0_MOSI | PIN85 |
| `MISO` | GPIO10 | PIN84 / SPI0_MISO | PIN84 |
| `SCK` | GPIO11 | PIN86 / SPI0_CLK | PIN86 |

## Default Peripheral Routes

| Arduino object | Hardware peripheral | Default pins |
| --- | --- | --- |
| `Serial` | USB/log channel | USB enumerated serial |
| `Serial1` | UART1 | RX GPIO18/PIN17, TX GPIO19/PIN18 |
| `Serial2` | UART2 | RX GPIO12/PIN28, TX GPIO13/PIN29 |
| `Serial3` | UART3 | RX GPIO14/PIN58, TX GPIO15/PIN57 |
| `Wire` | I2C0 | SCL GPIO14/PIN58, SDA GPIO15/PIN57 |
| `Wire1` | I2C1 | CSDK default I2C1 route |
| `SPI` | SPI0 | CS GPIO8/PIN83, MOSI GPIO9/PIN85, MISO GPIO10/PIN84, SCK GPIO11/PIN86 |
| `SPI1` | SPI1 | CSDK default SPI1 route |
| `PIN_PWM0` | PWM0 | GPIO1/PIN22 |
| `PIN_PWM1` | PWM1 | GPIO24/PIN20 |
| `PIN_PWM2` | PWM2 | GPIO31/PIN32 |
| `PIN_PWM4` | PWM4 | GPIO33/PIN26 |

`Serial1`, `Serial2`, and `Serial3` are available through LuatOS UART IDs
1, 2, and 3. `Serial1` TX/RX has been hardware-observed on its default
GPIO18/GPIO19 route. `Serial2` and `Serial3` are still pending hardware
validation.

`Wire` has been hardware-observed on I2C0 GPIO14/GPIO15 with both an SHT40
sensor and the SparkFun SCD4x third-party library example. `Wire1`, `SPI`, and
`SPI1` are compile-enabled through LuatOS bus IDs, but hardware validation is
still pending. `Wire.begin(sda, scl)` and `SPI.begin(sck, miso, mosi, ss)`
currently accept custom pin arguments for Arduino source compatibility, but
they do not implement custom hardware remap yet. Use CSDK default routes until
remap is designed and validated.

PWM is available through LuatOS PWM channels 0, 1, 2, and 4. Channel 3 is
reserved by the CSDK and is not exposed. `analogWrite(pin, value)` accepts the
GPIO-numbered PWM aliases above. `PIN_PWM4/GPIO33/PIN26` has been
hardware-observed with visible LED brightness changes. Calling `pinMode()` or
`digitalWrite()` on the same pin closes the active PWM channel and returns the
pin to digital GPIO.

## Board-Specific Display Bring-Up

AIR780EPM dev board v1.2 routes the accessory ST7796 LCD through the dedicated
hardware LCD interface, not through the current Arduino `SPI` abstraction.
The in-repo probe sketch uses:

| Item | Route |
| --- | --- |
| LCD data path | `LUAT_LCD_HW_ID_0` |
| LCD power enable | `GPIO28` |
| LCD reset | `GPIO36` |
| LCD clock/data/cs/dc | Dedicated LCD pads managed by `luat_lcd_IF_init()` |

The current Arduino `SPI.begin(sck, miso, mosi, ss)` compatibility layer does
not remap the LuatOS SPI driver onto the ST7796 accessory-board route. Use
`examples\05.Display\St7796HwIfProbe` for board validation until a real Arduino
display abstraction is designed.

## Not First-Batch Arduino Digital Pins

`PIN62`, `PIN63`, and `PIN64` are not independent first-batch Arduino digital
pins. The hardware table marks them as the same main-chip pads as `PIN81`,
`PIN80`, and `PIN55` respectively, and the CSDK `air780epx` table does not expose
them as independent `num` entries.

`PWR_KEY`, `VBUS/WAKEUP1`, `USIM_DET/WAKEUP2`, and `WAKEUP0` are tracked as
wakeup or power-control resources, not ordinary Arduino GPIO resources.

## Pending Hardware Validation

- Confirm `pinMode(27, OUTPUT)` still blinks the dev-board LED.
- Confirm `A0`, `A2`, and `A3` against known voltages on module `PIN9`,
  `PIN77`, and `PIN76`.
- Confirm `analogReadMilliVolts()` accuracy on at least one low-voltage and one
  near-full-scale input.
- Confirm `pinMode(18, OUTPUT)` follows the CSDK default GPIO18 route on PIN17.
- Confirm `pinMode(19, OUTPUT)` follows the CSDK default GPIO19 route on PIN18.
- Confirm `INPUT_PULLUP` and `INPUT_PULLDOWN` on at least GPIO18/GPIO19.
- Confirm GPIO16/GPIO17 route to PIN97/PIN100.
- Confirm `Serial2` and `Serial3` TX/RX on their default routes.
- Confirm `Wire1` on its default I2C1 route.
- Confirm `analogWrite(PIN_PWM2, 128)` outputs PWM2 on GPIO31/PIN32.
- Confirm `analogWriteFrequency()` changes measured PWM frequency.
- Design and validate an explicit remap API for GPIO18/GPIO19 on PIN67/PIN66.
- Confirm whether GPIO0/USB_BOOT should be blocked, warned, or left as a
  sensitive compile-visible pin.
