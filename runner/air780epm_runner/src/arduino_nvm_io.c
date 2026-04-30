#include "arduino_nvm_io.h"

#include <stdlib.h>
#include <string.h>

#include "osanvm.h"

int arduinoCoreNvmRead(const char *name, uint8_t *version, uint8_t **data, size_t *size)
{
    OsaNvmBodyInfo bodyInfo = {0};
    UINT8 sdkVersion = 0U;
    int rc = 0;

    if (data == NULL || size == NULL)
    {
        return -1;
    }

    *data = NULL;
    *size = 0U;

    rc = OsaNvmRead(name, &sdkVersion, &bodyInfo, 0U);
    if (rc != OSA_NVM_SUCC)
    {
        return rc;
    }

    if (version != NULL)
    {
        *version = (uint8_t)sdkVersion;
    }

    if (bodyInfo.bodySize > 0U)
    {
        *data = (uint8_t *)malloc(bodyInfo.bodySize);
        if (*data == NULL)
        {
            OsaNvmFreeBody(&bodyInfo);
            return -1;
        }
        memcpy(*data, bodyInfo.pBuf, bodyInfo.bodySize);
        *size = bodyInfo.bodySize;
    }

    OsaNvmFreeBody(&bodyInfo);
    return 0;
}

int arduinoCoreNvmWrite(const char *name, uint8_t version, const void *data, size_t size)
{
    if (size > UINT32_MAX)
    {
        return -1;
    }

    return OsaNvmWrite(name, (UINT8)version, (void *)data, (UINT32)size);
}
