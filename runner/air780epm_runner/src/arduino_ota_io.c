#include "arduino_ota_io.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arduino_modem_io.h"
#include "common_api.h"
#include "luat_base.h"
#include "luat_http.h"
#include "luat_mem.h"
#include "luat_rtos.h"

#define ARDUINO_OTA_DEFAULT_TIMEOUT_MS 60000UL
#define ARDUINO_OTA_TASK_STACK_SIZE (8 * 1024)
#define ARDUINO_OTA_TASK_PRIORITY 45
#define ARDUINO_OTA_HTTP_SUCCESS_OK 200
#define ARDUINO_OTA_HTTP_SUCCESS_PARTIAL 206
#define ARDUINO_OTA_INTERNAL_CALLBACK_NOMEM (-90001)

enum {
    ARDUINO_OTA_EVENT_START = USER_EVENT_ID_START + 41,
    ARDUINO_OTA_EVENT_HEAD_DONE,
    ARDUINO_OTA_EVENT_BODY_DATA,
    ARDUINO_OTA_EVENT_BODY_DONE,
    ARDUINO_OTA_EVENT_HTTP_ERROR
};

typedef struct ArduinoCoreOtaContext_Tag {
    luat_rtos_mutex_t mutex;
    luat_rtos_task_handle taskHandle;
    luat_http_ctrl_t *http;
    uint32_t generation;
    uint32_t totalBytes;
    uint32_t downloadedBytes;
    uint32_t timeoutMs;
    int32_t lastError;
    int32_t lastPlatformError;
    int32_t httpStatusCode;
    uint8_t taskReady;
    uint8_t state;
    uint8_t insecure;
    char *url;
    char *user;
    char *password;
    char *caCert;
} ArduinoCoreOtaContext;

static ArduinoCoreOtaContext g_arduinoOta = {0};

static bool arduinoOtaEnsureMutex(void)
{
    if (g_arduinoOta.mutex == NULL) {
        if (luat_rtos_mutex_create(&g_arduinoOta.mutex) != 0) {
            return false;
        }
    }

    return g_arduinoOta.mutex != NULL;
}

static bool arduinoOtaLock(void)
{
    return arduinoOtaEnsureMutex() &&
           (luat_rtos_mutex_lock(g_arduinoOta.mutex, LUAT_WAIT_FOREVER) == 0);
}

static void arduinoOtaUnlock(void)
{
    if (g_arduinoOta.mutex != NULL) {
        (void)luat_rtos_mutex_unlock(g_arduinoOta.mutex);
    }
}

static char *arduinoOtaDupString(const char *value)
{
    size_t length;
    char *copy;

    if (value == NULL) {
        return NULL;
    }

    length = strlen(value) + 1U;
    copy = (char *)luat_heap_malloc(length);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, value, length);
    return copy;
}

static void arduinoOtaFreeString(char **slot)
{
    if ((slot != NULL) && (*slot != NULL)) {
        luat_heap_free(*slot);
        *slot = NULL;
    }
}

static void arduinoOtaFreeSessionStringsLocked(void)
{
    arduinoOtaFreeString(&g_arduinoOta.url);
    arduinoOtaFreeString(&g_arduinoOta.user);
    arduinoOtaFreeString(&g_arduinoOta.password);
    arduinoOtaFreeString(&g_arduinoOta.caCert);
}

static void arduinoOtaResetSessionLocked(void)
{
    arduinoOtaFreeSessionStringsLocked();
    g_arduinoOta.totalBytes = 0U;
    g_arduinoOta.downloadedBytes = 0U;
    g_arduinoOta.timeoutMs = ARDUINO_OTA_DEFAULT_TIMEOUT_MS;
    g_arduinoOta.lastError = ARDUINO_OTA_ERROR_NONE;
    g_arduinoOta.lastPlatformError = 0;
    g_arduinoOta.httpStatusCode = 0;
    g_arduinoOta.insecure = 0U;
    g_arduinoOta.state = ARDUINO_OTA_STATE_IDLE;
}

static bool arduinoOtaStateIsRunningLocked(void)
{
    return (g_arduinoOta.state == ARDUINO_OTA_STATE_STARTING) ||
           (g_arduinoOta.state == ARDUINO_OTA_STATE_DOWNLOADING) ||
           (g_arduinoOta.state == ARDUINO_OTA_STATE_VERIFYING) ||
           (g_arduinoOta.state == ARDUINO_OTA_STATE_APPLYING);
}

