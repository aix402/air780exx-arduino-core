#include "Arduino.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

extern "C" {
#include "arduino_time_io.h"
}

namespace {

constexpr time_t kMinValidEpoch = 1700000000;
constexpr uint32_t kMinNtpAttemptMs = 1000UL;
constexpr uint32_t kMaxPerServerNtpMs = 5000UL;
constexpr uint16_t kNtpPort = 123U;
constexpr uint16_t kNtpLocalPort = 2390U;
constexpr size_t kNtpPacketSize = 48U;
constexpr uint32_t kNtpEpochOffset = 2208988800UL;
constexpr uint8_t kServerCount = 3U;
constexpr size_t kServerLength = 64U;

const char *const kDefaultServers[kServerCount] = {
    "ntp.aliyun.com",
    "ntp.ntsc.ac.cn",
    "time1.cloud.tencent.com",
};

struct ArduinoTimeConfig {
    int32_t localOffsetSeconds;
    bool customOffsetActive;
    char servers[kServerCount][kServerLength];
};

ArduinoTimeConfig gTimeConfig = {};
bool gTimeConfigInitialized = false;

void arduinoTimeInitDefaults()
{
    if (gTimeConfigInitialized)
    {
        return;
    }

    memset(&gTimeConfig, 0, sizeof(gTimeConfig));
    for (uint8_t i = 0; i < kServerCount; ++i)
    {
        strncpy(gTimeConfig.servers[i], kDefaultServers[i], kServerLength - 1U);
        gTimeConfig.servers[i][kServerLength - 1U] = '\0';
    }

    gTimeConfigInitialized = true;
}

bool arduinoTimeEpochLooksValid(time_t epoch)
{
    return epoch >= kMinValidEpoch;
}

uint32_t arduinoTimeElapsed(unsigned long start)
{
    return (uint32_t)(millis() - start);
}

uint32_t arduinoTimeRemaining(unsigned long start, uint32_t budgetMs)
{
    const uint32_t elapsed = arduinoTimeElapsed(start);
    return (elapsed >= budgetMs) ? 0UL : (budgetMs - elapsed);
}

void arduinoTimePreparePacket(uint8_t *packet)
{
    memset(packet, 0, kNtpPacketSize);
    packet[0] = 0b11100011;
    packet[1] = 0;
    packet[2] = 6;
    packet[3] = 0xEC;
    packet[12] = 49;
    packet[13] = 0x4E;
    packet[14] = 49;
    packet[15] = 52;
}

time_t arduinoTimeParseEpoch(const uint8_t *packet)
{
    const uint32_t highWord = ((uint32_t)packet[40] << 8) | packet[41];
    const uint32_t lowWord = ((uint32_t)packet[42] << 8) | packet[43];
    const uint32_t secondsSince1900 = (highWord << 16) | lowWord;

    if (secondsSince1900 < kNtpEpochOffset)
    {
        return 0;
    }

    return (time_t)(secondsSince1900 - kNtpEpochOffset);
}

void arduinoTimeSetServers(const char *server1, const char *server2, const char *server3)
{
    const char *const requested[kServerCount] = {server1, server2, server3};
    bool anyRequested = false;

    arduinoTimeInitDefaults();

    for (uint8_t i = 0; i < kServerCount; ++i)
    {
        if ((requested[i] != NULL) && (requested[i][0] != '\0'))
        {
            anyRequested = true;
            break;
        }
    }

    for (uint8_t i = 0; i < kServerCount; ++i)
    {
        const char *source = anyRequested ? requested[i] : kDefaultServers[i];
        if (source == NULL)
        {
            gTimeConfig.servers[i][0] = '\0';
            continue;
        }

        strncpy(gTimeConfig.servers[i], source, kServerLength - 1U);
        gTimeConfig.servers[i][kServerLength - 1U] = '\0';
    }
}

bool arduinoTimeHasServer(void)
{
    for (uint8_t i = 0; i < kServerCount; ++i)
    {
        if (gTimeConfig.servers[i][0] != '\0')
        {
            return true;
        }
    }

    return false;
}

int8_t arduinoTimeRoundQuarterHours(int32_t offsetSeconds)
{
    int32_t rounded = offsetSeconds;

    if (rounded >= 0)
    {
        rounded += 450;
    }
    else
    {
        rounded -= 450;
    }

    rounded /= 900;
    if (rounded > 56)
    {
        rounded = 56;
    }
    else if (rounded < -48)
    {
        rounded = -48;
    }

    return (int8_t)rounded;
}

void arduinoTimeApplyOffset(int32_t offsetSeconds)
{
    gTimeConfig.localOffsetSeconds = offsetSeconds;
    gTimeConfig.customOffsetActive = true;
    (void)arduinoCoreTimeSetTimezoneQuarterHours(arduinoTimeRoundQuarterHours(offsetSeconds));
}

bool arduinoTimeParseNumber(const char **cursor, int *outValue)
{
    const char *p = *cursor;
    int value = 0;

    if ((p == NULL) || !isdigit((unsigned char)*p))
    {
        return false;
    }

    while (isdigit((unsigned char)*p))
    {
        if (value > ((INT_MAX - 9) / 10))
        {
            return false;
        }
        value = (value * 10) + (*p - '0');
        ++p;
    }

    *cursor = p;
    *outValue = value;
    return true;
}

bool arduinoTimeParseFixedOffsetTz(const char *tz, int32_t *outLocalOffsetSeconds)
{
    const char *p = tz;
    int sign = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int32_t posixOffset;

    if ((tz == NULL) || (outLocalOffsetSeconds == NULL))
    {
        return false;
    }

    while (isalpha((unsigned char)*p))
    {
        ++p;
    }

    if (*p == '<')
    {
        ++p;
        while ((*p != '\0') && (*p != '>'))
        {
            ++p;
        }
        if (*p == '>')
        {
            ++p;
        }
    }

    if (*p == '+')
    {
        sign = 1;
        ++p;
    }
    else if (*p == '-')
    {
        sign = -1;
        ++p;
    }

    if (!arduinoTimeParseNumber(&p, &hours))
    {
        return false;
    }

    if (*p == ':')
    {
        ++p;
        if (!arduinoTimeParseNumber(&p, &minutes))
        {
            return false;
        }

        if (*p == ':')
        {
            ++p;
            if (!arduinoTimeParseNumber(&p, &seconds))
            {
                return false;
            }
        }
    }

    posixOffset = sign * (hours * 3600L + minutes * 60L + seconds);
    *outLocalOffsetSeconds = -posixOffset;
    return true;
}

bool arduinoTimeFillTm(struct tm *info, time_t epoch)
{
    struct tm *result;
    time_t adjustedEpoch = epoch;

    if (info == NULL)
    {
        return false;
    }

    if (gTimeConfig.customOffsetActive)
    {
        adjustedEpoch += gTimeConfig.localOffsetSeconds;
        result = gmtime(&adjustedEpoch);
    }
    else
    {
        result = localtime(&adjustedEpoch);
    }

    if (result == NULL)
    {
        return false;
    }

    memcpy(info, result, sizeof(*info));
    return true;
}

bool arduinoTimeReadNow(time_t *outEpoch)
{
    time_t now = 0;

    time(&now);
    if (!arduinoTimeEpochLooksValid(now))
    {
        return false;
    }

    if (outEpoch != NULL)
    {
        *outEpoch = now;
    }

    return true;
}

bool arduinoTimeTryNitz(uint32_t timeoutMs)
{
    if (timeoutMs == 0UL)
    {
        return arduinoTimeReadNow(NULL);
    }

    if (arduinoTimeReadNow(NULL))
    {
        return true;
    }

    return Modem.waitForTimeSync(timeoutMs);
}

bool arduinoTimeSyncSingleServer(const char *server, uint32_t timeoutMs)
{
    CellularUDP udp;
    uint8_t packet[kNtpPacketSize];
    int packetSize = 0;
    const unsigned long start = millis();

    if ((server == NULL) || (server[0] == '\0'))
    {
        return false;
    }

    if (!udp.begin(kNtpLocalPort))
    {
        return false;
    }

    arduinoTimePreparePacket(packet);
    if (!udp.beginPacket(server, kNtpPort))
    {
        udp.stop();
        return false;
    }

    if (udp.write(packet, sizeof(packet)) != sizeof(packet))
    {
        udp.stop();
        return false;
    }

    if (!udp.endPacket())
    {
        udp.stop();
        return false;
    }

    while (arduinoTimeElapsed(start) < timeoutMs)
    {
        packetSize = udp.parsePacket();
        if (packetSize >= (int)kNtpPacketSize)
        {
            break;
        }
        delay(50UL);
    }

    if (packetSize < (int)kNtpPacketSize)
    {
        udp.stop();
        return false;
    }

    if (udp.read(packet, sizeof(packet)) != (int)sizeof(packet))
    {
        udp.stop();
        return false;
    }

    udp.stop();

    const time_t epoch = arduinoTimeParseEpoch(packet);
    if (!arduinoTimeEpochLooksValid(epoch))
    {
        return false;
    }

    return arduinoCoreTimeSetEpoch((uint32_t)epoch) == 0;
}

bool arduinoTimeTryNtp(uint32_t timeoutMs)
{
    const unsigned long start = millis();

    if (timeoutMs < kMinNtpAttemptMs)
    {
        return false;
    }

    if (!Modem.waitForNetwork(timeoutMs))
    {
        return false;
    }

    if (!Modem.activatePDP(arduinoTimeRemaining(start, timeoutMs)))
    {
        return false;
    }

    for (uint8_t i = 0; i < kServerCount; ++i)
    {
        const uint32_t remaining = arduinoTimeRemaining(start, timeoutMs);
        uint32_t perServerBudget;

        if (remaining < kMinNtpAttemptMs)
        {
            break;
        }

        if (gTimeConfig.servers[i][0] == '\0')
        {
            continue;
        }

        perServerBudget = remaining;
        if (perServerBudget > kMaxPerServerNtpMs)
        {
            perServerBudget = kMaxPerServerNtpMs;
        }

        if (arduinoTimeSyncSingleServer(gTimeConfig.servers[i], perServerBudget))
        {
            return true;
        }
    }

    return false;
}

}  // namespace

