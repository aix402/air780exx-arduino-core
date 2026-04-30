#ifndef ARDUINO_TIME_IO_H
#define ARDUINO_TIME_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int arduinoCoreTimeSetEpoch(uint32_t epochSeconds);
int arduinoCoreTimeSetTimezoneQuarterHours(int8_t timezoneQuarterHours);
int arduinoCoreTimeGetTimezoneQuarterHours(int8_t *outTimezoneQuarterHours);
void arduinoCoreDelayMs(uint32_t milliseconds);
void arduinoCoreDelayUs(uint32_t microseconds);
uint64_t arduinoCoreMillis(void);
void arduinoCoreYield(void);

#ifdef __cplusplus
}
#endif

#endif