static bool arduinoOtaUrlLooksValid(const char *url)
{
    const char *host = NULL;
    size_t hostLength = 0U;

    if ((url == NULL) || (url[0] == '\0')) {
        return false;
    }

    if (strncmp(url, "http://", 7U) == 0) {
        host = url + 7U;
    }
    else if (strncmp(url, "https://", 8U) == 0) {
        host = url + 8U;
    }
    else {
        return false;
    }

    if ((host == NULL) || (host[0] == '\0') || (host[0] == '/')) {
        return false;
    }

    while ((host[hostLength] != '\0') && (host[hostLength] != '/')) {
        const unsigned char ch = (unsigned char)host[hostLength];
        if (ch <= 0x20U) {
            return false;
        }
        hostLength++;
    }

    return hostLength > 0U;
}

static bool arduinoOtaIsHttpsUrl(const char *url)
{
    return (url != NULL) && (strncmp(url, "https://", 8U) == 0);
}

static bool arduinoOtaHttpStatusOk(int32_t statusCode)
{
    return (statusCode == ARDUINO_OTA_HTTP_SUCCESS_OK) ||
           (statusCode == ARDUINO_OTA_HTTP_SUCCESS_PARTIAL);
}

static void arduinoOtaSetFailureLocked(uint8_t state,
                                       int32_t lastError,
                                       int32_t lastPlatformError)
{
    g_arduinoOta.state = state;
    g_arduinoOta.lastError = lastError;
    g_arduinoOta.lastPlatformError = lastPlatformError;
}

static void arduinoOtaCloseHttp(void)
{
    luat_http_ctrl_t *http = NULL;

    if (!arduinoOtaLock()) {
        return;
    }

    http = g_arduinoOta.http;
    g_arduinoOta.http = NULL;
    arduinoOtaUnlock();

    if (http != NULL) {
        luat_http_client_close(http);
        luat_http_client_destroy(&http);
    }
}

static char *arduinoOtaBuildUrlWithAuth(const char *url,
                                        const char *user,
                                        const char *password)
{
    const char *scheme = NULL;
    const char *rest = NULL;
    size_t schemeLength;
    size_t restLength;
    size_t userLength = 0U;
    size_t passwordLength = 0U;
    char *value = NULL;

    if (url == NULL) {
        return NULL;
    }

    if ((user == NULL) || (password == NULL)) {
        return arduinoOtaDupString(url);
    }

    if (strncmp(url, "https://", 8U) == 0) {
        scheme = "https://";
        rest = url + 8U;
    }
    else if (strncmp(url, "http://", 7U) == 0) {
        scheme = "http://";
        rest = url + 7U;
    }
    else {
        return NULL;
    }

    schemeLength = strlen(scheme);
    restLength = strlen(rest);
    userLength = strlen(user);
    passwordLength = strlen(password);
    value = (char *)luat_heap_malloc(schemeLength + userLength + passwordLength + restLength + 3U);
    if (value == NULL) {
        return NULL;
    }

    memcpy(value, scheme, schemeLength);
    memcpy(value + schemeLength, user, userLength);
    value[schemeLength + userLength] = ':';
    memcpy(value + schemeLength + userLength + 1U, password, passwordLength);
    value[schemeLength + userLength + passwordLength + 1U] = '@';
    memcpy(value + schemeLength + userLength + passwordLength + 2U, rest, restLength);
    value[schemeLength + userLength + passwordLength + restLength + 2U] = '\0';
    return value;
}

static void arduinoOtaFinalizeFailure(uint32_t generation,
                                      int32_t lastError,
                                      int32_t lastPlatformError,
                                      bool endFota)
{
    if (endFota) {
        (void)luat_fota_end(0U);
    }

    arduinoOtaCloseHttp();

    if (!arduinoOtaLock()) {
        return;
    }

    if (generation == g_arduinoOta.generation) {
        arduinoOtaSetFailureLocked(ARDUINO_OTA_STATE_ERROR,
                                   lastError,
                                   lastPlatformError);
    }
    arduinoOtaUnlock();
}

