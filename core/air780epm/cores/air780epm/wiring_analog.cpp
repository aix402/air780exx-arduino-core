#include "Arduino.h"

extern "C" {
#include "c_common.h"
#include "luat_adc.h"
#include "luat_pwm.h"
}

namespace {

constexpr uint32_t kDefaultPwmFrequency = 1000U;
constexpr uint32_t kMaxPwmFrequency = 13000000U;
constexpr uint32_t kServoFrequency = 50U;
constexpr uint32_t kServoRefreshIntervalUs = 20000U;
constexpr uint8_t kPwmChannels[] = {0, 1, 2, 4};
constexpr int kAdcHardwareResolutionBits = 12;
constexpr LUAT_ADC_RANGE_E kDefaultAdcRange = LUAT_ADC_AIO_RANGE_MAX;

enum class PwmOwner : uint8_t {
    None = 0,
    AnalogWrite,
    Servo
};

struct PwmState {
    uint32_t frequency;
    int value;
    bool active;
    PwmOwner owner;
};

PwmState g_pwm_state[5] = {
    {kDefaultPwmFrequency, 0, false, PwmOwner::None},
    {kDefaultPwmFrequency, 0, false, PwmOwner::None},
    {kDefaultPwmFrequency, 0, false, PwmOwner::None},
    {kDefaultPwmFrequency, 0, false, PwmOwner::None},
    {kDefaultPwmFrequency, 0, false, PwmOwner::None},
};

int g_analog_write_resolution_bits = 8;
int g_analog_read_resolution_bits = kAdcHardwareResolutionBits;

int pwmChannelFromPin(pin_size_t pin) {
    switch (pin) {
        case PIN_PWM0:
            return 0;
        case PIN_PWM1:
            return 1;
        case PIN_PWM2:
            return 2;
        case PIN_PWM4:
            return 4;
        default:
            return -1;
    }
}

int adcChannelFromPin(pin_size_t pin) {
    switch (pin) {
        case PIN_ADC0:
            return 0;
        case PIN_ADC1:
            return 1;
        case PIN_ADC2:
            return 2;
        case PIN_ADC3:
            return 3;
        default:
            return -1;
    }
}

uint32_t analogWriteMaxValue() {
    return (1UL << g_analog_write_resolution_bits) - 1UL;
}

size_t valueToPulse(int value) {
    const uint32_t max_value = analogWriteMaxValue();
    if (value <= 0) {
        return 0;
    }
    if (static_cast<uint32_t>(value) >= max_value) {
        return 1000;
    }
    return (static_cast<uint32_t>(value) * 1000UL + (max_value / 2UL)) / max_value;
}

size_t pulseUsToPermille(uint32_t pulseUs, uint32_t periodUs) {
    if (periodUs == 0U || pulseUs == 0U) {
        return 0U;
    }
    if (pulseUs >= periodUs) {
        return 1000U;
    }
    return static_cast<size_t>((pulseUs * 1000UL + (periodUs / 2UL)) / periodUs);
}

bool configurePwm(int channel, uint32_t frequency, size_t pulsePermille, int value, PwmOwner owner) {
    luat_pwm_conf_t config = {};
    config.channel = channel;
    config.period = frequency;
    config.pulse = pulsePermille;
    config.pnum = 0;
    config.precision = 1000;
    config.stop_level = 1;
    config.reverse = 0;

    if (luat_pwm_setup(&config) != 0) {
        g_pwm_state[channel].active = false;
        return false;
    }

    g_pwm_state[channel].frequency = frequency;
    g_pwm_state[channel].value = value;
    g_pwm_state[channel].active = true;
    g_pwm_state[channel].owner = owner;
    return true;
}

void driveDigitalFallback(pin_size_t pin, bool high) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, high ? HIGH : LOW);
}

uint32_t scaleAnalogReadValue(uint32_t value) {
    if (g_analog_read_resolution_bits == kAdcHardwareResolutionBits) {
        return value;
    }

    if (g_analog_read_resolution_bits > kAdcHardwareResolutionBits) {
        return value << (g_analog_read_resolution_bits - kAdcHardwareResolutionBits);
    }

    const int shift = kAdcHardwareResolutionBits - g_analog_read_resolution_bits;
    if (shift <= 0) {
        return value;
    }

    const uint32_t rounding = 1UL << (shift - 1);
    return (value + rounding) >> shift;
}

bool readAdcChannel(int channel, int* raw, int* microvolts) {
    if (channel < 0 || raw == nullptr || microvolts == nullptr) {
        return false;
    }

    luat_adc_ctrl_param_t range = {};
    range.range = kDefaultAdcRange;
    if (luat_adc_ctrl(channel, LUAT_ADC_SET_GLOBAL_RANGE, range) != 0) {
        return false;
    }

    if (luat_adc_open(channel, nullptr) != 0) {
        return false;
    }

    const int read_rc = luat_adc_read(channel, raw, microvolts);
    const int close_rc = luat_adc_close(channel);
    return (read_rc == 0) && (close_rc == 0);
}

}  // namespace

extern "C" void air780epm_pwm_detach_pin(uint8_t pin) {
    const int channel = pwmChannelFromPin(pin);
    if (channel < 0 || !g_pwm_state[channel].active) {
        return;
    }

    (void)luat_pwm_close(channel);
    g_pwm_state[channel].active = false;
    g_pwm_state[channel].owner = PwmOwner::None;
}

