#ifndef AIR780EPM_UDP_H
#define AIR780EPM_UDP_H

#include <stddef.h>
#include <stdint.h>

#include "Udp.h"

class AIR780EPMUDP : public UDP {
public:
    AIR780EPMUDP();
    ~AIR780EPMUDP();

    uint8_t begin(uint16_t port) override;
    void stop(void) override;

    int beginPacket(IPAddress ip, uint16_t port) override;
    int beginPacket(const char *host, uint16_t port) override;
    int endPacket(void) override;

    size_t write(uint8_t value) override;
    size_t write(const uint8_t *buffer, size_t size) override;

    int parsePacket(void) override;
    int available(void) override;
    int read(void) override;
    int read(unsigned char *buffer, size_t size) override;
    int peek(void) override;
    void flush(void) override;

    IPAddress remoteIP(void) override;
    uint16_t remotePort(void) override;

    uint16_t localPort(void) const;
    int32_t lastError(void) const;

private:
    static const size_t kMaxPacketSize = 1472U;

    void clearRxPacket(void);
    bool ensureOpen(void);

    int socket_;
    int32_t lastError_;
    uint16_t localPort_;

    IPAddress txRemoteIp_;
    uint16_t txRemotePort_;
    bool packetOpen_;
    uint8_t txBuffer_[kMaxPacketSize];
    size_t txLength_;

    IPAddress rxRemoteIp_;
    uint16_t rxRemotePort_;
    uint8_t rxBuffer_[kMaxPacketSize];
    size_t rxLength_;
    size_t rxOffset_;
};

typedef AIR780EPMUDP CellularUDP;

#endif
