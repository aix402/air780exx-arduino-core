#include "Arduino.h"

#include "arduino_pin_io.h"

void pinMode(uint8_t pin, uint8_t mode) {
    arduino_pin_mode(pin, mode);
}

void digitalWrite(uint8_t pin, uint8_t val) {
    arduino_pin_write(pin, val);
}

int digitalRead(uint8_t pin) {
    return arduino_pin_read(pin) ? HIGH : LOW;
}
