#include "Servo.h"

extern "C" int arduinoCoreServoAttach(uint32_t pin, uint32_t pulseUs, uint32_t periodUs);
extern "C" void arduinoCoreServoDetachPin(uint32_t pin);
extern "C" int arduinoCoreServoAttached(uint32_t pin);
extern "C" int arduinoCoreServoWriteMicroseconds(uint32_t pin, uint32_t pulseUs, uint32_t periodUs);

namespace {

static const pin_size_t kInvalidServoPin = (pin_size_t)0xFFFFFFFFUL;

int clampPulse(int value, int minPulse, int maxPulse)
{
    if (value < minPulse)
    {
        return minPulse;
    }

    if (value > maxPulse)
    {
        return maxPulse;
    }

    return value;
}

int angleToPulse(int angle, int minPulse, int maxPulse)
{
    if (angle < 0)
    {
        angle = 0;
    }

    if (angle > 180)
    {
        angle = 180;
    }

    return minPulse + (int)(((long)(maxPulse - minPulse) * (long)angle + 90L) / 180L);
}

int pulseToAngle(int pulse, int minPulse, int maxPulse)
{
    if (maxPulse <= minPulse)
    {
        return 0;
    }

    pulse = clampPulse(pulse, minPulse, maxPulse);
    return (int)(((long)(pulse - minPulse) * 180L + (long)((maxPulse - minPulse) / 2)) /
                 (long)(maxPulse - minPulse));
}

}  // namespace

Servo::Servo()
    : pin_(kInvalidServoPin),
      min_pulse_us_(MIN_PULSE_WIDTH),
      max_pulse_us_(MAX_PULSE_WIDTH),
      pulse_us_(DEFAULT_PULSE_WIDTH),
      attached_(false)
{
}

uint8_t Servo::attach(int pin)
{
    return attach(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

uint8_t Servo::attach(int pin, int min, int max)
{
    int result;

    if (pin < 0)
    {
        attached_ = false;
        pin_ = kInvalidServoPin;
        return INVALID_SERVO;
    }

    if (min <= 0)
    {
        min = MIN_PULSE_WIDTH;
    }

    if (max <= min)
    {
        max = MAX_PULSE_WIDTH;
    }

    min_pulse_us_ = min;
    max_pulse_us_ = max;
    pulse_us_ = clampPulse(pulse_us_, min_pulse_us_, max_pulse_us_);

    result = arduinoCoreServoAttach((uint32_t)pin, (uint32_t)pulse_us_, REFRESH_INTERVAL);
    if (result < 0)
    {
        attached_ = false;
        pin_ = kInvalidServoPin;
        return INVALID_SERVO;
    }

    pin_ = (pin_size_t)pin;
    attached_ = true;
    return (uint8_t)result;
}

void Servo::detach(void)
{
    if (attached_ && (pin_ != kInvalidServoPin))
    {
        arduinoCoreServoDetachPin((uint32_t)pin_);
    }

    attached_ = false;
    pin_ = kInvalidServoPin;
}

void Servo::write(int value)
{
    if (value < MIN_PULSE_WIDTH)
    {
        writeMicroseconds(angleToPulse(value, min_pulse_us_, max_pulse_us_));
        return;
    }

    writeMicroseconds(value);
}

void Servo::writeMicroseconds(int value)
{
    if (!attached_ || (pin_ == kInvalidServoPin))
    {
        return;
    }

    pulse_us_ = clampPulse(value, min_pulse_us_, max_pulse_us_);
    if (arduinoCoreServoWriteMicroseconds((uint32_t)pin_, (uint32_t)pulse_us_, REFRESH_INTERVAL) != 0)
    {
        attached_ = false;
        pin_ = kInvalidServoPin;
    }
}

int Servo::read(void) const
{
    return pulseToAngle(pulse_us_, min_pulse_us_, max_pulse_us_);
}

int Servo::readMicroseconds(void) const
{
    return pulse_us_;
}

bool Servo::attached(void) const
{
    if (!attached_ || (pin_ == kInvalidServoPin))
    {
        return false;
    }

    return (arduinoCoreServoAttached((uint32_t)pin_) != 0);
}