static void arduinoOtaHandleHeadDone(uint32_t generation)
{
    uint32_t totalBytes = 0U;
    int32_t statusCode = 0;
    luat_http_ctrl_t *http = NULL;

    if (!arduinoOtaLock()) {
        return;
    }

    if ((generation != g_arduinoOta.generation) ||
        ((g_arduinoOta.state != ARDUINO_OTA_STATE_STARTING) &&
         (g_arduinoOta.state != ARDUINO_OTA_STATE_DOWNLOADING))) {
        arduinoOtaUnlock();
        return;
    }

    http = g_arduinoOta.http;
    arduinoOtaUnlock();

    if (http == NULL) {
        arduinoOtaFinalizeFailure(generation,
                                  ARDUINO_OTA_ERROR_INTERNAL,
                                  0,
                                  true);
        return;
    }

    statusCode = luat_http_client_get_status_code(http);
    if (luat_http_client_get_context_len(http, &totalBytes) != 0) {
        totalBytes = 0U;
    }

    if (!arduinoOtaLock()) {
        return;
    }

    if (generation == g_arduinoOta.generation) {
        g_arduinoOta.httpStatusCode = statusCode;
        g_arduinoOta.totalBytes = totalBytes;
        if (arduinoOtaHttpStatusOk(statusCode)) {
            g_arduinoOta.state = ARDUINO_OTA_STATE_DOWNLOADING;
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_NONE;
            g_arduinoOta.lastPlatformError = 0;
        }
    }
    arduinoOtaUnlock();

    if (!arduinoOtaHttpStatusOk(statusCode)) {
        arduinoOtaFinalizeFailure(generation,
                                  ARDUINO_OTA_ERROR_HTTP_STATUS_ERROR,
                                  statusCode,
                                  true);
    }
}

static void arduinoOtaHandleBodyData(uint32_t generation, uint8_t *data, uint32_t length)
{
    int result;
    bool acceptData = false;

    if (!arduinoOtaLock()) {
        if (data != NULL) {
            luat_heap_free(data);
        }
        return;
    }

    if ((generation == g_arduinoOta.generation) &&
        ((g_arduinoOta.state == ARDUINO_OTA_STATE_STARTING) ||
         (g_arduinoOta.state == ARDUINO_OTA_STATE_DOWNLOADING)) &&
        arduinoOtaHttpStatusOk(g_arduinoOta.httpStatusCode)) {
        g_arduinoOta.state = ARDUINO_OTA_STATE_DOWNLOADING;
        acceptData = true;
    }
    arduinoOtaUnlock();

    if (!acceptData) {
        if (data != NULL) {
            luat_heap_free(data);
        }
        return;
    }

    result = luat_fota_write(data, length);
    luat_heap_free(data);

    if (result < 0) {
        arduinoOtaFinalizeFailure(generation,
                                  ARDUINO_OTA_ERROR_VERIFY_FAILED,
                                  result,
                                  true);
        return;
    }

    if (!arduinoOtaLock()) {
        return;
    }

    if (generation == g_arduinoOta.generation) {
        g_arduinoOta.downloadedBytes += length;
    }
    arduinoOtaUnlock();
}

static void arduinoOtaHandleBodyDone(uint32_t generation)
{
    int result;
    int finalizeResult = -1;

    if (!arduinoOtaLock()) {
        return;
    }

    if ((generation != g_arduinoOta.generation) ||
        !arduinoOtaHttpStatusOk(g_arduinoOta.httpStatusCode) ||
        ((g_arduinoOta.state != ARDUINO_OTA_STATE_STARTING) &&
         (g_arduinoOta.state != ARDUINO_OTA_STATE_DOWNLOADING))) {
        arduinoOtaUnlock();
        return;
    }

    g_arduinoOta.state = ARDUINO_OTA_STATE_VERIFYING;
    arduinoOtaUnlock();

    result = luat_fota_done();
    while (result > 0) {
        luat_rtos_task_sleep(100);
        result = luat_fota_done();
    }

    if (result == 0) {
        finalizeResult = luat_fota_end(1U);
    }

    arduinoOtaCloseHttp();

    if (!arduinoOtaLock()) {
        return;
    }

    if (generation == g_arduinoOta.generation) {
        if ((result == 0) && (finalizeResult == 0)) {
            g_arduinoOta.state = ARDUINO_OTA_STATE_STAGED;
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_NONE;
            g_arduinoOta.lastPlatformError = 0;
        }
        else {
            arduinoOtaSetFailureLocked(ARDUINO_OTA_STATE_ERROR,
                                       ARDUINO_OTA_ERROR_VERIFY_FAILED,
                                       (finalizeResult != 0) ? finalizeResult : result);
        }
    }
    arduinoOtaUnlock();
}

