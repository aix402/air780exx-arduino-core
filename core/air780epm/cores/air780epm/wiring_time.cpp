#include "Arduino.h"

extern "C" {
#include "common_api.h"
#include "luat_rtos.h"
void delay_us(uint32_t us);
}

void delay(unsigned long ms) {
    luat_rtos_task_sleep(ms);
}

void delayMicroseconds(unsigned int us) {
    delay_us(static_cast<uint32_t>(us));
}

unsigned long millis(void) {
    return static_cast<unsigned long>(soc_get_poweron_time_ms());
}

unsigned long micros(void) {
    return static_cast<unsigned long>(soc_get_poweron_time_ms() * 1000ULL);
}

void yield(void) {
    luat_rtos_task_sleep(1);
}
