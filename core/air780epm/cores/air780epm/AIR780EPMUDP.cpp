#include "AIR780EPMUDP.h"

#include <string.h>

#include "arduino_udp_io.h"

AIR780EPMUDP::AIR780EPMUDP() :
    socket_(-1),
    lastError_(0),
    localPort_(0U),
    txRemoteIp_(),
    txRemotePort_(0U),
    packetOpen_(false),
    txBuffer_(),
    txLength_(0U),
    rxRemoteIp_(),
    rxRemotePort_(0U),
    rxBuffer_(),
    rxLength_(0U),
    rxOffset_(0U)
{
}

AIR780EPMUDP::~AIR780EPMUDP()
{
    stop();
}

uint8_t AIR780EPMUDP::begin(uint16_t port)
{
    stop();
    socket_ = arduinoCoreUdpOpen(port, &lastError_);
    if (socket_ < 0)
    {
        return 0U;
    }

    localPort_ = port;
    packetOpen_ = false;
    txLength_ = 0U;
    clearRxPacket();
    return 1U;
}

void AIR780EPMUDP::stop(void)
{
    if (socket_ >= 0)
    {
        arduinoCoreUdpClose(socket_);
        socket_ = -1;
    }

    localPort_ = 0U;
    packetOpen_ = false;
    txLength_ = 0U;
    clearRxPacket();
}

int AIR780EPMUDP::beginPacket(IPAddress ip, uint16_t port)
{
    if (!ensureOpen())
    {
        return 0;
    }

    txRemoteIp_ = ip;
    txRemotePort_ = port;
    txLength_ = 0U;
    packetOpen_ = true;
    return 1;
}

int AIR780EPMUDP::beginPacket(const char *host, uint16_t port)
{
    uint8_t address[4] = {0U, 0U, 0U, 0U};
    IPAddress ip;

    if ((host == NULL) || (host[0] == '\0'))
    {
        lastError_ = -22;
        return 0;
    }

    if (arduinoCoreUdpResolveHost(host, address, &lastError_) != 0)
    {
        return 0;
    }

    ip = address;
    return beginPacket(ip, port);
}

int AIR780EPMUDP::endPacket(void)
{
    int sent;

    if ((socket_ < 0) || !packetOpen_)
    {
        lastError_ = -22;
        return 0;
    }

    packetOpen_ = false;
    sent = arduinoCoreUdpSendToIPv4(socket_,
                                    txRemoteIp_.raw_address(),
                                    txRemotePort_,
                                    txBuffer_,
                                    static_cast<uint32_t>(txLength_),
                                    &lastError_);
    txLength_ = 0U;
    return (sent >= 0) ? 1 : 0;
}

size_t AIR780EPMUDP::write(uint8_t value)
{
    return write(&value, 1U);
}

size_t AIR780EPMUDP::write(const uint8_t *buffer, size_t size)
{
    size_t available;
    size_t copySize;

    if (!packetOpen_ || (buffer == NULL) || (size == 0U))
    {
        return 0U;
    }

    available = kMaxPacketSize - txLength_;
    copySize = (size < available) ? size : available;
    if (copySize == 0U)
    {
        lastError_ = -90;
        return 0U;
    }

    memcpy(txBuffer_ + txLength_, buffer, copySize);
    txLength_ += copySize;
    return copySize;
}

int AIR780EPMUDP::parsePacket(void)
{
    uint8_t remoteAddress[4] = {0U, 0U, 0U, 0U};
    uint16_t remotePort = 0U;
    int received;

    if (socket_ < 0)
    {
        return 0;
    }

    if (available() > 0)
    {
        return available();
    }

    clearRxPacket();
    received = arduinoCoreUdpParsePacket(socket_,
                                         rxBuffer_,
                                         static_cast<uint32_t>(sizeof(rxBuffer_)),
                                         remoteAddress,
                                         &remotePort,
                                         &lastError_);
    if (received <= 0)
    {
        return 0;
    }

    rxRemoteIp_ = remoteAddress;
    rxRemotePort_ = remotePort;
    rxLength_ = static_cast<size_t>(received);
    rxOffset_ = 0U;
    return received;
}

int AIR780EPMUDP::available(void)
{
    if (rxOffset_ >= rxLength_)
    {
        return 0;
    }

    return static_cast<int>(rxLength_ - rxOffset_);
}

int AIR780EPMUDP::read(void)
{
    if (available() <= 0)
    {
        return -1;
    }

    return rxBuffer_[rxOffset_++];
}

int AIR780EPMUDP::read(unsigned char *buffer, size_t size)
{
    size_t remaining;
    size_t copySize;

    if ((buffer == NULL) || (size == 0U) || (available() <= 0))
    {
        return 0;
    }

    remaining = rxLength_ - rxOffset_;
    copySize = (size < remaining) ? size : remaining;
    memcpy(buffer, rxBuffer_ + rxOffset_, copySize);
    rxOffset_ += copySize;
    return static_cast<int>(copySize);
}

int AIR780EPMUDP::peek(void)
{
    if (available() <= 0)
    {
        return -1;
    }

    return rxBuffer_[rxOffset_];
}

void AIR780EPMUDP::flush(void)
{
    clearRxPacket();
}

IPAddress AIR780EPMUDP::remoteIP(void)
{
    return rxRemoteIp_;
}

uint16_t AIR780EPMUDP::remotePort(void)
{
    return rxRemotePort_;
}

uint16_t AIR780EPMUDP::localPort(void) const
{
    return localPort_;
}

int32_t AIR780EPMUDP::lastError(void) const
{
    return lastError_;
}

void AIR780EPMUDP::clearRxPacket(void)
{
    rxLength_ = 0U;
    rxOffset_ = 0U;
    rxRemoteIp_ = IPAddress();
    rxRemotePort_ = 0U;
}

bool AIR780EPMUDP::ensureOpen(void)
{
    if (socket_ >= 0)
    {
        return true;
    }

    return begin(0U) != 0U;
}