static void arduinoOtaHttpCallback(int status, void *data, uint32_t dataLen, void *userParam)
{
    ArduinoCoreOtaContext *context = (ArduinoCoreOtaContext *)userParam;
    uint32_t generation = 0U;
    uint8_t *bodyCopy = NULL;

    if ((context == NULL) || (context->taskHandle == NULL)) {
        return;
    }

    if (arduinoOtaLock()) {
        generation = context->generation;
        arduinoOtaUnlock();
    }

    if (status < 0) {
        (void)luat_rtos_event_send(context->taskHandle,
                                   ARDUINO_OTA_EVENT_HTTP_ERROR,
                                   (uint32_t)status,
                                   generation,
                                   0,
                                   0);
        return;
    }

    switch (status) {
        case HTTP_STATE_GET_HEAD:
            if (data == NULL) {
                (void)luat_rtos_event_send(context->taskHandle,
                                           ARDUINO_OTA_EVENT_HEAD_DONE,
                                           0,
                                           generation,
                                           0,
                                           0);
            }
            break;
        case HTTP_STATE_GET_BODY:
            if ((data != NULL) && (dataLen != 0U)) {
                bodyCopy = (uint8_t *)luat_heap_malloc(dataLen);
                if (bodyCopy == NULL) {
                    (void)luat_rtos_event_send(context->taskHandle,
                                               ARDUINO_OTA_EVENT_HTTP_ERROR,
                                               (uint32_t)ARDUINO_OTA_INTERNAL_CALLBACK_NOMEM,
                                               generation,
                                               0,
                                               0);
                    return;
                }
                memcpy(bodyCopy, data, dataLen);
                (void)luat_rtos_event_send(context->taskHandle,
                                           ARDUINO_OTA_EVENT_BODY_DATA,
                                           (uint32_t)bodyCopy,
                                           dataLen,
                                           generation,
                                           0);
            }
            else {
                (void)luat_rtos_event_send(context->taskHandle,
                                           ARDUINO_OTA_EVENT_BODY_DONE,
                                           0,
                                           generation,
                                           0,
                                           0);
            }
            break;
        default:
            break;
    }
}

