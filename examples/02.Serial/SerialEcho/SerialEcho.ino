#include <Arduino.h>

void setup() {
  Serial.begin(921600);
  Serial.println("+ARDUINO: SERIAL,READY");
}
void loop() {
  Serial.println("+ARDUINO: SERIAL,TICK");
  delay(1000);
}
