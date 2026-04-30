#include "Arduino.h"

#include "arduino_pin_io.h"

extern "C" void analogWrite(pin_size_t pin, int value) {
    arduino_analog_write(pin, value);
}

extern "C" void analogWriteResolution(int bits) {
    arduino_analog_write_resolution(bits);
}

bool analogWriteFrequency(pin_size_t pin, uint32_t frequency) {
    return arduino_analog_write_frequency_pin(pin, frequency);
}

bool analogWriteFrequency(uint32_t frequency) {
    return arduino_analog_write_frequency_all(frequency);
}

void analogWriteFreq(uint32_t frequency) {
    (void)analogWriteFrequency(frequency);
}

extern "C" int analogRead(pin_size_t pin) {
    return arduino_analog_read(pin);
}

extern "C" uint32_t analogReadMilliVolts(pin_size_t pin) {
    return arduino_analog_read_millivolts(pin);
}

extern "C" void analogReadResolution(int bits) {
    arduino_analog_read_resolution(bits);
}

extern "C" int arduinoCoreServoAttach(uint32_t pin, uint32_t pulseUs, uint32_t periodUs) {
    return arduino_servo_attach(pin, pulseUs, periodUs);
}

extern "C" int arduinoCoreServoAttached(uint32_t pin) {
    return arduino_servo_attached(pin);
}

extern "C" void arduinoCoreServoDetachPin(uint32_t pin) {
    arduino_servo_detach_pin(pin);
}

extern "C" int arduinoCoreServoWriteMicroseconds(uint32_t pin, uint32_t pulseUs, uint32_t periodUs) {
    return arduino_servo_write_microseconds(pin, pulseUs, periodUs);
}