static void arduinoOtaTask(void *param)
{
    luat_event_t event = {0};
    (void)param;

    while (1) {
        if (luat_rtos_event_recv(g_arduinoOta.taskHandle,
                                 0,
                                 &event,
                                 NULL,
                                 LUAT_WAIT_FOREVER) != 0) {
            continue;
        }

        switch (event.id) {
            case ARDUINO_OTA_EVENT_START: {
                luat_http_ctrl_t *http = NULL;
                char *requestUrl = NULL;
                char *url = NULL;
                char *user = NULL;
                char *password = NULL;
                char *caCert = NULL;
                uint32_t timeoutMs = ARDUINO_OTA_DEFAULT_TIMEOUT_MS;
                uint32_t generation = (uint32_t)event.param1;
                uint8_t insecure = 0U;
                bool useHttps = false;
                int result;

                if (!arduinoOtaLock()) {
                    break;
                }

                if ((generation != g_arduinoOta.generation) ||
                    (g_arduinoOta.state != ARDUINO_OTA_STATE_STARTING)) {
                    arduinoOtaUnlock();
                    break;
                }

                url = g_arduinoOta.url;
                user = g_arduinoOta.user;
                password = g_arduinoOta.password;
                caCert = g_arduinoOta.caCert;
                timeoutMs = g_arduinoOta.timeoutMs;
                insecure = g_arduinoOta.insecure;
                useHttps = arduinoOtaIsHttpsUrl(url);
                arduinoOtaUnlock();

                result = luat_fota_init(0U, 0U, NULL, NULL, 0U);
                if (result != 0) {
                    arduinoOtaFinalizeFailure(generation,
                                              ARDUINO_OTA_ERROR_INTERNAL,
                                              result,
                                              false);
                    break;
                }

                http = luat_http_client_create(arduinoOtaHttpCallback, &g_arduinoOta, -1);
                if (http == NULL) {
                    (void)luat_fota_end(0U);
                    arduinoOtaFinalizeFailure(generation,
                                              ARDUINO_OTA_ERROR_INTERNAL,
                                              0,
                                              false);
                    break;
                }

                if (timeoutMs == 0U) {
                    timeoutMs = ARDUINO_OTA_DEFAULT_TIMEOUT_MS;
                }
                (void)luat_http_client_base_config(http, timeoutMs, 0U, 2U);

                if (useHttps) {
                    const int sslMode = (caCert != NULL && caCert[0] != '\0' && insecure == 0U) ? 2 : 0;
                    result = luat_http_client_ssl_config(http,
                                                         sslMode,
                                                         (sslMode == 2) ? caCert : NULL,
                                                         (sslMode == 2) ? (uint32_t)(strlen(caCert) + 1U) : 0U,
                                                         NULL,
                                                         0U,
                                                         NULL,
                                                         0U,
                                                         NULL,
                                                         0U);
                    if (result != 0) {
                        luat_http_client_destroy(&http);
                        (void)luat_fota_end(0U);
                        arduinoOtaFinalizeFailure(generation,
                                                  ARDUINO_OTA_ERROR_INTERNAL,
                                                  result,
                                                  false);
                        break;
                    }
                }

                requestUrl = arduinoOtaBuildUrlWithAuth(url, user, password);
                if (requestUrl == NULL) {
                    luat_http_client_destroy(&http);
                    (void)luat_fota_end(0U);
                    arduinoOtaFinalizeFailure(generation,
                                              ARDUINO_OTA_ERROR_INTERNAL,
                                              0,
                                              false);
                    break;
                }

                if (!arduinoOtaLock()) {
                    luat_http_client_destroy(&http);
                    arduinoOtaFreeString(&requestUrl);
                    (void)luat_fota_end(0U);
                    break;
                }

                if (generation != g_arduinoOta.generation) {
                    arduinoOtaUnlock();
                    luat_http_client_destroy(&http);
                    arduinoOtaFreeString(&requestUrl);
                    (void)luat_fota_end(0U);
                    break;
                }

                g_arduinoOta.http = http;
                arduinoOtaUnlock();

                result = luat_http_client_start(http, requestUrl, 0U, 0U, 1U);
                arduinoOtaFreeString(&requestUrl);
                if (result != 0) {
                    (void)luat_fota_end(0U);
                    arduinoOtaFinalizeFailure(generation,
                                              arduinoOtaUrlLooksValid(url) ? ARDUINO_OTA_ERROR_DOWNLOAD_FAILED
                                                                           : ARDUINO_OTA_ERROR_INVALID_ARGUMENT,
                                              result,
                                              false);
                }
                break;
            }
            case ARDUINO_OTA_EVENT_HEAD_DONE:
                arduinoOtaHandleHeadDone((uint32_t)event.param2);
                break;
            case ARDUINO_OTA_EVENT_BODY_DATA:
                arduinoOtaHandleBodyData((uint32_t)event.param3,
                                         (uint8_t *)event.param1,
                                         (uint32_t)event.param2);
                break;
            case ARDUINO_OTA_EVENT_BODY_DONE:
                arduinoOtaHandleBodyDone((uint32_t)event.param2);
                break;
            case ARDUINO_OTA_EVENT_HTTP_ERROR: {
                const int32_t platformError = (int32_t)event.param1;
                const uint32_t generation = (uint32_t)event.param2;
                const int32_t lastError =
                    (platformError == ARDUINO_OTA_INTERNAL_CALLBACK_NOMEM) ?
                        ARDUINO_OTA_ERROR_INTERNAL :
                        ARDUINO_OTA_ERROR_DOWNLOAD_FAILED;
                arduinoOtaFinalizeFailure(generation,
                                          lastError,
                                          platformError,
                                          true);
                break;
            }
            default:
                break;
        }
    }
}

