#ifndef SERVO_H
#define SERVO_H

#include "Arduino.h"

#define MIN_PULSE_WIDTH 544
#define MAX_PULSE_WIDTH 2400
#define DEFAULT_PULSE_WIDTH 1500
#define REFRESH_INTERVAL 20000
#define INVALID_SERVO 255
#define MAX_SERVOS 6
#define SERVOS_PER_TIMER 1

class Servo {
public:
    Servo();

    uint8_t attach(int pin);
    uint8_t attach(int pin, int min, int max);
    void detach(void);
    void write(int value);
    void writeMicroseconds(int value);
    int read(void) const;
    int readMicroseconds(void) const;
    bool attached(void) const;

private:
    pin_size_t pin_;
    int min_pulse_us_;
    int max_pulse_us_;
    int pulse_us_;
    bool attached_;
};

#endif
