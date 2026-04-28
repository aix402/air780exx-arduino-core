#ifndef AIR780EPM_CLIENT_H
#define AIR780EPM_CLIENT_H

#include <stdint.h>

#include "Client.h"
#include "pgmspace.h"

class AIR780EPMClient : public Client {
public:
    AIR780EPMClient();
    ~AIR780EPMClient();

    int connect(IPAddress ip, uint16_t port) override;
    int connect(IPAddress ip, uint16_t port, int32_t timeoutMs);
    int connect(const char *host, uint16_t port) override;
    int connect(const char *host, uint16_t port, int32_t timeoutMs);
    size_t write(uint8_t value) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    size_t write_P(PGM_P buffer, size_t size);
    int available(void) override;
    int read(void) override;
    int read(uint8_t *buffer, size_t size) override;
    int peek(void) override;
    void flush(void) override;
    void stop(void) override;
    uint8_t connected(void) override;
    operator bool() override;

    void setConnectTimeout(uint32_t timeoutMs);
    void setConnectionTimeout(uint32_t timeoutMs);
    uint32_t connectTimeout(void) const;
    int setNoDelay(bool noDelay);
    bool getNoDelay(void) const;
    int32_t lastError(void) const;

private:
    int socket_;
    int32_t lastError_;
    uint32_t connectTimeoutMs_;
};

typedef AIR780EPMClient CellularClient;

#endif
