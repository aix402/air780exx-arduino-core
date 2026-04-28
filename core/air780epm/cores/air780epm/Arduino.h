#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "pins_arduino.h"

#ifndef ARDUINO
#define ARDUINO 10819
#endif

#ifndef ARDUINO_ARCH_EC718PM
#define ARDUINO_ARCH_EC718PM 1
#endif

#ifndef ARDUINO_ARCH_AIR780EPM
#define ARDUINO_ARCH_AIR780EPM 1
#endif

#define PI         3.1415926535897932384626433832795
#define HALF_PI    1.5707963267948966192313216916398
#define TWO_PI     6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER      2.718281828459045235360287471352

#define LSBFIRST 0
#define MSBFIRST 1

#define RISING  0x01
#define FALLING 0x02
#define CHANGE  0x03
#define ONLOW   0x04
#define ONHIGH  0x05

#define DEFAULT  1

#ifdef __cplusplus
extern "C" {
#endif

#define HIGH 0x1
#define LOW  0x0

#define INPUT        0x0
#define OUTPUT       0x1
#define INPUT_PULLUP 0x2
#define INPUT_PULLDOWN 0x3

#define LED_BUILTIN AIR780EPM_LED_BUILTIN

#define NOT_A_PIN        -1
#define NOT_A_PORT       -1
#define NOT_AN_INTERRUPT -1
#define NOT_ON_TIMER     0
#define TIMER_PWM0       1
#define TIMER_PWM1       2
#define TIMER_PWM2       3
#define TIMER_PWM4       4

#define digitalPinToInterrupt(pin) ((pin) < NUM_DIGITAL_PINS ? (pin) : NOT_AN_INTERRUPT)
#define digitalPinToTimer(pin) (((pin) == PIN_PWM0) ? TIMER_PWM0 : (((pin) == PIN_PWM1) ? TIMER_PWM1 : (((pin) == PIN_PWM2) ? TIMER_PWM2 : (((pin) == PIN_PWM4) ? TIMER_PWM4 : NOT_ON_TIMER))))
#define digitalPinHasPWM(pin) (digitalPinToTimer(pin) != NOT_ON_TIMER)
#define analogInputToDigitalPin(pin) (NOT_A_PIN)
#define digitalPinToAnalogInput(pin) (((pin) >= A0 && (pin) < (A0 + NUM_ANALOG_INPUTS)) ? ((pin) - A0) : -1)
#define digitalPinToPort(pin) (0)
#define digitalPinToBitMask(pin) (1UL << (pin))
#define portOutputRegister(port) ((volatile uint32_t *)0)
#define portInputRegister(port) ((volatile uint32_t *)0)
#define portModeRegister(port) ((volatile uint32_t *)0)

typedef uint8_t byte;
typedef bool boolean;
typedef unsigned int word;
typedef uint8_t pin_size_t;
typedef void (*voidFuncPtr)(void);
typedef void (*voidFuncPtrArg)(void *);

#include "pgmspace.h"

#ifdef __cplusplus
}

class __FlashStringHelper;

#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(str) (reinterpret_cast<const __FlashStringHelper *>(PSTR(str)))

extern "C" {
#else
#define F(str) (str)
#endif

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define radians(deg) ((deg) * DEG_TO_RAD)
#define degrees(rad) ((rad) * RAD_TO_DEG)
#define sq(x) ((x) * (x))

#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
#define bit(b) (1UL << (b))
#define _BV(b) (1UL << (b))

static inline void arduinoCoreEnableInterrupts(void) {
    __asm volatile("cpsie i" ::: "memory");
}

static inline void arduinoCoreDisableInterrupts(void) {
    __asm volatile("cpsid i" ::: "memory");
}

#define interrupts() arduinoCoreEnableInterrupts()
#define noInterrupts() arduinoCoreDisableInterrupts()
#define sei() interrupts()
#define cli() noInterrupts()

void pinMode(pin_size_t pin, uint8_t mode);
void digitalWrite(pin_size_t pin, uint8_t val);
int digitalRead(pin_size_t pin);

void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
unsigned long millis(void);
unsigned long micros(void);
void yield(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);
int analogRead(pin_size_t pin);
uint32_t analogReadMilliVolts(pin_size_t pin);
void analogReadResolution(int bits);
void analogWrite(pin_size_t pin, int value);
void analogWriteResolution(int bits);

#ifdef __cplusplus
}

#include "HardwareSerial.h"
#include "Print.h"
#include "SPI.h"
#include "Stream.h"
#include "WCharacter.h"
#include "WString.h"
#include "Wire.h"
#include "IPAddress.h"
#include "Client.h"
#include "Udp.h"
#include "AIR780EPMModem.h"
#include "AIR780EPMClient.h"
#include "AIR780EPMTLSClient.h"
#include "AIR780EPMUDP.h"
#include "WiFiClient.h"
#include "WiFiClientSecure.h"
#include "WiFiUdp.h"
#include "FS.h"
#include "LittleFS.h"
#include "EEPROM.h"
#include "Preferences.h"
#include "Servo.h"

template <typename T, typename U>
inline auto min(T a, U b) -> decltype((a < b) ? a : b) {
    return (a < b) ? a : b;
}

template <typename T, typename U>
inline auto max(T a, U b) -> decltype((a > b) ? a : b) {
    return (a > b) ? a : b;
}

void randomSeed(unsigned long seed);
long random(long howbig);
long random(long howsmall, long howbig);
extern "C" bool getLocalTime(struct tm *info, uint32_t ms = 5000UL);
extern "C" void configTime(long gmtOffset_sec,
                           int daylightOffset_sec,
                           const char *server1,
                           const char *server2 = nullptr,
                           const char *server3 = nullptr);
extern "C" void configTzTime(const char *tz,
                             const char *server1,
                             const char *server2 = nullptr,
                             const char *server3 = nullptr);

uint16_t makeWord(uint16_t value);
uint16_t makeWord(uint8_t high, uint8_t low);

#define word(...) makeWord(__VA_ARGS__)

bool analogWriteFrequency(pin_size_t pin, uint32_t frequency);
bool analogWriteFrequency(uint32_t frequency);
void analogWriteFreq(uint32_t frequency);

void setup(void);
void loop(void);
#endif
