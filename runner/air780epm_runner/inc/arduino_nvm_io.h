#ifndef ARDUINO_NVM_IO_H
#define ARDUINO_NVM_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int arduinoCoreNvmRead(const char *name, uint8_t *version, uint8_t **data, size_t *size);
int arduinoCoreNvmWrite(const char *name, uint8_t version, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
