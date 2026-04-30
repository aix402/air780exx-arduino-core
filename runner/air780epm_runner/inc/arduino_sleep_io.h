#ifndef ARDUINO_SLEEP_IO_H
#define ARDUINO_SLEEP_IO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool arduinoCoreSleepLight(uint32_t milliseconds);
bool arduinoCoreSleepDeep(uint32_t milliseconds, uint8_t timerId);
bool arduinoCoreSleepSetWakeupPad(uint8_t pad,
                                  uint8_t edge,
                                  uint8_t pullup,
                                  uint8_t pulldown);
bool arduinoCoreSleepClearWakeupPad(uint8_t pad);
uint8_t arduinoCoreSleepWakeupReason(void);
uint8_t arduinoCoreSleepLastState(void);
uint32_t arduinoCoreSleepTimeMillis(void);
uint8_t arduinoCoreSleepWakeupPinBitmap(void);

#ifdef __cplusplus
}
#endif

#endif
