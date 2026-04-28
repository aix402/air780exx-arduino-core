#include "arduino_time_io.h"

#include "luat_rtc.h"
#include "mw_aon_info.h"

int arduinoCoreTimeSetEpoch(uint32_t epochSeconds)
{
    luat_rtc_set_tamp32(epochSeconds);
    mwAonSetUtcTimeSyncFlag(1);
    mwAonSetNitzUtcTimeSyncFlag(0);
    return 0;
}

int arduinoCoreTimeSetTimezoneQuarterHours(int8_t timezoneQuarterHours)
{
    int timezone = timezoneQuarterHours;
    return luat_rtc_timezone(&timezone);
}

int arduinoCoreTimeGetTimezoneQuarterHours(int8_t *outTimezoneQuarterHours)
{
    const int timezone = luat_rtc_timezone(NULL);

    if (outTimezoneQuarterHours == NULL)
    {
        return timezone;
    }

    *outTimezoneQuarterHours = (int8_t)timezone;
    return timezone;
}
