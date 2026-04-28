#ifndef ARDUINO_UDP_H
#define ARDUINO_UDP_H

#include <stddef.h>
#include <stdint.h>

#include "IPAddress.h"
#include "Stream.h"

class UDP : public Stream {
public:
    virtual ~UDP() {}

    virtual uint8_t begin(uint16_t port) = 0;
    virtual uint8_t beginMulticast(IPAddress multicastAddress, uint16_t port)
    {
        (void)multicastAddress;
        (void)port;
        return 0U;
    }
    virtual void stop(void) = 0;

    virtual int beginPacket(IPAddress ip, uint16_t port) = 0;
    virtual int beginPacket(const char *host, uint16_t port) = 0;
    virtual int endPacket(void) = 0;

    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;

    virtual int parsePacket(void) = 0;
    virtual int available(void) = 0;
    virtual int read(void) = 0;
    virtual int read(unsigned char *buffer, size_t size) = 0;
    virtual int peek(void) = 0;
    virtual void flush(void) = 0;

    virtual IPAddress remoteIP(void) = 0;
    virtual uint16_t remotePort(void) = 0;

    int read(char *buffer, size_t size)
    {
        return read(reinterpret_cast<unsigned char *>(buffer), size);
    }

    using Print::write;

protected:
    uint8_t *rawIPAddress(IPAddress &address)
    {
        return address.raw_address();
    }
};

#endif
