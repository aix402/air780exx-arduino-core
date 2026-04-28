#ifndef AIR780EPM_TLS_CLIENT_H
#define AIR780EPM_TLS_CLIENT_H

#include <stdint.h>

#include "Client.h"
#include "pgmspace.h"

enum AIR780EPMTLSError : int32_t {
    AIR780EPM_TLS_ERR_BAD_ARG = -70001,
    AIR780EPM_TLS_ERR_NO_MEMORY = -70002,
    AIR780EPM_TLS_ERR_MISSING_TRUST = -70003
};

class AIR780EPMTLSClient : public Client {
public:
    AIR780EPMTLSClient();
    ~AIR780EPMTLSClient();

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

    void setInsecure();
    void setCACert(const char *rootCA);
    void setConnectTimeout(uint32_t timeoutMs);
    void setConnectionTimeout(uint32_t timeoutMs);
    uint32_t connectTimeout(void) const;
    void setReadTimeout(uint32_t timeoutMs);
    uint32_t readTimeout(void) const;
    int32_t lastError(void) const;

private:
    int connectHost(const char *host, uint16_t port, bool useSni);

    void *handle_;
    const char *caCert_;
    int32_t lastError_;
    uint32_t connectTimeoutMs_;
    uint32_t readTimeoutMs_;
    bool insecure_;
};

typedef AIR780EPMTLSClient CellularClientSecure;

#endif
