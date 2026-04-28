#pragma once

#include <stdint.h>

// AIR780EPM Arduino digital pins use GPIO numbers, not module physical pin
// numbers. Plain digital GPIO follows the CSDK default IOMUX route.
#define AIR780EPM_LED_BUILTIN 27

static const uint8_t D0 = 0;
static const uint8_t D1 = 1;
static const uint8_t D2 = 2;
static const uint8_t D3 = 3;
static const uint8_t D4 = 4;
static const uint8_t D5 = 5;
static const uint8_t D6 = 6;
static const uint8_t D7 = 7;
static const uint8_t D8 = 8;
static const uint8_t D9 = 9;
static const uint8_t D10 = 10;
static const uint8_t D11 = 11;
static const uint8_t D12 = 12;
static const uint8_t D13 = 13;
static const uint8_t D14 = 14;
static const uint8_t D15 = 15;
static const uint8_t D16 = 16;
static const uint8_t D17 = 17;
static const uint8_t D18 = 18;
static const uint8_t D19 = 19;
static const uint8_t D20 = 20;
static const uint8_t D21 = 21;
static const uint8_t D22 = 22;
static const uint8_t D23 = 23;
static const uint8_t D24 = 24;
static const uint8_t D25 = 25;
static const uint8_t D26 = 26;
static const uint8_t D27 = 27;
static const uint8_t D28 = 28;
static const uint8_t D29 = 29;
static const uint8_t D30 = 30;
static const uint8_t D31 = 31;
static const uint8_t D32 = 32;
static const uint8_t D33 = 33;
static const uint8_t D34 = 34;
static const uint8_t D35 = 35;
static const uint8_t D36 = 36;
static const uint8_t D37 = 37;
static const uint8_t D38 = 38;

static const uint8_t PIN_GPIO0 = 0;
static const uint8_t PIN_GPIO1 = 1;
static const uint8_t PIN_GPIO2 = 2;
static const uint8_t PIN_GPIO3 = 3;
static const uint8_t PIN_GPIO4 = 4;
static const uint8_t PIN_GPIO5 = 5;
static const uint8_t PIN_GPIO6 = 6;
static const uint8_t PIN_GPIO7 = 7;
static const uint8_t PIN_GPIO8 = 8;
static const uint8_t PIN_GPIO9 = 9;
static const uint8_t PIN_GPIO10 = 10;
static const uint8_t PIN_GPIO11 = 11;
static const uint8_t PIN_GPIO12 = 12;
static const uint8_t PIN_GPIO13 = 13;
static const uint8_t PIN_GPIO14 = 14;
static const uint8_t PIN_GPIO15 = 15;
static const uint8_t PIN_GPIO16 = 16;
static const uint8_t PIN_GPIO17 = 17;
static const uint8_t PIN_GPIO18 = 18;
static const uint8_t PIN_GPIO19 = 19;
static const uint8_t PIN_GPIO20 = 20;
static const uint8_t PIN_GPIO21 = 21;
static const uint8_t PIN_GPIO22 = 22;
static const uint8_t PIN_GPIO23 = 23;
static const uint8_t PIN_GPIO24 = 24;
static const uint8_t PIN_GPIO25 = 25;
static const uint8_t PIN_GPIO26 = 26;
static const uint8_t PIN_GPIO27 = 27;
static const uint8_t PIN_GPIO28 = 28;
static const uint8_t PIN_GPIO29 = 29;
static const uint8_t PIN_GPIO30 = 30;
static const uint8_t PIN_GPIO31 = 31;
static const uint8_t PIN_GPIO32 = 32;
static const uint8_t PIN_GPIO33 = 33;
static const uint8_t PIN_GPIO34 = 34;
static const uint8_t PIN_GPIO35 = 35;
static const uint8_t PIN_GPIO36 = 36;
static const uint8_t PIN_GPIO37 = 37;
static const uint8_t PIN_GPIO38 = 38;

static const uint8_t SDA = PIN_GPIO15;
static const uint8_t SCL = PIN_GPIO14;
static const uint8_t PIN_WIRE_SDA = SDA;
static const uint8_t PIN_WIRE_SCL = SCL;
static const uint8_t PIN_WIRE1_SDA = PIN_GPIO19;
static const uint8_t PIN_WIRE1_SCL = PIN_GPIO18;

static const uint8_t SS = PIN_GPIO8;
static const uint8_t MOSI = PIN_GPIO9;
static const uint8_t MISO = PIN_GPIO10;
static const uint8_t SCK = PIN_GPIO11;
static const uint8_t PIN_SPI_SS = SS;
static const uint8_t PIN_SPI_MOSI = MOSI;
static const uint8_t PIN_SPI_MISO = MISO;
static const uint8_t PIN_SPI_SCK = SCK;

static const uint8_t PIN_PWM0 = PIN_GPIO1;
static const uint8_t PIN_PWM1 = PIN_GPIO24;
static const uint8_t PIN_PWM2 = PIN_GPIO31;
static const uint8_t PIN_PWM4 = PIN_GPIO33;

static const uint8_t PIN_UART1_RX = PIN_GPIO18;
static const uint8_t PIN_UART1_TX = PIN_GPIO19;
static const uint8_t PIN_UART2_RX = PIN_GPIO12;
static const uint8_t PIN_UART2_TX = PIN_GPIO13;
static const uint8_t PIN_UART3_RX = PIN_GPIO14;
static const uint8_t PIN_UART3_TX = PIN_GPIO15;

// AIR780EPM ADC is exposed as logical Arduino analog channels first. These are
// not GPIO numbers and are intentionally kept separate from digital pin tokens.
// Current module physical pin mapping is:
//   A0 -> ADC0 -> PIN9
//   A1 -> ADC1 -> PIN96
//   A2 -> ADC2 -> PIN77
//   A3 -> ADC3 -> PIN76
static const uint8_t PIN_ADC0 = 39;
static const uint8_t PIN_ADC1 = 40;
static const uint8_t PIN_ADC2 = 41;
static const uint8_t PIN_ADC3 = 42;

static const uint8_t A0 = PIN_ADC0;
static const uint8_t A1 = PIN_ADC1;
static const uint8_t A2 = PIN_ADC2;
static const uint8_t A3 = PIN_ADC3;

#define NUM_DIGITAL_PINS 39
#define NUM_ANALOG_INPUTS 4
