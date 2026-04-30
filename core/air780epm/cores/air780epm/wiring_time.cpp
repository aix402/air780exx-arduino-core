#include "Arduino.h"

extern "C" {
#include "arduino_time_io.h"
}

void delay(unsigned long ms) {
    arduinoCoreDelayMs(static_cast<uint32_t>(ms));
}

void delayMicroseconds(unsigned int us) {
    arduinoCoreDelayUs(static_cast<uint32_t>(us));
}

unsigned long millis(void) {
    return static_cast<unsigned long>(arduinoCoreMillis());
}

unsigned long micros(void) {
    return static_cast<unsigned long>(arduinoCoreMillis() * 1000ULL);
}

void yield(void) {
    arduinoCoreYield();
}
