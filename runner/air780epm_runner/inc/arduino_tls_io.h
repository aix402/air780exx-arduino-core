#ifndef ARDUINO_TLS_IO_H
#define ARDUINO_TLS_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ArduinoCoreTlsConfig {
    const char *caCert;
    uint32_t caCertLen;
    uint32_t connectTimeoutMs;
    uint32_t ioTimeoutMs;
    uint8_t insecure;
    uint8_t sni;
} ArduinoCoreTlsConfig;

void *arduinoCoreTlsConnectHost(const char *host, uint16_t port, const ArduinoCoreTlsConfig *config, int32_t *outError);
int arduinoCoreTlsWrite(void *handle, const uint8_t *buffer, uint32_t size, uint32_t timeoutMs, int32_t *outError);
int arduinoCoreTlsAvailable(void *handle, int32_t *outError);
int arduinoCoreTlsRead(void *handle, uint8_t *buffer, uint32_t size, int peek, int32_t *outError);
int arduinoCoreTlsIsConnected(void *handle, int32_t *outError);
void arduinoCoreTlsClose(void *handle);

#ifdef __cplusplus
}
#endif

#endif
