#ifndef AIR780EPM_OTA_H
#define AIR780EPM_OTA_H

#include <stddef.h>
#include <stdint.h>

struct AIR780EPMOTAConfig {
    uint32_t timeoutMs;
    const char *user;
    const char *password;
    const char *caCert;
    bool insecure;

    AIR780EPMOTAConfig()
        : timeoutMs(60000UL)
        , user(NULL)
        , password(NULL)
        , caCert(NULL)
        , insecure(false)
    {
    }
};

class AIR780EPMOTAClass {
public:
    enum State {
        OTA_STATE_IDLE = 0,
        OTA_STATE_STARTING,
        OTA_STATE_DOWNLOADING,
        OTA_STATE_VERIFYING,
        OTA_STATE_STAGED,
        OTA_STATE_APPLYING,
        OTA_STATE_ERROR
    };

    enum Error {
        OTA_ERROR_NONE = 0,
        OTA_ERROR_INVALID_ARGUMENT = -1,
        OTA_ERROR_INVALID_STATE = -2,
        OTA_ERROR_NETWORK_NOT_READY = -3,
        OTA_ERROR_DOWNLOAD_FAILED = -4,
        OTA_ERROR_VERIFY_FAILED = -5,
        OTA_ERROR_HTTP_STATUS_ERROR = -6,
        OTA_ERROR_INTERNAL = -7
    };

    bool begin(const char *url, const AIR780EPMOTAConfig &config = AIR780EPMOTAConfig());
    State poll(void);
    State state(void);
    bool isRunning(void);
    bool isStaged(void);
    uint32_t totalBytes(void);
    uint32_t downloadedBytes(void);
    int32_t lastError(void);
    bool apply(void);
    bool clear(void);
};

extern AIR780EPMOTAClass AIR780EPMOTA;

#endif
