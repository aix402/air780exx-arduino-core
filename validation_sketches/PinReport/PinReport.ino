#include <Arduino.h>

namespace {

void printAlias(const char* name, unsigned int value) {
  Serial.printf("+ARDUINO: PIN_REPORT,%s,%u\r\n", name, value);
}

void printOnce() {
  Serial.printf("+ARDUINO: PIN_REPORT,READY\r\n");
  Serial.printf("+ARDUINO: PIN_REPORT,NUM_DIGITAL,%u\r\n", NUM_DIGITAL_PINS);
  Serial.printf("+ARDUINO: PIN_REPORT,NUM_ANALOG,%u\r\n", NUM_ANALOG_INPUTS);

  printAlias("LED_BUILTIN", LED_BUILTIN);
  printAlias("SCL", SCL);
  printAlias("SDA", SDA);
  printAlias("WIRE1_SCL", PIN_WIRE1_SCL);
  printAlias("WIRE1_SDA", PIN_WIRE1_SDA);
  printAlias("SS", SS);
  printAlias("MOSI", MOSI);
  printAlias("MISO", MISO);
  printAlias("SCK", SCK);
  printAlias("PIN_PWM0", PIN_PWM0);
  printAlias("PIN_PWM1", PIN_PWM1);
  printAlias("PIN_PWM2", PIN_PWM2);
  printAlias("PIN_PWM4", PIN_PWM4);
  printAlias("PIN_UART1_RX", PIN_UART1_RX);
  printAlias("PIN_UART1_TX", PIN_UART1_TX);
  printAlias("PIN_UART2_RX", PIN_UART2_RX);
  printAlias("PIN_UART2_TX", PIN_UART2_TX);
  printAlias("PIN_UART3_RX", PIN_UART3_RX);
  printAlias("PIN_UART3_TX", PIN_UART3_TX);
}

}  // namespace

void setup() {
  Serial.begin(921600);
  delay(300);
  printOnce();
}

void loop() {
  delay(5000);
  printOnce();
}
