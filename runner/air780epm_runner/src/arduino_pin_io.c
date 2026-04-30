#include "arduino_pin_io.h"

#include <stddef.h>

#include "c_common.h"
#include "luat_adc.h"
#include "luat_gpio.h"
#include "luat_pwm.h"

enum {
    ARDUINO_PIN_PWM0 = 1,
    ARDUINO_PIN_PWM1 = 24,
    ARDUINO_PIN_PWM2 = 31,
    ARDUINO_PIN_PWM4 = 33,
    ARDUINO_PIN_ADC0 = 39,
    ARDUINO_PIN_ADC1 = 40,
    ARDUINO_PIN_ADC2 = 41,
    ARDUINO_PIN_ADC3 = 42,
};

typedef enum {
    PWM_OWNER_NONE = 0,
    PWM_OWNER_ANALOG_WRITE,
    PWM_OWNER_SERVO,
} arduino_pwm_owner_t;

typedef struct {
    uint32_t frequency;
    int value;
    bool active;
    arduino_pwm_owner_t owner;
} arduino_pwm_state_t;

static const uint32_t k_default_pwm_frequency = 1000U;
static const uint32_t k_max_pwm_frequency = 13000000U;
static const uint32_t k_servo_refresh_interval_us = 20000U;
static const uint8_t k_pwm_channels[] = {0, 1, 2, 4};
static const int k_adc_hardware_resolution_bits = 12;
static const LUAT_ADC_RANGE_E k_default_adc_range = LUAT_ADC_AIO_RANGE_MAX;

static arduino_pwm_state_t g_pwm_state[5] = {
    {1000U, 0, false, PWM_OWNER_NONE},
    {1000U, 0, false, PWM_OWNER_NONE},
    {1000U, 0, false, PWM_OWNER_NONE},
    {1000U, 0, false, PWM_OWNER_NONE},
    {1000U, 0, false, PWM_OWNER_NONE},
};

static int g_analog_write_resolution_bits = 8;
static int g_analog_read_resolution_bits = 12;

static int pwm_channel_from_pin(uint8_t pin) {
    switch (pin) {
        case ARDUINO_PIN_PWM0:
            return 0;
        case ARDUINO_PIN_PWM1:
            return 1;
        case ARDUINO_PIN_PWM2:
            return 2;
        case ARDUINO_PIN_PWM4:
            return 4;
        default:
            return -1;
    }
}

static int adc_channel_from_pin(uint8_t pin) {
    switch (pin) {
        case ARDUINO_PIN_ADC0:
            return 0;
        case ARDUINO_PIN_ADC1:
            return 1;
        case ARDUINO_PIN_ADC2:
            return 2;
        case ARDUINO_PIN_ADC3:
            return 3;
        default:
            return -1;
    }
}

static uint32_t analog_write_max_value(void) {
    return (1UL << g_analog_write_resolution_bits) - 1UL;
}

static size_t value_to_pulse(int value) {
    const uint32_t max_value = analog_write_max_value();
    if (value <= 0) {
        return 0;
    }
    if ((uint32_t)value >= max_value) {
        return 1000;
    }
    return ((uint32_t)value * 1000UL + (max_value / 2UL)) / max_value;
}

static size_t pulse_us_to_permille(uint32_t pulse_us, uint32_t period_us) {
    if (period_us == 0U || pulse_us == 0U) {
        return 0U;
    }
    if (pulse_us >= period_us) {
        return 1000U;
    }
    return (size_t)((pulse_us * 1000UL + (period_us / 2UL)) / period_us);
}

