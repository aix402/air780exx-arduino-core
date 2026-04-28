#include "Arduino.h"

extern "C" {
#include "luat_gpio.h"
}

extern "C" void air780epm_pwm_detach_pin(uint8_t pin);

void pinMode(uint8_t pin, uint8_t mode) {
    air780epm_pwm_detach_pin(pin);

    luat_gpio_cfg_t cfg;
    luat_gpio_set_default_cfg(&cfg);
    cfg.pin = pin;
    cfg.mode = (mode == OUTPUT) ? LUAT_GPIO_OUTPUT : LUAT_GPIO_INPUT;
    if (mode == INPUT_PULLUP) {
        cfg.pull = LUAT_GPIO_PULLUP;
    } else if (mode == INPUT_PULLDOWN) {
        cfg.pull = LUAT_GPIO_PULLDOWN;
    } else {
        cfg.pull = LUAT_GPIO_DEFAULT;
    }
    cfg.output_level = LUAT_GPIO_LOW;
    luat_gpio_open(&cfg);
}

void digitalWrite(uint8_t pin, uint8_t val) {
    air780epm_pwm_detach_pin(pin);
    luat_gpio_set(pin, val ? LUAT_GPIO_HIGH : LUAT_GPIO_LOW);
}

int digitalRead(uint8_t pin) {
    return luat_gpio_get(pin) ? HIGH : LOW;
}
