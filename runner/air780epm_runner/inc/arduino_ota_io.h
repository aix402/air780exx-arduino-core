#ifndef ARDUINO_OTA_IO_H
#define ARDUINO_OTA_IO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ArduinoCoreOtaConfig_Tag {
    const char *user;
    const char *password;
    const char *caCert;
    uint32_t timeoutMs;
    uint8_t insecure;
} ArduinoCoreOtaConfig;

enum {
    ARDUINO_OTA_STATE_IDLE = 0,
    ARDUINO_OTA_STATE_STARTING,
    ARDUINO_OTA_STATE_DOWNLOADING,
    ARDUINO_OTA_STATE_VERIFYING,
    ARDUINO_OTA_STATE_STAGED,
    ARDUINO_OTA_STATE_APPLYING,
    ARDUINO_OTA_STATE_ERROR
};

enum {
    ARDUINO_OTA_ERROR_NONE = 0,
    ARDUINO_OTA_ERROR_INVALID_ARGUMENT = -1,
    ARDUINO_OTA_ERROR_INVALID_STATE = -2,
    ARDUINO_OTA_ERROR_NETWORK_NOT_READY = -3,
    ARDUINO_OTA_ERROR_DOWNLOAD_FAILED = -4,
    ARDUINO_OTA_ERROR_VERIFY_FAILED = -5,
    ARDUINO_OTA_ERROR_HTTP_STATUS_ERROR = -6,
    ARDUINO_OTA_ERROR_INTERNAL = -7
};

int arduinoCoreOtaBegin(const char *url, const ArduinoCoreOtaConfig *config);
int arduinoCoreOtaPoll(void);
int arduinoCoreOtaState(void);
int arduinoCoreOtaIsRunning(void);
int arduinoCoreOtaIsStaged(void);
uint32_t arduinoCoreOtaTotalBytes(void);
uint32_t arduinoCoreOtaDownloadedBytes(void);
int32_t arduinoCoreOtaLastError(void);
bool arduinoCoreOtaApply(void);
bool arduinoCoreOtaClear(void);

#ifdef __cplusplus
}
#endif

#endif
