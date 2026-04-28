#ifndef ARDUINO_TCP_IO_H
#define ARDUINO_TCP_IO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int arduinoCoreTcpConnectHost(const char *host, uint16_t port, uint32_t timeoutMs, int32_t *outError);
int arduinoCoreTcpConnectIPv4(const uint8_t ip[4], uint16_t port, uint32_t timeoutMs, int32_t *outError);
int arduinoCoreTcpWrite(int socketId, const uint8_t *buffer, uint32_t size, uint32_t timeoutMs, int32_t *outError);
int arduinoCoreTcpAvailable(int socketId, int32_t *outError);
int arduinoCoreTcpRead(int socketId, uint8_t *buffer, uint32_t size, int peek, int32_t *outError);
int arduinoCoreTcpIsConnected(int socketId, int32_t *outError);
void arduinoCoreTcpClose(int socketId);

#ifdef __cplusplus
}
#endif

#endif
