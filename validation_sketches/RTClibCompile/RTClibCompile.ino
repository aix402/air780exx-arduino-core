#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

static RTC_DS3231 ds3231;
static RTC_PCF8523 pcf8523;

static uint32_t exerciseRtclibApi() {
  DateTime base(2026, 4, 28, 12, 0, 0);
  TimeSpan offset(1, 2, 3, 4);
  DateTime adjusted = base + offset;
  return adjusted.unixtime();
}

void setup() {
  Serial.begin(921600);

  uint32_t timestamp = exerciseRtclibApi();

  if (millis() == 0xFFFFFFFFUL) {
    (void)ds3231.begin(&Wire);
    (void)pcf8523.begin(&Wire);
  }

  Serial.print(F("+ARDUINO: LIB_COMPAT,RTCLIB,READY,TS,"));
  Serial.println(timestamp);
}

void loop() {
  delay(1000);
}