static bool configure_pwm(int channel, uint32_t frequency, size_t pulse_permille, int value, arduino_pwm_owner_t owner) {
    luat_pwm_conf_t config = {0};
    config.channel = channel;
    config.period = frequency;
    config.pulse = pulse_permille;
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

static void drive_digital_fallback(uint8_t pin, bool high) {
    arduino_pin_mode(pin, ARDUINO_PIN_IO_OUTPUT);
    arduino_pin_write(pin, high ? ARDUINO_PIN_IO_HIGH : ARDUINO_PIN_IO_LOW);
}

static uint32_t scale_analog_read_value(uint32_t value) {
    if (g_analog_read_resolution_bits == k_adc_hardware_resolution_bits) {
        return value;
    }

    if (g_analog_read_resolution_bits > k_adc_hardware_resolution_bits) {
        return value << (g_analog_read_resolution_bits - k_adc_hardware_resolution_bits);
    }

    const int shift = k_adc_hardware_resolution_bits - g_analog_read_resolution_bits;
    if (shift <= 0) {
        return value;
    }

    const uint32_t rounding = 1UL << (shift - 1);
    return (value + rounding) >> shift;
}

static bool read_adc_channel(int channel, int* raw, int* microvolts) {
    if (channel < 0 || raw == NULL || microvolts == NULL) {
        return false;
    }

    luat_adc_ctrl_param_t range = {0};
    range.range = k_default_adc_range;
    if (luat_adc_ctrl(channel, LUAT_ADC_SET_GLOBAL_RANGE, range) != 0) {
        return false;
    }

    if (luat_adc_open(channel, NULL) != 0) {
        return false;
    }

    const int read_rc = luat_adc_read(channel, raw, microvolts);
    const int close_rc = luat_adc_close(channel);
    return (read_rc == 0) && (close_rc == 0);
}

void arduino_pin_mode(uint8_t pin, uint8_t mode) {
    arduino_pwm_detach_pin(pin);

    luat_gpio_cfg_t cfg;
    luat_gpio_set_default_cfg(&cfg);
    cfg.pin = pin;
    cfg.mode = (mode == ARDUINO_PIN_IO_OUTPUT) ? LUAT_GPIO_OUTPUT : LUAT_GPIO_INPUT;
    if (mode == ARDUINO_PIN_IO_INPUT_PULLUP) {
        cfg.pull = LUAT_GPIO_PULLUP;
    } else if (mode == ARDUINO_PIN_IO_INPUT_PULLDOWN) {
        cfg.pull = LUAT_GPIO_PULLDOWN;
    } else {
        cfg.pull = LUAT_GPIO_DEFAULT;
    }
    cfg.output_level = LUAT_GPIO_LOW;
    luat_gpio_open(&cfg);
}

void arduino_pin_write(uint8_t pin, uint8_t value) {
    arduino_pwm_detach_pin(pin);
    luat_gpio_set(pin, value ? LUAT_GPIO_HIGH : LUAT_GPIO_LOW);
}

int arduino_pin_read(uint8_t pin) {
    return luat_gpio_get(pin) ? ARDUINO_PIN_IO_HIGH : ARDUINO_PIN_IO_LOW;
}

void arduino_pwm_detach_pin(uint8_t pin) {
    const int channel = pwm_channel_from_pin(pin);
    if (channel < 0 || !g_pwm_state[channel].active) {
        return;
    }

    (void)luat_pwm_close(channel);
    g_pwm_state[channel].active = false;
    g_pwm_state[channel].owner = PWM_OWNER_NONE;
}

void arduino_analog_write(uint8_t pin, int value) {
    const uint32_t max_value = analog_write_max_value();
    const int channel = pwm_channel_from_pin(pin);

    if (channel < 0) {
        drive_digital_fallback(pin, value >= (int)((max_value + 1UL) / 2UL));
        return;
    }

    if (value <= 0) {
        arduino_pwm_detach_pin(pin);
        drive_digital_fallback(pin, false);
        return;
    }
    if ((uint32_t)value >= max_value) {
        arduino_pwm_detach_pin(pin);
        drive_digital_fallback(pin, true);
        return;
    }

    (void)configure_pwm(channel,
                        g_pwm_state[channel].frequency,
                        value_to_pulse(value),
                        value,
                        PWM_OWNER_ANALOG_WRITE);
}

void arduino_analog_write_resolution(int bits) {
    if (bits < 1) {
        bits = 1;
    } else if (bits > 16) {
        bits = 16;
    }
    g_analog_write_resolution_bits = bits;
}

bool arduino_analog_write_frequency_pin(uint8_t pin, uint32_t frequency) {
    if (frequency == 0U || frequency > k_max_pwm_frequency) {
        return false;
    }

    const int channel = pwm_channel_from_pin(pin);
    if (channel < 0) {
        return false;
    }

    if (g_pwm_state[channel].owner == PWM_OWNER_SERVO) {
        return false;
    }

    g_pwm_state[channel].frequency = frequency;
    if (g_pwm_state[channel].active) {
        return configure_pwm(channel,
                             g_pwm_state[channel].frequency,
                             value_to_pulse(g_pwm_state[channel].value),
                             g_pwm_state[channel].value,
                             PWM_OWNER_ANALOG_WRITE);
    }
    return true;
}

bool arduino_analog_write_frequency_all(uint32_t frequency) {
    if (frequency == 0U || frequency > k_max_pwm_frequency) {
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i < (sizeof(k_pwm_channels) / sizeof(k_pwm_channels[0])); ++i) {
        const uint8_t channel = k_pwm_channels[i];
        if (g_pwm_state[channel].owner != PWM_OWNER_SERVO) {
            g_pwm_state[channel].frequency = frequency;
        }
    }
    for (size_t i = 0; i < (sizeof(k_pwm_channels) / sizeof(k_pwm_channels[0])); ++i) {
        const uint8_t channel = k_pwm_channels[i];
        if (g_pwm_state[channel].active &&
            g_pwm_state[channel].owner == PWM_OWNER_ANALOG_WRITE &&
            !configure_pwm(channel,
                           g_pwm_state[channel].frequency,
                           value_to_pulse(g_pwm_state[channel].value),
                           g_pwm_state[channel].value,
                           PWM_OWNER_ANALOG_WRITE)) {
            ok = false;
        }
    }
    return ok;
}

int arduino_analog_read(uint8_t pin) {
    const int channel = adc_channel_from_pin(pin);
    if (channel < 0) {
        return 0;
    }

    int raw = 0;
    int microvolts = 0;
    if (!read_adc_channel(channel, &raw, &microvolts) || raw < 0) {
        return 0;
    }

    return (int)scale_analog_read_value((uint32_t)raw);
}

uint32_t arduino_analog_read_millivolts(uint8_t pin) {
    const int channel = adc_channel_from_pin(pin);
    if (channel < 0) {
        return 0;
    }

    int raw = 0;
    int microvolts = 0;
    if (!read_adc_channel(channel, &raw, &microvolts) || microvolts <= 0) {
        return 0;
    }

    return (uint32_t)((microvolts + 500) / 1000);
}

void arduino_analog_read_resolution(int bits) {
    if (bits < 1) {
        bits = 1;
    } else if (bits > 16) {
        bits = 16;
    }
    g_analog_read_resolution_bits = bits;
}

int arduino_servo_attach(uint32_t pin, uint32_t pulse_us, uint32_t period_us) {
    const int channel = pwm_channel_from_pin((uint8_t)pin);
    const uint32_t effective_period_us = (period_us == 0U) ? k_servo_refresh_interval_us : period_us;
    const uint32_t frequency = 1000000UL / effective_period_us;

    if (channel < 0 || frequency == 0U) {
        return -1;
    }

    return configure_pwm(channel,
                         frequency,
                         pulse_us_to_permille(pulse_us, effective_period_us),
                         (int)pulse_us,
                         PWM_OWNER_SERVO)
               ? 1
               : -1;
}

int arduino_servo_attached(uint32_t pin) {
    const int channel = pwm_channel_from_pin((uint8_t)pin);
    if (channel < 0) {
        return 0;
    }

    return (g_pwm_state[channel].active && g_pwm_state[channel].owner == PWM_OWNER_SERVO) ? 1 : 0;
}

void arduino_servo_detach_pin(uint32_t pin) {
    arduino_pwm_detach_pin((uint8_t)pin);
}

int arduino_servo_write_microseconds(uint32_t pin, uint32_t pulse_us, uint32_t period_us) {
    const int channel = pwm_channel_from_pin((uint8_t)pin);
    const uint32_t effective_period_us = (period_us == 0U) ? k_servo_refresh_interval_us : period_us;
    const uint32_t frequency = 1000000UL / effective_period_us;

    if (channel < 0 || frequency == 0U || g_pwm_state[channel].owner != PWM_OWNER_SERVO) {
        return -1;
    }

    return configure_pwm(channel,
                         frequency,
                         pulse_us_to_permille(pulse_us, effective_period_us),
                         (int)pulse_us,
                         PWM_OWNER_SERVO)
               ? 0
               : -1;
}
