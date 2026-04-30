#ifndef ARDUINO_DEBUG_IO_H
#define ARDUINO_DEBUG_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t arduinoCoreDebugWrite(const uint8_t *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif
