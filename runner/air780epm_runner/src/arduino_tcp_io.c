#include "arduino_tcp_io.h"

#include <stdio.h>
#include <string.h>

#include "commontypedef.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip_config_cat.h"

#define ARDUINO_TCP_DEFAULT_TIMEOUT_MS 30000UL

static void arduinoTcpSetError(int32_t *outError, int32_t error)
{
    if (outError != NULL)
    {
        *outError = error;
    }
}

static void arduinoTcpBuildTimeout(struct timeval *timeout, uint32_t timeoutMs)
{
    if (timeoutMs == 0UL)
    {
        timeoutMs = ARDUINO_TCP_DEFAULT_TIMEOUT_MS;
    }

    timeout->tv_sec = (long)(timeoutMs / 1000UL);
    timeout->tv_usec = (long)((timeoutMs % 1000UL) * 1000UL);
}

static int arduinoTcpWaitWritable(int socketId, uint32_t timeoutMs, int32_t *outError)
{
    fd_set writeSet;
    fd_set errorSet;
    struct timeval timeout;
    int result;

    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(socketId, &writeSet);
    FD_SET(socketId, &errorSet);
    arduinoTcpBuildTimeout(&timeout, timeoutMs);

    result = select(socketId + 1, NULL, &writeSet, &errorSet, &timeout);
    if (result <= 0)
    {
        arduinoTcpSetError(outError, (result == 0) ? -110 : sock_get_errno(socketId));
        return -1;
    }

    if (FD_ISSET(socketId, &errorSet))
    {
        const int error = sock_get_errno(socketId);
        arduinoTcpSetError(outError, error);
        return (error == 0) ? 0 : -1;
    }

    if (FD_ISSET(socketId, &writeSet))
    {
        arduinoTcpSetError(outError, 0);
        return 0;
    }

    arduinoTcpSetError(outError, -111);
    return -1;
}

