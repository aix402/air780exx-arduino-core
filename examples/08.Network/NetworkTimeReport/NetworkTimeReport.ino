#include <Arduino.h>

namespace {

constexpr long kGmtOffsetSeconds = 8L * 3600L;
constexpr int kDaylightOffsetSeconds = 0;
const char *const kServer1 = "ntp.aliyun.com";
const char *const kServer2 = "ntp.ntsc.ac.cn";
const char *const kServer3 = "time1.cloud.tencent.com";

void printStatus(const char *stage)
{
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);

  Serial.print("+ARDUINO: NET_TIME,STATUS,");
  Serial.print(stage);
  Serial.print(",GET,");
  Serial.print(ok ? 1 : 0);
  Serial.print(",REGISTERED,");
  Serial.print(status.registered ? 1 : 0);
  Serial.print(",NET_READY,");
  Serial.print(status.networkReady ? 1 : 0);
  Serial.print(",HAS_IPV4,");
  Serial.print(status.hasIPv4 ? 1 : 0);
  Serial.print(",IP,");
  Serial.println(status.localIPv4);
}

void printTimeStatus(const char *stage)
{
  AIR780EPMTimeStatus status;
  const bool ok = Modem.getTimeStatus(status);

  Serial.print("+ARDUINO: NET_TIME,TIME_STATUS,");
  Serial.print(stage);
  Serial.print(",GET,");
  Serial.print(ok ? 1 : 0);
  Serial.print(",VALID,");
  Serial.print(status.valid ? 1 : 0);
  Serial.print(",SYNCED,");
  Serial.print(status.synced ? 1 : 0);
  Serial.print(",NITZ,");
  Serial.print(status.nitzSynced ? 1 : 0);
  Serial.print(",TZQH,");
  Serial.print((int)status.timezoneQuarterHours);
  Serial.print(",EPOCH,");
  Serial.println(status.epoch);
}

void fail(const char *stage, int32_t error)
{
  Serial.print("+ARDUINO: NET_TIME,FAIL,");
  Serial.print(stage);
  Serial.print(",ERR,");
  Serial.println(error);
}

void printLocalTime(const struct tm &timeinfo)
{
  char buffer[32] = {0};
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  Serial.print("+ARDUINO: NET_TIME,LOCAL,");
  Serial.println(buffer);
}

}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println("+ARDUINO: NET_TIME,READY");
  Modem.begin();

  printStatus("EARLY");
  printTimeStatus("EARLY");

  if (!Modem.waitForNetwork(60000UL))
  {
    printStatus("REGISTER_TIMEOUT");
    fail("REGISTER_TIMEOUT", 0);
    return;
  }

  printStatus("REGISTERED");

  if (!Modem.activatePDP(60000UL))
  {
    printStatus("PDP_TIMEOUT");
    fail("PDP_TIMEOUT", 0);
    return;
  }

  printStatus("NET_READY");
  printTimeStatus("NET_READY");

  configTime(kGmtOffsetSeconds, kDaylightOffsetSeconds, kServer1, kServer2, kServer3);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 15000UL))
  {
    printTimeStatus("GET_LOCAL_TIME_FAIL");
    fail("GET_LOCAL_TIME", 0);
    return;
  }

  time_t now = 0;
  time(&now);

  Serial.print("+ARDUINO: NET_TIME,EPOCH,");
  Serial.println((uint32_t)now);
  printLocalTime(timeinfo);
  printTimeStatus("AFTER_SYNC");

  if ((uint32_t)now < 1700000000UL)
  {
    fail("EPOCH_RANGE", 0);
    return;
  }

  Serial.println("+ARDUINO: NET_TIME,PASS");
}

void loop()
{
  delay(1000);
}
