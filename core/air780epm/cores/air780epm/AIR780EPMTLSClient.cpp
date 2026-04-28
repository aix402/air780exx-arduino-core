#include "AIR780EPMTLSClient.h"

#include <stdio.h>
#include <string.h>

#include "IPAddress.h"
#include "arduino_tls_io.h"

AIR780EPMTLSClient::AIR780EPMTLSClient() :
    handle_(0),
    caCert_(0),
    lastError_(0),
    connectTimeoutMs_(60000UL),
    readTimeoutMs_(5000UL),
    insecure_(false)
{
}

AIR780EPMTLSClient::~AIR780EPMTLSClient()
{
    stop();
}

int AIR780EPMTLSClient::connect(IPAddress ip, uint16_t port)
{
    char host[16];
    const uint8_t *raw = ip.raw_address();
    snprintf(host, sizeof(host), "%u.%u.%u.%u",
             static_cast<unsigned int>(raw[0]),
             static_cast<unsigned int>(raw[1]),
             static_cast<unsigned int>(raw[2]),
             static_cast<unsigned int>(raw[3]));
    return connectHost(host, port, false);
}

int AIR780EPMTLSClient::connect(IPAddress ip, uint16_t port, int32_t timeoutMs)
{
    const uint32_t previousTimeout = connectTimeoutMs_;
    connectTimeoutMs_ = (timeoutMs > 0) ? static_cast<uint32_t>(timeoutMs) : 1U;
    const int result = connect(ip, port);
    connectTimeoutMs_ = previousTimeout;
    return result;
}

int AIR780EPMTLSClient::connect(const char *host, uint16_t port)
{
    return connectHost(host, port, true);
}

int AIR780EPMTLSClient::connect(const char *host, uint16_t port, int32_t timeoutMs)
{
    const uint32_t previousTimeout = connectTimeoutMs_;
    connectTimeoutMs_ = (timeoutMs > 0) ? static_cast<uint32_t>(timeoutMs) : 1U;
    const int result = connect(host, port);
    connectTimeoutMs_ = previousTimeout;
    return result;
}

int AIR780EPMTLSClient::connectHost(const char *host, uint16_t port, bool useSni)
{
    ArduinoCoreTlsConfig config;

    stop();

    memset(&config, 0, sizeof(config));
    config.caCert = caCert_;
    config.caCertLen = (caCert_ != 0) ? static_cast<uint32_t>(strlen(caCert_) + 1U) : 0U;
    config.connectTimeoutMs = connectTimeoutMs_;
    config.ioTimeoutMs = readTimeoutMs_;
    config.insecure = insecure_ ? 1U : 0U;
    config.sni = useSni ? 1U : 0U;

    handle_ = arduinoCoreTlsConnectHost(host, port, &config, &lastError_);
    return (handle_ != 0) ? 1 : 0;
}

size_t AIR780EPMTLSClient::write(uint8_t value)
{
    return write(&value, 1U);
}

size_t AIR780EPMTLSClient::write(const uint8_t *buffer, size_t size)
{
    int sent;

    if ((handle_ == 0) || (buffer == 0) || (size == 0U))
    {
        return 0U;
    }

    sent = arduinoCoreTlsWrite(handle_, buffer, static_cast<uint32_t>(size), readTimeoutMs_, &lastError_);
    if (sent < 0)
    {
        stop();
        return 0U;
    }

    return static_cast<size_t>(sent);
}

size_t AIR780EPMTLSClient::write_P(PGM_P buffer, size_t size)
{
    return write(reinterpret_cast<const uint8_t *>(buffer), size);
}

int AIR780EPMTLSClient::available(void)
{
    int result;

    if (handle_ == 0)
    {
        return 0;
    }

    result = arduinoCoreTlsAvailable(handle_, &lastError_);
    if (result < 0)
    {
        stop();
        return 0;
    }

    return result;
}

int AIR780EPMTLSClient::read(void)
{
    uint8_t value = 0U;
    const int result = read(&value, 1U);

    if (result <= 0)
    {
        return -1;
    }

    return value;
}

int AIR780EPMTLSClient::read(uint8_t *buffer, size_t size)
{
    int result;

    if ((handle_ == 0) || (buffer == 0) || (size == 0U))
    {
        return 0;
    }

    result = arduinoCoreTlsRead(handle_, buffer, static_cast<uint32_t>(size), 0, &lastError_);
    if (result < 0)
    {
        stop();
        return -1;
    }

    return result;
}

int AIR780EPMTLSClient::peek(void)
{
    uint8_t value = 0U;
    int result;

    if (handle_ == 0)
    {
        return -1;
    }

    result = arduinoCoreTlsRead(handle_, &value, 1U, 1, &lastError_);
    if (result < 0)
    {
        stop();
        return -1;
    }

    if (result == 0)
    {
        return -1;
    }

    return value;
}

void AIR780EPMTLSClient::flush(void)
{
}

void AIR780EPMTLSClient::stop(void)
{
    if (handle_ != 0)
    {
        arduinoCoreTlsClose(handle_);
        handle_ = 0;
    }
}

uint8_t AIR780EPMTLSClient::connected(void)
{
    int result;

    if (handle_ == 0)
    {
        return 0U;
    }

    result = arduinoCoreTlsIsConnected(handle_, &lastError_);
    if (result <= 0)
    {
        stop();
        return 0U;
    }

    return 1U;
}

AIR780EPMTLSClient::operator bool()
{
    return connected() != 0U;
}

void AIR780EPMTLSClient::setInsecure()
{
    insecure_ = true;
    caCert_ = 0;
}

void AIR780EPMTLSClient::setCACert(const char *rootCA)
{
    caCert_ = rootCA;
    insecure_ = false;
}

void AIR780EPMTLSClient::setConnectTimeout(uint32_t timeoutMs)
{
    connectTimeoutMs_ = timeoutMs;
}

void AIR780EPMTLSClient::setConnectionTimeout(uint32_t timeoutMs)
{
    setConnectTimeout(timeoutMs);
}

uint32_t AIR780EPMTLSClient::connectTimeout(void) const
{
    return connectTimeoutMs_;
}

void AIR780EPMTLSClient::setReadTimeout(uint32_t timeoutMs)
{
    readTimeoutMs_ = timeoutMs;
}

uint32_t AIR780EPMTLSClient::readTimeout(void) const
{
    return readTimeoutMs_;
}

int32_t AIR780EPMTLSClient::lastError(void) const
{
    return lastError_;
}
