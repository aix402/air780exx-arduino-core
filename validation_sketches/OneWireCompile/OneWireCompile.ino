#include <Arduino.h>
#include <OneWire.h>

static OneWire oneWire(PIN_GPIO14);

void setup() {
  Serial.begin(921600);

  uint8_t address[8] = {0};
  oneWire.reset_search();
  bool found = oneWire.search(address);

  Serial.print(F("+ARDUINO: LIB_COMPAT,ONEWIRE,READY,FOUND,"));
  Serial.println(found ? 1 : 0);
}

void loop() {
  delay(1000);
}
