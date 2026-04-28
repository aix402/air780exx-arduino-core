#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

namespace {

const char *kNtpHost = "pool.ntp.org";
const long kTimeOffsetSeconds = 8L * 3600L;
const unsigned long kUpdateIntervalMs = 60000UL;
const uint16_t kLocalPort = 2390U;

WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, kNtpHost, kTimeOffsetSeconds, kUpdateIntervalMs);

void printStatus(const char *stage)
{
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);

  Serial.print("+ARDUINO: NTPCLIENT,STATUS,");
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

void fail(const char *stage)
{
  Serial.print("+ARDUINO: NTPCLIENT,FAIL,");
  Serial.println(stage);
  timeClient.end();
}

bool updateTimeWithRetries(uint8_t attempts)
{
  for (uint8_t i = 0; i < attempts; ++i)
  {
    Serial.print("+ARDUINO: NTPCLIENT,UPDATE_ATTEMPT,");
    Serial.println((unsigned int)(i + 1U));

    if (timeClient.forceUpdate())
    {
      return true;
    }

    delay(1000);
  }

  return false;
}

} // namespace

void setup()
{
  Serial.begin(921600);
  delay(1500);

  Serial.println("+ARDUINO: NTPCLIENT,READY");
  Modem.begin();

  printStatus("EARLY");
  if (!Modem.waitForNetwork(60000UL))
  {
    printStatus("REGISTER_TIMEOUT");
    fail("REGISTER_TIMEOUT");
    return;
  }

  printStatus("REGISTERED");
  if (!Modem.activatePDP(60000UL))
  {
    printStatus("PDP_TIMEOUT");
    fail("PDP_TIMEOUT");
    return;
  }

  printStatus("NET_READY");

  timeClient.begin(kLocalPort);
  if (!updateTimeWithRetries(10U))
  {
    fail("UPDATE_TIMEOUT");
    return;
  }

  const unsigned long epoch = timeClient.getEpochTime();
  const String formatted = timeClient.getFormattedTime();

  Serial.print("+ARDUINO: NTPCLIENT,EPOCH,");
  Serial.println(epoch);
  Serial.print("+ARDUINO: NTPCLIENT,FORMATTED,");
  Serial.println(formatted);
  Serial.print("+ARDUINO: NTPCLIENT,TIME_SET,");
  Serial.println(timeClient.isTimeSet() ? 1 : 0);

  if (epoch < 1700000000UL)
  {
    fail("EPOCH_RANGE");
    return;
  }

  if (!timeClient.isTimeSet())
  {
    fail("TIME_NOT_SET");
    return;
  }

  Serial.println("+ARDUINO: NTPCLIENT,PASS");
  timeClient.end();
}

void loop()
{
  delay(1000);
}
