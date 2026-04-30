#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ARDUINO_PIN_IO_LOW = 0,
    ARDUINO_PIN_IO_HIGH = 1,
};

enum {
    ARDUINO_PIN_IO_INPUT = 0,
    ARDUINO_PIN_IO_OUTPUT = 1,
    ARDUINO_PIN_IO_INPUT_PULLUP = 2,
    ARDUINO_PIN_IO_INPUT_PULLDOWN = 3,
};

void arduino_pin_mode(uint8_t pin, uint8_t mode);
void arduino_pin_write(uint8_t pin, uint8_t value);
int arduino_pin_read(uint8_t pin);

void arduino_pwm_detach_pin(uint8_t pin);
void arduino_analog_write(uint8_t pin, int value);
void arduino_analog_write_resolution(int bits);
bool arduino_analog_write_frequency_pin(uint8_t pin, uint32_t frequency);
bool arduino_analog_write_frequency_all(uint32_t frequency);
int arduino_analog_read(uint8_t pin);
uint32_t arduino_analog_read_millivolts(uint8_t pin);
void arduino_analog_read_resolution(int bits);

int arduino_servo_attach(uint32_t pin, uint32_t pulse_us, uint32_t period_us);
int arduino_servo_attached(uint32_t pin);
void arduino_servo_detach_pin(uint32_t pin);
int arduino_servo_write_microseconds(uint32_t pin, uint32_t pulse_us, uint32_t period_us);

#ifdef __cplusplus
}
#endif
