#include "arduino_tls_io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip_config_cat.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "luat_crypto.h"

#define ARDUINO_TLS_DEFAULT_CONNECT_TIMEOUT_MS 60000UL
#define ARDUINO_TLS_DEFAULT_IO_TIMEOUT_MS 5000UL
#define ARDUINO_TLS_ERR_BAD_ARG (-70001)
#define ARDUINO_TLS_ERR_NO_MEMORY (-70002)
#define ARDUINO_TLS_ERR_MISSING_TRUST (-70003)

typedef struct ArduinoCoreTlsHandle {
    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt caCert;
    int32_t lastError;
    uint8_t connected;
    uint8_t hasPeek;
    uint8_t peekByte;
} ArduinoCoreTlsHandle;

static void arduinoTlsNetInit(mbedtls_net_context *context)
{
    if (context != NULL)
    {
        context->fd = -1;
    }
}

static void arduinoTlsNetFree(mbedtls_net_context *context)
{
    if ((context != NULL) && (context->fd >= 0))
    {
        closesocket(context->fd);
        context->fd = -1;
    }
}

static int arduinoTlsNetSetNonblock(mbedtls_net_context *context)
{
    int flags;

    if ((context == NULL) || (context->fd < 0))
    {
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    }

    flags = fcntl(context->fd, F_GETFL, 0);
    if (flags < 0)
    {
        return MBEDTLS_ERR_NET_SOCKET_FAILED;
    }

    if (fcntl(context->fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return MBEDTLS_ERR_NET_SOCKET_FAILED;
    }

    return 0;
}

static int arduinoTlsNetPoll(mbedtls_net_context *context, uint32_t rw, uint32_t timeoutMs)
{
    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;
    struct timeval timeout;
    struct timeval *timeoutPtr = NULL;
    int result;
    int readyMask = 0;

    if ((context == NULL) || (context->fd < 0))
    {
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    }

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);

    if ((rw & MBEDTLS_NET_POLL_READ) != 0U)
    {
        FD_SET(context->fd, &readSet);
    }
    if ((rw & MBEDTLS_NET_POLL_WRITE) != 0U)
    {
        FD_SET(context->fd, &writeSet);
    }
    FD_SET(context->fd, &errorSet);

    if (timeoutMs != UINT32_MAX)
    {
        timeout.tv_sec = (long)(timeoutMs / 1000UL);
        timeout.tv_usec = (long)((timeoutMs % 1000UL) * 1000UL);
        timeoutPtr = &timeout;
    }

    result = select(context->fd + 1,
                    ((rw & MBEDTLS_NET_POLL_READ) != 0U) ? &readSet : NULL,
                    ((rw & MBEDTLS_NET_POLL_WRITE) != 0U) ? &writeSet : NULL,
                    &errorSet,
                    timeoutPtr);
    if (result < 0)
    {
        return MBEDTLS_ERR_NET_POLL_FAILED;
    }

    if (result == 0)
    {
        return 0;
    }

    if (FD_ISSET(context->fd, &errorSet))
    {
        return MBEDTLS_ERR_NET_POLL_FAILED;
    }
    if (FD_ISSET(context->fd, &readSet))
    {
        readyMask |= MBEDTLS_NET_POLL_READ;
    }
    if (FD_ISSET(context->fd, &writeSet))
    {
        readyMask |= MBEDTLS_NET_POLL_WRITE;
    }

    return readyMask;
}

