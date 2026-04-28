#include "AIR780EPMClient.h"

#include "arduino_tcp_io.h"

AIR780EPMClient::AIR780EPMClient() :
    socket_(-1),
    lastError_(0),
    connectTimeoutMs_(30000UL)
{
}

AIR780EPMClient::~AIR780EPMClient()
{
    stop();
}

int AIR780EPMClient::connect(IPAddress ip, uint16_t port)
{
    stop();
    socket_ = arduinoCoreTcpConnectIPv4(ip.raw_address(), port, connectTimeoutMs_, &lastError_);
    return (socket_ >= 0) ? 1 : 0;
}

int AIR780EPMClient::connect(IPAddress ip, uint16_t port, int32_t timeoutMs)
{
    const uint32_t previousTimeout = connectTimeoutMs_;
    connectTimeoutMs_ = (timeoutMs > 0) ? static_cast<uint32_t>(timeoutMs) : 1U;
    const int result = connect(ip, port);
    connectTimeoutMs_ = previousTimeout;
    return result;
}

int AIR780EPMClient::connect(const char *host, uint16_t port)
{
    stop();
    socket_ = arduinoCoreTcpConnectHost(host, port, connectTimeoutMs_, &lastError_);
    return (socket_ >= 0) ? 1 : 0;
}

int AIR780EPMClient::connect(const char *host, uint16_t port, int32_t timeoutMs)
{
    const uint32_t previousTimeout = connectTimeoutMs_;
    connectTimeoutMs_ = (timeoutMs > 0) ? static_cast<uint32_t>(timeoutMs) : 1U;
    const int result = connect(host, port);
    connectTimeoutMs_ = previousTimeout;
    return result;
}

size_t AIR780EPMClient::write(uint8_t value)
{
    return write(&value, 1U);
}

size_t AIR780EPMClient::write(const uint8_t *buffer, size_t size)
{
    int sent;

    if ((socket_ < 0) || (buffer == 0) || (size == 0U))
    {
        return 0U;
    }

    sent = arduinoCoreTcpWrite(socket_, buffer, static_cast<uint32_t>(size), 5000UL, &lastError_);
    if (sent < 0)
    {
        stop();
        return 0U;
    }

    return static_cast<size_t>(sent);
}

size_t AIR780EPMClient::write_P(PGM_P buffer, size_t size)
{
    return write(reinterpret_cast<const uint8_t *>(buffer), size);
}

int AIR780EPMClient::available(void)
{
    int result;

    if (socket_ < 0)
    {
        return 0;
    }

    result = arduinoCoreTcpAvailable(socket_, &lastError_);
    if (result < 0)
    {
        stop();
        return 0;
    }

    return result;
}

int AIR780EPMClient::read(void)
{
    uint8_t value = 0U;
    const int result = read(&value, 1U);

    if (result <= 0)
    {
        return -1;
    }

    return value;
}

int AIR780EPMClient::read(uint8_t *buffer, size_t size)
{
    int result;

    if ((socket_ < 0) || (buffer == 0) || (size == 0U))
    {
        return 0;
    }

    result = arduinoCoreTcpRead(socket_, buffer, static_cast<uint32_t>(size), 0, &lastError_);
    if (result < 0)
    {
        stop();
        return -1;
    }

    return result;
}

int AIR780EPMClient::peek(void)
{
    uint8_t value = 0U;
    int result;

    if (socket_ < 0)
    {
        return -1;
    }

    result = arduinoCoreTcpRead(socket_, &value, 1U, 1, &lastError_);
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

void AIR780EPMClient::flush(void)
{
}

void AIR780EPMClient::stop(void)
{
    if (socket_ >= 0)
    {
        arduinoCoreTcpClose(socket_);
        socket_ = -1;
    }
}

uint8_t AIR780EPMClient::connected(void)
{
    int result;

    if (socket_ < 0)
    {
        return 0U;
    }

    result = arduinoCoreTcpIsConnected(socket_, &lastError_);
    if (result <= 0)
    {
        stop();
        return 0U;
    }

    return 1U;
}

AIR780EPMClient::operator bool()
{
    return connected() != 0U;
}

void AIR780EPMClient::setConnectTimeout(uint32_t timeoutMs)
{
    connectTimeoutMs_ = timeoutMs;
}

void AIR780EPMClient::setConnectionTimeout(uint32_t timeoutMs)
{
    setConnectTimeout(timeoutMs);
}

uint32_t AIR780EPMClient::connectTimeout(void) const
{
    return connectTimeoutMs_;
}

int AIR780EPMClient::setNoDelay(bool noDelay)
{
    (void)noDelay;
    return 0;
}

bool AIR780EPMClient::getNoDelay(void) const
{
    return false;
}

int32_t AIR780EPMClient::lastError(void) const
{
    return lastError_;
}