static bool arduinoOtaEnsureTask(void)
{
    int result;

    if (!arduinoOtaLock()) {
        return false;
    }

    if (g_arduinoOta.taskReady != 0U) {
        arduinoOtaUnlock();
        return true;
    }

    result = luat_rtos_task_create(&g_arduinoOta.taskHandle,
                                   ARDUINO_OTA_TASK_STACK_SIZE,
                                   ARDUINO_OTA_TASK_PRIORITY,
                                   "arduino_ota",
                                   arduinoOtaTask,
                                   NULL,
                                   16U);
    if (result == 0) {
        g_arduinoOta.taskReady = 1U;
    }
    arduinoOtaUnlock();
    return result == 0;
}

int arduinoCoreOtaBegin(const char *url, const ArduinoCoreOtaConfig *config)
{
    ArduinoCoreModemStatus status;
    const char *user = NULL;
    const char *password = NULL;
    const char *caCert = NULL;
    uint32_t timeoutMs = ARDUINO_OTA_DEFAULT_TIMEOUT_MS;
    uint8_t insecure = 0U;
    char *urlCopy = NULL;
    char *userCopy = NULL;
    char *passwordCopy = NULL;
    char *caCertCopy = NULL;
    uint32_t generation;

    if (!arduinoOtaUrlLooksValid(url)) {
        if (arduinoOtaLock()) {
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INVALID_ARGUMENT;
            arduinoOtaUnlock();
        }
        return 0;
    }

    if (config != NULL) {
        user = config->user;
        password = config->password;
        caCert = config->caCert;
        timeoutMs = (config->timeoutMs != 0U) ? config->timeoutMs : ARDUINO_OTA_DEFAULT_TIMEOUT_MS;
        insecure = config->insecure;
    }

    if ((user == NULL) != (password == NULL)) {
        if (arduinoOtaLock()) {
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INVALID_ARGUMENT;
            arduinoOtaUnlock();
        }
        return 0;
    }

    if (!arduinoOtaEnsureTask()) {
        if (arduinoOtaLock()) {
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INTERNAL;
            arduinoOtaUnlock();
        }
        return 0;
    }

    memset(&status, 0, sizeof(status));
    if (arduinoCoreModemGetStatus(&status) != 0) {
        if (arduinoOtaLock()) {
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INTERNAL;
            arduinoOtaUnlock();
        }
        return 0;
    }

    if (status.networkReady == 0U) {
        if (arduinoOtaLock()) {
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_NETWORK_NOT_READY;
            arduinoOtaUnlock();
        }
        return 0;
    }

    urlCopy = arduinoOtaDupString(url);
    if (urlCopy == NULL) {
        if (arduinoOtaLock()) {
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INTERNAL;
            arduinoOtaUnlock();
        }
        return 0;
    }

    if (user != NULL) {
        userCopy = arduinoOtaDupString(user);
        passwordCopy = arduinoOtaDupString(password);
        if ((userCopy == NULL) || (passwordCopy == NULL)) {
            arduinoOtaFreeString(&urlCopy);
            arduinoOtaFreeString(&userCopy);
            arduinoOtaFreeString(&passwordCopy);
            if (arduinoOtaLock()) {
                g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INTERNAL;
                arduinoOtaUnlock();
            }
            return 0;
        }
    }

    if (caCert != NULL) {
        caCertCopy = arduinoOtaDupString(caCert);
        if (caCertCopy == NULL) {
            arduinoOtaFreeString(&urlCopy);
            arduinoOtaFreeString(&userCopy);
            arduinoOtaFreeString(&passwordCopy);
            if (arduinoOtaLock()) {
                g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INTERNAL;
                arduinoOtaUnlock();
            }
            return 0;
        }
    }

    if (!arduinoOtaLock()) {
        arduinoOtaFreeString(&urlCopy);
        arduinoOtaFreeString(&userCopy);
        arduinoOtaFreeString(&passwordCopy);
        arduinoOtaFreeString(&caCertCopy);
        return 0;
    }

    if ((g_arduinoOta.state != ARDUINO_OTA_STATE_IDLE) &&
        (g_arduinoOta.state != ARDUINO_OTA_STATE_ERROR)) {
        g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INVALID_STATE;
        arduinoOtaUnlock();
        arduinoOtaFreeString(&urlCopy);
        arduinoOtaFreeString(&userCopy);
        arduinoOtaFreeString(&passwordCopy);
        arduinoOtaFreeString(&caCertCopy);
        return 0;
    }

    arduinoOtaResetSessionLocked();
    g_arduinoOta.url = urlCopy;
    g_arduinoOta.user = userCopy;
    g_arduinoOta.password = passwordCopy;
    g_arduinoOta.caCert = caCertCopy;
    g_arduinoOta.timeoutMs = timeoutMs;
    g_arduinoOta.insecure = insecure;
    g_arduinoOta.state = ARDUINO_OTA_STATE_STARTING;
    g_arduinoOta.lastError = ARDUINO_OTA_ERROR_NONE;
    g_arduinoOta.lastPlatformError = 0;
    g_arduinoOta.generation++;
    if (g_arduinoOta.generation == 0U) {
        g_arduinoOta.generation = 1U;
    }
    generation = g_arduinoOta.generation;
    arduinoOtaUnlock();

    if (luat_rtos_event_send(g_arduinoOta.taskHandle,
                             ARDUINO_OTA_EVENT_START,
                             generation,
                             0,
                             0,
                             0) != 0) {
        if (arduinoOtaLock()) {
            arduinoOtaResetSessionLocked();
            g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INTERNAL;
            g_arduinoOta.state = ARDUINO_OTA_STATE_ERROR;
            arduinoOtaUnlock();
        }
        return 0;
    }

    return 1;
}

