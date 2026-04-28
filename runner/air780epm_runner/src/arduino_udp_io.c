#include "arduino_udp_io.h"

#include <string.h>

#include "commontypedef.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip_config_cat.h"

static void arduinoUdpSetError(int32_t *outError, int32_t error)
{
    if (outError != NULL)
    {
        *outError = error;
    }
}

static int arduinoUdpSetNonBlocking(int socketId, int32_t *outError)
{
    const int flags = fcntl(socketId, F_GETFL, 0);

    if (flags < 0)
    {
        arduinoUdpSetError(outError, sock_get_errno(socketId));
        return -1;
    }

    if (fcntl(socketId, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        arduinoUdpSetError(outError, sock_get_errno(socketId));
        return -1;
    }

    return 0;
}

static void arduinoUdpStoreIPv4(uint32_t networkOrderAddress, uint8_t outIp[4])
{
    const uint32_t raw = ntohl(networkOrderAddress);

    outIp[0] = (uint8_t)((raw >> 24) & 0xFFU);
    outIp[1] = (uint8_t)((raw >> 16) & 0xFFU);
    outIp[2] = (uint8_t)((raw >> 8) & 0xFFU);
    outIp[3] = (uint8_t)(raw & 0xFFU);
}

int arduinoCoreUdpOpen(uint16_t localPort, int32_t *outError)
{
    struct sockaddr_in address;
    int socketId = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketId < 0)
    {
        arduinoUdpSetError(outError, socketId);
        return -1;
    }

    if (arduinoUdpSetNonBlocking(socketId, outError) != 0)
    {
        closesocket(socketId);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(localPort);
    address.sin_addr.s_addr = htonl(0UL);

    if (bind(socketId, (const struct sockaddr *)&address, sizeof(address)) != 0)
    {
        arduinoUdpSetError(outError, sock_get_errno(socketId));
        closesocket(socketId);
        return -1;
    }

    arduinoUdpSetError(outError, 0);
    return socketId;
}

void arduinoCoreUdpClose(int socketId)
{
    if (socketId >= 0)
    {
        closesocket(socketId);
    }
}

int arduinoCoreUdpResolveHost(const char *host, uint8_t outIp[4], int32_t *outError)
{
    struct addrinfo hints;
    struct addrinfo *addressList = NULL;
    struct addrinfo *entry;
    int result;

    if ((host == NULL) || (host[0] == '\0') || (outIp == NULL))
    {
        arduinoUdpSetError(outError, -22);
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    result = getaddrinfowithcid(host, NULL, &hints, &addressList, LWIP_PS_INVALID_CID);
    if ((result != 0) || (addressList == NULL))
    {
        arduinoUdpSetError(outError, -1000 - result);
        return -1;
    }

    for (entry = addressList; entry != NULL; entry = entry->ai_next)
    {
        if ((entry->ai_family == AF_INET) && (entry->ai_addr != NULL))
        {
            const struct sockaddr_in *address = (const struct sockaddr_in *)entry->ai_addr;
            arduinoUdpStoreIPv4(address->sin_addr.s_addr, outIp);
            freeaddrinfo(addressList);
            arduinoUdpSetError(outError, 0);
            return 0;
        }
    }

    freeaddrinfo(addressList);
    arduinoUdpSetError(outError, -2);
    return -1;
}

int arduinoCoreUdpSendToIPv4(int socketId,
                             const uint8_t ip[4],
                             uint16_t port,
                             const uint8_t *buffer,
                             uint32_t size,
                             int32_t *outError)
{
    struct sockaddr_in address;
    uint32_t rawAddress;
    int sent;

    if ((socketId < 0) || (ip == NULL) || ((buffer == NULL) && (size > 0U)))
    {
        arduinoUdpSetError(outError, -22);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    rawAddress = ((uint32_t)ip[0] << 24) |
                 ((uint32_t)ip[1] << 16) |
                 ((uint32_t)ip[2] << 8) |
                 ((uint32_t)ip[3]);
    address.sin_addr.s_addr = htonl(rawAddress);

    sent = sendto(socketId, buffer, (size_t)size, 0, (const struct sockaddr *)&address, sizeof(address));
    if (sent < 0)
    {
        arduinoUdpSetError(outError, sock_get_errno(socketId));
        return -1;
    }

    arduinoUdpSetError(outError, 0);
    return sent;
}

int arduinoCoreUdpParsePacket(int socketId,
                              uint8_t *buffer,
                              uint32_t bufferSize,
                              uint8_t remoteIp[4],
                              uint16_t *remotePort,
                              int32_t *outError)
{
    struct sockaddr_in remoteAddress;
    socklen_t remoteLength = sizeof(remoteAddress);
    int received;

    if ((socketId < 0) || (buffer == NULL) || (bufferSize == 0U) || (remoteIp == NULL) || (remotePort == NULL))
    {
        arduinoUdpSetError(outError, -22);
        return -1;
    }

    memset(&remoteAddress, 0, sizeof(remoteAddress));
    received = recvfrom(socketId,
                        buffer,
                        (size_t)bufferSize,
                        MSG_DONTWAIT,
                        (struct sockaddr *)&remoteAddress,
                        &remoteLength);
    if (received > 0)
    {
        arduinoUdpStoreIPv4(remoteAddress.sin_addr.s_addr, remoteIp);
        *remotePort = ntohs(remoteAddress.sin_port);
        arduinoUdpSetError(outError, 0);
        return received;
    }

    if (received == 0)
    {
        arduinoUdpSetError(outError, 0);
        return 0;
    }

    if ((sock_get_errno(socketId) == EWOULDBLOCK) || (sock_get_errno(socketId) == EAGAIN))
    {
        arduinoUdpSetError(outError, 0);
        return 0;
    }

    arduinoUdpSetError(outError, sock_get_errno(socketId));
    return -1;
}