extern "C" void analogWrite(pin_size_t pin, int value) {
    const uint32_t max_value = analogWriteMaxValue();
    const int channel = pwmChannelFromPin(pin);

    if (channel < 0) {
        driveDigitalFallback(pin, value >= static_cast<int>((max_value + 1UL) / 2UL));
        return;
    }

    if (value <= 0) {
        air780epm_pwm_detach_pin(pin);
        driveDigitalFallback(pin, false);
        return;
    }
    if (static_cast<uint32_t>(value) >= max_value) {
        air780epm_pwm_detach_pin(pin);
        driveDigitalFallback(pin, true);
        return;
    }

    (void)configurePwm(channel,
                       g_pwm_state[channel].frequency,
                       valueToPulse(value),
                       value,
                       PwmOwner::AnalogWrite);
}

extern "C" void analogWriteResolution(int bits) {
    if (bits < 1) {
        bits = 1;
    } else if (bits > 16) {
        bits = 16;
    }
    g_analog_write_resolution_bits = bits;
}

bool analogWriteFrequency(pin_size_t pin, uint32_t frequency) {
    if (frequency == 0U || frequency > kMaxPwmFrequency) {
        return false;
    }

    const int channel = pwmChannelFromPin(pin);
    if (channel < 0) {
        return false;
    }

    if (g_pwm_state[channel].owner == PwmOwner::Servo) {
        return false;
    }

    g_pwm_state[channel].frequency = frequency;
    if (g_pwm_state[channel].active) {
        return configurePwm(channel,
                            g_pwm_state[channel].frequency,
                            valueToPulse(g_pwm_state[channel].value),
                            g_pwm_state[channel].value,
                            PwmOwner::AnalogWrite);
    }
    return true;
}

bool analogWriteFrequency(uint32_t frequency) {
    if (frequency == 0U || frequency > kMaxPwmFrequency) {
        return false;
    }

    bool ok = true;
    for (uint8_t channel : kPwmChannels) {
        if (g_pwm_state[channel].owner != PwmOwner::Servo) {
            g_pwm_state[channel].frequency = frequency;
        }
    }
    for (uint8_t channel : kPwmChannels) {
        if (g_pwm_state[channel].active &&
            g_pwm_state[channel].owner == PwmOwner::AnalogWrite &&
            !configurePwm(channel,
                          g_pwm_state[channel].frequency,
                          valueToPulse(g_pwm_state[channel].value),
                          g_pwm_state[channel].value,
                          PwmOwner::AnalogWrite)) {
            ok = false;
        }
    }
    return ok;
}

void analogWriteFreq(uint32_t frequency) {
    (void)analogWriteFrequency(frequency);
}

extern "C" int analogRead(pin_size_t pin) {
    const int channel = adcChannelFromPin(pin);
    if (channel < 0) {
        return 0;
    }

    int raw = 0;
    int microvolts = 0;
    if (!readAdcChannel(channel, &raw, &microvolts) || raw < 0) {
        return 0;
    }

    return static_cast<int>(scaleAnalogReadValue(static_cast<uint32_t>(raw)));
}

extern "C" uint32_t analogReadMilliVolts(pin_size_t pin) {
    const int channel = adcChannelFromPin(pin);
    if (channel < 0) {
        return 0;
    }

    int raw = 0;
    int microvolts = 0;
    if (!readAdcChannel(channel, &raw, &microvolts) || microvolts <= 0) {
        return 0;
    }

    return static_cast<uint32_t>((microvolts + 500) / 1000);
}

extern "C" void analogReadResolution(int bits) {
    if (bits < 1) {
        bits = 1;
    } else if (bits > 16) {
        bits = 16;
    }
    g_analog_read_resolution_bits = bits;
}

extern "C" int arduinoCoreServoAttach(uint32_t pin, uint32_t pulseUs, uint32_t periodUs) {
    const int channel = pwmChannelFromPin(static_cast<pin_size_t>(pin));
    const uint32_t effectivePeriodUs = (periodUs == 0U) ? kServoRefreshIntervalUs : periodUs;
    const uint32_t frequency = 1000000UL / effectivePeriodUs;

    if (channel < 0 || frequency == 0U) {
        return -1;
    }

    return configurePwm(channel,
                        frequency,
                        pulseUsToPermille(pulseUs, effectivePeriodUs),
                        static_cast<int>(pulseUs),
                        PwmOwner::Servo)
               ? 1
               : -1;
}

extern "C" int arduinoCoreServoAttached(uint32_t pin) {
    const int channel = pwmChannelFromPin(static_cast<pin_size_t>(pin));
    if (channel < 0) {
        return 0;
    }

    return (g_pwm_state[channel].active && g_pwm_state[channel].owner == PwmOwner::Servo) ? 1 : 0;
}

extern "C" void arduinoCoreServoDetachPin(uint32_t pin) {
    air780epm_pwm_detach_pin(static_cast<uint8_t>(pin));
}

extern "C" int arduinoCoreServoWriteMicroseconds(uint32_t pin, uint32_t pulseUs, uint32_t periodUs) {
    const int channel = pwmChannelFromPin(static_cast<pin_size_t>(pin));
    const uint32_t effectivePeriodUs = (periodUs == 0U) ? kServoRefreshIntervalUs : periodUs;
    const uint32_t frequency = 1000000UL / effectivePeriodUs;

    if (channel < 0 || frequency == 0U || g_pwm_state[channel].owner != PwmOwner::Servo) {
        return -1;
    }

    return configurePwm(channel,
                        frequency,
                        pulseUsToPermille(pulseUs, effectivePeriodUs),
                        static_cast<int>(pulseUs),
                        PwmOwner::Servo)
               ? 0
               : -1;
}