extern "C" void configTime(long gmtOffset_sec,
                           int daylightOffset_sec,
                           const char *server1,
                           const char *server2,
                           const char *server3)
{
    arduinoTimeInitDefaults();
    arduinoTimeSetServers(server1, server2, server3);
    arduinoTimeApplyOffset((int32_t)gmtOffset_sec + (int32_t)daylightOffset_sec);
}

extern "C" void configTzTime(const char *tz,
                             const char *server1,
                             const char *server2,
                             const char *server3)
{
    int32_t localOffsetSeconds = 0;

    arduinoTimeInitDefaults();
    arduinoTimeSetServers(server1, server2, server3);

    if (arduinoTimeParseFixedOffsetTz(tz, &localOffsetSeconds))
    {
        arduinoTimeApplyOffset(localOffsetSeconds);
    }
    else
    {
        gTimeConfig.customOffsetActive = false;
    }
}

extern "C" bool getLocalTime(struct tm *info, uint32_t ms)
{
    const unsigned long start = millis();
    time_t epoch = 0;
    const uint32_t nitzBudget = (ms > 3000UL) ? 3000UL : ms;

    arduinoTimeInitDefaults();

    if (arduinoTimeReadNow(&epoch))
    {
        return arduinoTimeFillTm(info, epoch);
    }

    if (arduinoTimeTryNitz(nitzBudget) && arduinoTimeReadNow(&epoch))
    {
        return arduinoTimeFillTm(info, epoch);
    }

    if (!arduinoTimeHasServer())
    {
        return false;
    }

    if (arduinoTimeTryNtp(arduinoTimeRemaining(start, ms)) && arduinoTimeReadNow(&epoch))
    {
        return arduinoTimeFillTm(info, epoch);
    }

    return false;
}
