#include "arduino_debug_io.h"

#include "common_api.h"

size_t arduinoCoreDebugWrite(const uint8_t *buffer, size_t size)
{
    if (buffer == NULL || size == 0U)
    {
        return 0U;
    }

    soc_debug_out((const char *)buffer, (uint32_t)size);
    return size;
}
