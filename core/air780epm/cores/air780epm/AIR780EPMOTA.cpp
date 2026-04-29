#include "AIR780EPMOTA.h"

#include <string.h>

#include "arduino_ota_io.h"

AIR780EPMOTAClass AIR780EPMOTA;

bool AIR780EPMOTAClass::begin(const char *url, const AIR780EPMOTAConfig &config)
{
    ArduinoCoreOtaConfig nativeConfig;

    memset(&nativeConfig, 0, sizeof(nativeConfig));
    nativeConfig.user = config.user;
    nativeConfig.password = config.password;
    nativeConfig.caCert = config.caCert;
    nativeConfig.timeoutMs = config.timeoutMs;
    nativeConfig.insecure = config.insecure ? 1U : 0U;

    return arduinoCoreOtaBegin(url, &nativeConfig) != 0;
}

AIR780EPMOTAClass::State AIR780EPMOTAClass::poll(void)
{
    return static_cast<State>(arduinoCoreOtaPoll());
}

AIR780EPMOTAClass::State AIR780EPMOTAClass::state(void)
{
    return static_cast<State>(arduinoCoreOtaState());
}

bool AIR780EPMOTAClass::isRunning(void)
{
    return arduinoCoreOtaIsRunning() != 0;
}

bool AIR780EPMOTAClass::isStaged(void)
{
    return arduinoCoreOtaIsStaged() != 0;
}

uint32_t AIR780EPMOTAClass::totalBytes(void)
{
    return arduinoCoreOtaTotalBytes();
}

uint32_t AIR780EPMOTAClass::downloadedBytes(void)
{
    return arduinoCoreOtaDownloadedBytes();
}

int32_t AIR780EPMOTAClass::lastError(void)
{
    return arduinoCoreOtaLastError();
}

bool AIR780EPMOTAClass::apply(void)
{
    return arduinoCoreOtaApply();
}

bool AIR780EPMOTAClass::clear(void)
{
    return arduinoCoreOtaClear();
}