int arduinoCoreOtaPoll(void)
{
    return arduinoCoreOtaState();
}

int arduinoCoreOtaState(void)
{
    int state = ARDUINO_OTA_STATE_ERROR;

    if (!arduinoOtaLock()) {
        return state;
    }

    state = g_arduinoOta.state;
    arduinoOtaUnlock();
    return state;
}

int arduinoCoreOtaIsRunning(void)
{
    int running = 0;

    if (!arduinoOtaLock()) {
        return running;
    }

    running = arduinoOtaStateIsRunningLocked() ? 1 : 0;
    arduinoOtaUnlock();
    return running;
}

int arduinoCoreOtaIsStaged(void)
{
    int staged = 0;

    if (!arduinoOtaLock()) {
        return staged;
    }

    staged = (g_arduinoOta.state == ARDUINO_OTA_STATE_STAGED) ? 1 : 0;
    arduinoOtaUnlock();
    return staged;
}

uint32_t arduinoCoreOtaTotalBytes(void)
{
    uint32_t totalBytes = 0U;

    if (!arduinoOtaLock()) {
        return totalBytes;
    }

    totalBytes = g_arduinoOta.totalBytes;
    arduinoOtaUnlock();
    return totalBytes;
}

uint32_t arduinoCoreOtaDownloadedBytes(void)
{
    uint32_t downloadedBytes = 0U;

    if (!arduinoOtaLock()) {
        return downloadedBytes;
    }

    downloadedBytes = g_arduinoOta.downloadedBytes;
    arduinoOtaUnlock();
    return downloadedBytes;
}

int32_t arduinoCoreOtaLastError(void)
{
    int32_t lastError = ARDUINO_OTA_ERROR_INTERNAL;

    if (!arduinoOtaLock()) {
        return lastError;
    }

    lastError = g_arduinoOta.lastError;
    arduinoOtaUnlock();
    return lastError;
}

bool arduinoCoreOtaApply(void)
{
    if (!arduinoOtaLock()) {
        return false;
    }

    if (g_arduinoOta.state != ARDUINO_OTA_STATE_STAGED) {
        g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INVALID_STATE;
        arduinoOtaUnlock();
        return false;
    }

    g_arduinoOta.state = ARDUINO_OTA_STATE_APPLYING;
    arduinoOtaUnlock();

    luat_os_reboot(0);
    return true;
}

bool arduinoCoreOtaClear(void)
{
    if (!arduinoOtaLock()) {
        return false;
    }

    if (arduinoOtaStateIsRunningLocked() ||
        (g_arduinoOta.state == ARDUINO_OTA_STATE_STAGED)) {
        g_arduinoOta.lastError = ARDUINO_OTA_ERROR_INVALID_STATE;
        arduinoOtaUnlock();
        return false;
    }

    arduinoOtaResetSessionLocked();
    arduinoOtaUnlock();
    return true;
}
