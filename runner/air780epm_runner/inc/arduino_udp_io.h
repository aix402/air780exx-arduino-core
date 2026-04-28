#ifndef ARDUINO_UDP_IO_H
#define ARDUINO_UDP_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int arduinoCoreUdpOpen(uint16_t localPort, int32_t *outError);
void arduinoCoreUdpClose(int socketId);
int arduinoCoreUdpResolveHost(const char *host, uint8_t outIp[4], int32_t *outError);
int arduinoCoreUdpSendToIPv4(int socketId,
                             const uint8_t ip[4],
                             uint16_t port,
                             const uint8_t *buffer,
                             uint32_t size,
                             int32_t *outError);
int arduinoCoreUdpParsePacket(int socketId,
                              uint8_t *buffer,
                              uint32_t bufferSize,
                              uint8_t remoteIp[4],
                              uint16_t *remotePort,
                              int32_t *outError);

#ifdef __cplusplus
}
#endif

#endif
