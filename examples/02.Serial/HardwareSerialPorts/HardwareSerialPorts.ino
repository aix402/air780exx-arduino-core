#include <Arduino.h>

void setup() {
  Serial.begin(921600);
  Serial.println("+ARDUINO: UART,COMPILE");

  Serial1.begin(115200, SERIAL_8N1);
  Serial2.begin(115200, SERIAL_8N1);
  Serial3.begin(115200, SERIAL_8N1);

  Serial1.println("Serial1 ready");
  Serial2.println("Serial2 ready");
  Serial3.println("Serial3 ready");
}

void loop() {
  if (Serial1.available()) {
    Serial1.write((uint8_t)Serial1.read());
  }
  if (Serial2.available()) {
    Serial2.write((uint8_t)Serial2.read());
  }
  if (Serial3.available()) {
    Serial3.write((uint8_t)Serial3.read());
  }
  delay(10);
}
