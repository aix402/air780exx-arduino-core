#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire oneWire(PIN_GPIO14);
static DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(921600);

  sensors.begin();
  int deviceCount = sensors.getDeviceCount();

  Serial.print(F("+ARDUINO: LIB_COMPAT,DALLAS_TEMPERATURE,READY,COUNT,"));
  Serial.println(deviceCount);
}

void loop() {
  delay(1000);
}
