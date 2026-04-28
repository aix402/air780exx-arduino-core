#ifndef CLIENT_H
#define CLIENT_H

#include "IPAddress.h"
#include "Stream.h"

class Client : public Stream {
public:
    virtual ~Client() {}

    virtual int connect(IPAddress ip, uint16_t port) = 0;
    virtual int connect(const char *host, uint16_t port) = 0;
    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual int available(void) = 0;
    virtual int read(void) = 0;
    virtual int read(uint8_t *buffer, size_t size) = 0;
    virtual int peek(void) = 0;
    virtual void flush(void) = 0;
    virtual void stop(void) = 0;
    virtual uint8_t connected(void) = 0;
    virtual operator bool() = 0;

    using Print::write;

protected:
    uint8_t *rawIPAddress(IPAddress &address)
    {
        return address.raw_address();
    }
};

#endif