static int arduinoTlsNetConnect(mbedtls_net_context *context,
                                const char *host,
                                const char *port,
                                uint32_t timeoutMs)
{
    struct addrinfo hints;
    struct addrinfo *addressList = NULL;
    struct addrinfo *entry;
    int result;

    if ((context == NULL) || (host == NULL) || (port == NULL))
    {
        return MBEDTLS_ERR_NET_BAD_INPUT_DATA;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfowithcid(host, port, &hints, &addressList, LWIP_PS_INVALID_CID);
    if ((result != 0) || (addressList == NULL))
    {
        return MBEDTLS_ERR_NET_UNKNOWN_HOST;
    }

    result = MBEDTLS_ERR_NET_CONNECT_FAILED;
    for (entry = addressList; entry != NULL; entry = entry->ai_next)
    {
        context->fd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
        if (context->fd < 0)
        {
            result = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }

        result = arduinoTlsNetSetNonblock(context);
        if (result != 0)
        {
            arduinoTlsNetFree(context);
            continue;
        }

        if (connect(context->fd, entry->ai_addr, entry->ai_addrlen) == 0)
        {
            result = 0;
            break;
        }

        if (sock_get_errno(context->fd) == EINPROGRESS)
        {
            result = arduinoTlsNetPoll(context, MBEDTLS_NET_POLL_WRITE, timeoutMs);
            if ((result & MBEDTLS_NET_POLL_WRITE) != 0)
            {
                result = 0;
                break;
            }

            if (result == 0)
            {
                result = MBEDTLS_ERR_SSL_TIMEOUT;
            }
            else if (result > 0)
            {
                result = MBEDTLS_ERR_NET_CONNECT_FAILED;
            }
        }
        else
        {
            result = MBEDTLS_ERR_NET_CONNECT_FAILED;
        }

        arduinoTlsNetFree(context);
    }

    freeaddrinfo(addressList);
    return result;
}

static int arduinoTlsNetRecv(void *contextPtr, unsigned char *buffer, size_t length)
{
    mbedtls_net_context *context = (mbedtls_net_context *)contextPtr;
    int result;
    int error;

    if ((context == NULL) || (context->fd < 0) || (buffer == NULL))
    {
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    }

    result = recv(context->fd, buffer, length, MSG_DONTWAIT);
    if (result > 0)
    {
        return result;
    }

    if (result == 0)
    {
        return MBEDTLS_ERR_NET_CONN_RESET;
    }

    error = sock_get_errno(context->fd);
    if ((error == EWOULDBLOCK) || (error == EAGAIN))
    {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return MBEDTLS_ERR_NET_RECV_FAILED;
}

static int arduinoTlsNetRecvTimeout(void *contextPtr,
                                    unsigned char *buffer,
                                    size_t length,
                                    uint32_t timeoutMs)
{
    mbedtls_net_context *context = (mbedtls_net_context *)contextPtr;
    int pollResult;

    pollResult = arduinoTlsNetPoll(context, MBEDTLS_NET_POLL_READ, timeoutMs);
    if (pollResult < 0)
    {
        return pollResult;
    }
    if ((pollResult & MBEDTLS_NET_POLL_READ) == 0)
    {
        return MBEDTLS_ERR_SSL_TIMEOUT;
    }

    return arduinoTlsNetRecv(contextPtr, buffer, length);
}

static int arduinoTlsNetSend(void *contextPtr, const unsigned char *buffer, size_t length)
{
    mbedtls_net_context *context = (mbedtls_net_context *)contextPtr;
    int result;
    int error;

    if ((context == NULL) || (context->fd < 0) || (buffer == NULL))
    {
        return MBEDTLS_ERR_NET_INVALID_CONTEXT;
    }

    result = send(context->fd, buffer, length, MSG_DONTWAIT);
    if (result >= 0)
    {
        return result;
    }

    error = sock_get_errno(context->fd);
    if ((error == EWOULDBLOCK) || (error == EAGAIN))
    {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    return MBEDTLS_ERR_NET_SEND_FAILED;
}

static uint32_t arduinoTlsMillis(void)
{
    const uint32_t frequency = osKernelGetTickFreq();

    if (frequency == 0U)
    {
        return 0U;
    }

    return (uint32_t)(((uint64_t)osKernelGetTickCount() * 1000ULL) / frequency);
}

static uint8_t arduinoTlsTimedOut(uint32_t startMs, uint32_t timeoutMs)
{
    if (timeoutMs == 0UL)
    {
        timeoutMs = ARDUINO_TLS_DEFAULT_CONNECT_TIMEOUT_MS;
    }

    return ((uint32_t)(arduinoTlsMillis() - startMs) >= timeoutMs) ? 1U : 0U;
}

static int arduinoTlsRandom(void *context, unsigned char *output, size_t outputLength)
{
    (void)context;

    if ((output == NULL) || (outputLength == 0U))
    {
        return 0;
    }

    if (luat_crypto_trng((char *)output, outputLength) == 0)
    {
        return 0;
    }

    while (outputLength > 0U)
    {
        *output++ = (unsigned char)rand();
        outputLength--;
    }

    return 0;
}

static void arduinoTlsSetError(int32_t *outError, int32_t error)
{
    if (outError != NULL)
    {
        *outError = error;
    }
}

static void arduinoTlsHandleSetError(ArduinoCoreTlsHandle *handle, int32_t *outError, int32_t error)
{
    if (handle != NULL)
    {
        handle->lastError = error;
    }

    arduinoTlsSetError(outError, error);
}

static uint32_t arduinoTlsEffectiveConnectTimeout(const ArduinoCoreTlsConfig *config)
{
    if ((config == NULL) || (config->connectTimeoutMs == 0UL))
    {
        return ARDUINO_TLS_DEFAULT_CONNECT_TIMEOUT_MS;
    }

    return config->connectTimeoutMs;
}

static uint32_t arduinoTlsEffectiveIoTimeout(const ArduinoCoreTlsConfig *config)
{
    if ((config == NULL) || (config->ioTimeoutMs == 0UL))
    {
        return ARDUINO_TLS_DEFAULT_IO_TIMEOUT_MS;
    }

    return config->ioTimeoutMs;
}

static void arduinoTlsInit(ArduinoCoreTlsHandle *handle)
{
    memset(handle, 0, sizeof(*handle));
    arduinoTlsNetInit(&handle->net);
    mbedtls_ssl_init(&handle->ssl);
    mbedtls_ssl_config_init(&handle->config);
    mbedtls_x509_crt_init(&handle->caCert);
}

static void arduinoTlsFree(ArduinoCoreTlsHandle *handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->connected != 0U)
    {
        (void)mbedtls_ssl_close_notify(&handle->ssl);
    }

    arduinoTlsNetFree(&handle->net);
    mbedtls_x509_crt_free(&handle->caCert);
    mbedtls_ssl_free(&handle->ssl);
    mbedtls_ssl_config_free(&handle->config);
    free(handle);
}

static int arduinoTlsConfigure(ArduinoCoreTlsHandle *handle, const char *host, const ArduinoCoreTlsConfig *config)
{
    int result;
    const uint32_t ioTimeout = arduinoTlsEffectiveIoTimeout(config);
    const int authMode = ((config != NULL) && (config->insecure != 0U)) ? MBEDTLS_SSL_VERIFY_NONE : MBEDTLS_SSL_VERIFY_REQUIRED;

    if (authMode == MBEDTLS_SSL_VERIFY_REQUIRED)
    {
        if ((config == NULL) || (config->caCert == NULL) || (config->caCertLen == 0UL))
        {
            return ARDUINO_TLS_ERR_MISSING_TRUST;
        }

        result = mbedtls_x509_crt_parse(&handle->caCert,
                                        (const unsigned char *)config->caCert,
                                        (size_t)config->caCertLen);
        if (result != 0)
        {
            return result;
        }
    }

    result = mbedtls_ssl_config_defaults(&handle->config,
                                         MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT);
    if (result != 0)
    {
        return result;
    }

    mbedtls_ssl_conf_authmode(&handle->config, authMode);
    mbedtls_ssl_conf_ca_chain(&handle->config, &handle->caCert, NULL);
    mbedtls_ssl_conf_min_version(&handle->config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&handle->config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_rng(&handle->config, arduinoTlsRandom, NULL);
    mbedtls_ssl_conf_read_timeout(&handle->config, ioTimeout);

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    result = mbedtls_ssl_conf_max_frag_len(&handle->config, MBEDTLS_SSL_MAX_FRAG_LEN_4096);
    if (result != 0)
    {
        return result;
    }
#endif

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    mbedtls_ssl_conf_session_tickets(&handle->config, MBEDTLS_SSL_SESSION_TICKETS_ENABLED);
#endif

    result = mbedtls_ssl_setup(&handle->ssl, &handle->config);
    if (result != 0)
    {
        return result;
    }

    if ((host != NULL) && ((authMode == MBEDTLS_SSL_VERIFY_REQUIRED) || ((config != NULL) && (config->sni != 0U))))
    {
        result = mbedtls_ssl_set_hostname(&handle->ssl, host);
        if (result != 0)
        {
            return result;
        }
    }

    mbedtls_ssl_set_bio(&handle->ssl,
                        &handle->net,
                        arduinoTlsNetSend,
                        arduinoTlsNetRecv,
                        arduinoTlsNetRecvTimeout);

    return 0;
}

static int arduinoTlsHandshake(ArduinoCoreTlsHandle *handle, uint32_t timeoutMs)
{
    int result;
    const uint32_t startMs = arduinoTlsMillis();

    while ((result = mbedtls_ssl_handshake(&handle->ssl)) != 0)
    {
        if ((result != MBEDTLS_ERR_SSL_WANT_READ) && (result != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            return result;
        }

        if (arduinoTlsTimedOut(startMs, timeoutMs) != 0U)
        {
            return MBEDTLS_ERR_SSL_TIMEOUT;
        }

        (void)osDelay(1U);
    }

    if (mbedtls_ssl_get_verify_result(&handle->ssl) != 0)
    {
        return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
    }

    return 0;
}

void *arduinoCoreTlsConnectHost(const char *host, uint16_t port, const ArduinoCoreTlsConfig *config, int32_t *outError)
{
    ArduinoCoreTlsHandle *handle;
    char portBuffer[8];
    int result;
    const uint32_t connectTimeout = arduinoTlsEffectiveConnectTimeout(config);

    if ((host == NULL) || (host[0] == '\0'))
    {
        arduinoTlsSetError(outError, ARDUINO_TLS_ERR_BAD_ARG);
        return NULL;
    }

    handle = (ArduinoCoreTlsHandle *)malloc(sizeof(ArduinoCoreTlsHandle));
    if (handle == NULL)
    {
        arduinoTlsSetError(outError, ARDUINO_TLS_ERR_NO_MEMORY);
        return NULL;
    }

    arduinoTlsInit(handle);

    result = arduinoTlsConfigure(handle, host, config);
    if (result != 0)
    {
        arduinoTlsHandleSetError(handle, outError, result);
        arduinoTlsFree(handle);
        return NULL;
    }

    snprintf(portBuffer, sizeof(portBuffer), "%u", (unsigned int)port);
    result = arduinoTlsNetConnect(&handle->net, host, portBuffer, connectTimeout);
    if (result != 0)
    {
        arduinoTlsHandleSetError(handle, outError, result);
        arduinoTlsFree(handle);
        return NULL;
    }

    result = arduinoTlsHandshake(handle, connectTimeout);
    if (result != 0)
    {
        arduinoTlsHandleSetError(handle, outError, result);
        arduinoTlsFree(handle);
        return NULL;
    }

    (void)arduinoTlsNetSetNonblock(&handle->net);
    handle->connected = 1U;
    arduinoTlsHandleSetError(handle, outError, 0);
    return handle;
}

int arduinoCoreTlsWrite(void *tlsHandle, const uint8_t *buffer, uint32_t size, uint32_t timeoutMs, int32_t *outError)
{
    ArduinoCoreTlsHandle *handle = (ArduinoCoreTlsHandle *)tlsHandle;
    uint32_t written = 0U;
    const uint32_t startMs = arduinoTlsMillis();

    if ((handle == NULL) || (buffer == NULL))
    {
        arduinoTlsSetError(outError, ARDUINO_TLS_ERR_BAD_ARG);
        return -1;
    }

    while (written < size)
    {
        const int result = mbedtls_ssl_write(&handle->ssl, buffer + written, (size_t)(size - written));
        if (result > 0)
        {
            written += (uint32_t)result;
            continue;
        }

        if ((result == MBEDTLS_ERR_SSL_WANT_READ) || (result == MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            if (arduinoTlsTimedOut(startMs, timeoutMs) != 0U)
            {
                arduinoTlsHandleSetError(handle, outError, MBEDTLS_ERR_SSL_TIMEOUT);
                return (written > 0U) ? (int)written : -1;
            }
            (void)osDelay(1U);
            continue;
        }

        arduinoTlsHandleSetError(handle, outError, result);
        handle->connected = 0U;
        return (written > 0U) ? (int)written : -1;
    }

    arduinoTlsHandleSetError(handle, outError, 0);
    return (int)written;
}

int arduinoCoreTlsAvailable(void *tlsHandle, int32_t *outError)
{
    ArduinoCoreTlsHandle *handle = (ArduinoCoreTlsHandle *)tlsHandle;
    size_t pending;
    int pollResult;

    if (handle == NULL)
    {
        arduinoTlsSetError(outError, ARDUINO_TLS_ERR_BAD_ARG);
        return -1;
    }

    if (handle->hasPeek != 0U)
    {
        arduinoTlsHandleSetError(handle, outError, 0);
        return 1;
    }

    pending = mbedtls_ssl_get_bytes_avail(&handle->ssl);
    if (pending > 0U)
    {
        arduinoTlsHandleSetError(handle, outError, 0);
        return (pending > 0x7FFFFFFFU) ? 0x7FFFFFFF : (int)pending;
    }

    pollResult = arduinoTlsNetPoll(&handle->net, MBEDTLS_NET_POLL_READ, 0U);
    if ((pollResult & MBEDTLS_NET_POLL_READ) != 0)
    {
        arduinoTlsHandleSetError(handle, outError, 0);
        return 1;
    }

    if (pollResult < 0)
    {
        arduinoTlsHandleSetError(handle, outError, pollResult);
        return -1;
    }

    arduinoTlsHandleSetError(handle, outError, 0);
    return 0;
}

int arduinoCoreTlsRead(void *tlsHandle, uint8_t *buffer, uint32_t size, int peek, int32_t *outError)
{
    ArduinoCoreTlsHandle *handle = (ArduinoCoreTlsHandle *)tlsHandle;
    uint32_t copied = 0U;
    int result;

    if ((handle == NULL) || (buffer == NULL) || (size == 0U))
    {
        arduinoTlsSetError(outError, ARDUINO_TLS_ERR_BAD_ARG);
        return -1;
    }

    if (handle->hasPeek != 0U)
    {
        buffer[0] = handle->peekByte;
        if (peek == 0)
        {
            handle->hasPeek = 0U;
        }
        arduinoTlsHandleSetError(handle, outError, 0);
        return 1;
    }

    result = mbedtls_ssl_read(&handle->ssl, buffer, (size_t)size);
    if (result > 0)
    {
        if (peek != 0)
        {
            handle->peekByte = buffer[0];
            handle->hasPeek = 1U;
            copied = 1U;
        }
        else
        {
            copied = (uint32_t)result;
        }

        arduinoTlsHandleSetError(handle, outError, 0);
        return (int)copied;
    }

    if ((result == MBEDTLS_ERR_SSL_WANT_READ) ||
        (result == MBEDTLS_ERR_SSL_WANT_WRITE) ||
        (result == MBEDTLS_ERR_SSL_TIMEOUT))
    {
        arduinoTlsHandleSetError(handle, outError, 0);
        return 0;
    }

    if ((result == 0) || (result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY))
    {
        handle->connected = 0U;
        arduinoTlsHandleSetError(handle, outError, 0);
        return -1;
    }

    handle->connected = 0U;
    arduinoTlsHandleSetError(handle, outError, result);
    return -1;
}

int arduinoCoreTlsIsConnected(void *tlsHandle, int32_t *outError)
{
    ArduinoCoreTlsHandle *handle = (ArduinoCoreTlsHandle *)tlsHandle;

    if (handle == NULL)
    {
        arduinoTlsSetError(outError, ARDUINO_TLS_ERR_BAD_ARG);
        return 0;
    }

    arduinoTlsHandleSetError(handle, outError, 0);
    return (handle->connected != 0U) ? 1 : 0;
}

void arduinoCoreTlsClose(void *tlsHandle)
{
    arduinoTlsFree((ArduinoCoreTlsHandle *)tlsHandle);
}