static int arduinoTcpSetNonBlocking(int socketId, int32_t *outError)
{
    const int flags = fcntl(socketId, F_GETFL, 0);

    if (flags < 0)
    {
        arduinoTcpSetError(outError, sock_get_errno(socketId));
        return -1;
    }

    if (fcntl(socketId, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        arduinoTcpSetError(outError, sock_get_errno(socketId));
        return -1;
    }

    return 0;
}

static int arduinoTcpConnectAddress(const struct sockaddr *address, socklen_t addressLength, uint32_t timeoutMs, int32_t *outError)
{
    int socketId = socket(address->sa_family, SOCK_STREAM, IPPROTO_TCP);
    int result;

    if (socketId < 0)
    {
        arduinoTcpSetError(outError, socketId);
        return -1;
    }

    if (arduinoTcpSetNonBlocking(socketId, outError) != 0)
    {
        closesocket(socketId);
        return -1;
    }

    result = connect(socketId, address, addressLength);
    if (result == 0)
    {
        arduinoTcpSetError(outError, 0);
        return socketId;
    }

    if (sock_get_errno(socketId) != EINPROGRESS)
    {
        arduinoTcpSetError(outError, sock_get_errno(socketId));
        closesocket(socketId);
        return -1;
    }

    if (arduinoTcpWaitWritable(socketId, timeoutMs, outError) == 0)
    {
        arduinoTcpSetError(outError, 0);
        return socketId;
    }

    closesocket(socketId);
    return -1;
}

int arduinoCoreTcpConnectIPv4(const uint8_t ip[4], uint16_t port, uint32_t timeoutMs, int32_t *outError)
{
    struct sockaddr_in address;
    uint32_t rawAddress;

    if (ip == NULL)
    {
        arduinoTcpSetError(outError, -22);
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

    return arduinoTcpConnectAddress((const struct sockaddr *)&address, sizeof(address), timeoutMs, outError);
}

int arduinoCoreTcpConnectHost(const char *host, uint16_t port, uint32_t timeoutMs, int32_t *outError)
{
    struct addrinfo hints;
    struct addrinfo *addressList = NULL;
    struct addrinfo *entry;
    char portBuffer[8];
    int result;
    int socketId = -1;

    if ((host == NULL) || (host[0] == '\0'))
    {
        arduinoTcpSetError(outError, -22);
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    snprintf(portBuffer, sizeof(portBuffer), "%u", (unsigned int)port);

    result = getaddrinfowithcid(host, portBuffer, &hints, &addressList, LWIP_PS_INVALID_CID);
    if ((result != 0) || (addressList == NULL))
    {
        arduinoTcpSetError(outError, -1000 - result);
        return -1;
    }

    for (entry = addressList; entry != NULL; entry = entry->ai_next)
    {
        socketId = arduinoTcpConnectAddress(entry->ai_addr, entry->ai_addrlen, timeoutMs, outError);
        if (socketId >= 0)
        {
            break;
        }
    }

    freeaddrinfo(addressList);
    return socketId;
}

int arduinoCoreTcpWrite(int socketId, const uint8_t *buffer, uint32_t size, uint32_t timeoutMs, int32_t *outError)
{
    uint32_t sentTotal = 0U;

    if ((socketId < 0) || (buffer == NULL))
    {
        arduinoTcpSetError(outError, -22);
        return -1;
    }

    while (sentTotal < size)
    {
        int sent;
        if (arduinoTcpWaitWritable(socketId, timeoutMs, outError) != 0)
        {
            return (sentTotal > 0U) ? (int)sentTotal : -1;
        }

        sent = send(socketId, buffer + sentTotal, (size_t)(size - sentTotal), MSG_DONTWAIT);
        if (sent > 0)
        {
            sentTotal += (uint32_t)sent;
            continue;
        }

        if (sent == 0)
        {
            arduinoTcpSetError(outError, 0);
            return (int)sentTotal;
        }

        if ((sock_get_errno(socketId) == EWOULDBLOCK) || (sock_get_errno(socketId) == EAGAIN))
        {
            continue;
        }

        arduinoTcpSetError(outError, sock_get_errno(socketId));
        return (sentTotal > 0U) ? (int)sentTotal : -1;
    }

    arduinoTcpSetError(outError, 0);
    return (int)sentTotal;
}

int arduinoCoreTcpAvailable(int socketId, int32_t *outError)
{
    unsigned long count = 0UL;

    if (socketId < 0)
    {
        arduinoTcpSetError(outError, -22);
        return -1;
    }

    if (ioctlsocket(socketId, FIONREAD, &count) != 0)
    {
        arduinoTcpSetError(outError, sock_get_errno(socketId));
        return -1;
    }

    arduinoTcpSetError(outError, 0);
    if (count > 0x7FFFFFFFUL)
    {
        return 0x7FFFFFFF;
    }

    return (int)count;
}

int arduinoCoreTcpRead(int socketId, uint8_t *buffer, uint32_t size, int peek, int32_t *outError)
{
    int result;
    int flags = MSG_DONTWAIT;
    int error;

    if ((socketId < 0) || (buffer == NULL) || (size == 0U))
    {
        arduinoTcpSetError(outError, -22);
        return -1;
    }

    if (peek != 0)
    {
        flags |= MSG_PEEK;
    }

    result = recv(socketId, buffer, (size_t)size, flags);
    if (result > 0)
    {
        arduinoTcpSetError(outError, 0);
        return result;
    }

    if (result == 0)
    {
        arduinoTcpSetError(outError, 0);
        return 0;
    }

    error = sock_get_errno(socketId);
    if ((error == 0) || (error == EWOULDBLOCK) || (error == EAGAIN))
    {
        arduinoTcpSetError(outError, 0);
        return 0;
    }

    arduinoTcpSetError(outError, error);
    return -1;
}

int arduinoCoreTcpIsConnected(int socketId, int32_t *outError)
{
    uint8_t byte;
    int result;
    int error;

    if (socketId < 0)
    {
        arduinoTcpSetError(outError, -22);
        return 0;
    }

    result = recv(socketId, &byte, 1U, MSG_PEEK | MSG_DONTWAIT);
    if (result > 0)
    {
        arduinoTcpSetError(outError, 0);
        return 1;
    }

    if (result == 0)
    {
        arduinoTcpSetError(outError, 0);
        return 1;
    }

    error = sock_get_errno(socketId);
    if ((error == 0) || (error == EWOULDBLOCK) || (error == EAGAIN))
    {
        arduinoTcpSetError(outError, 0);
        return 1;
    }

    arduinoTcpSetError(outError, error);
    return socket_error_is_fatal(error) ? 0 : 1;
}

void arduinoCoreTcpClose(int socketId)
{
    if (socketId >= 0)
    {
        closesocket(socketId);
    }
}
