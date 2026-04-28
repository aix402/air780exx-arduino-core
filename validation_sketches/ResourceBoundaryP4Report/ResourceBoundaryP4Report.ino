#include <Arduino.h>

namespace {

void printLine(const char* bucket, const char* item, const char* note) {
  Serial.printf("+ARDUINO: RESOURCE,%s,%s,%s\r\n", bucket, item, note);
}

void printOnce() {
  Serial.printf("+ARDUINO: RESOURCE,READY\r\n");

  printLine("VERIFIED", "SERIAL", "USB_LOG_CHANNEL");
  printLine("VERIFIED", "LED_BUILTIN", "GPIO27_PIN16");
  printLine("VERIFIED", "WIRE", "I2C0_GPIO14_GPIO15");
  printLine("VERIFIED", "SERIAL1", "UART1_GPIO18_GPIO19");
  printLine("VERIFIED", "PWM4", "GPIO33_PIN26");
  printLine("VERIFIED", "LCD_HWIF", "DEV_BOARD_V1_2_ONLY");

  printLine("AVAILABLE", "SERIAL2", "COMPILE_ENABLED");
  printLine("AVAILABLE", "SERIAL3", "COMPILE_ENABLED");
  printLine("AVAILABLE", "WIRE1", "COMPILE_ENABLED");
  printLine("AVAILABLE", "SPI", "COMPILE_ENABLED");
  printLine("AVAILABLE", "SPI1", "COMPILE_ENABLED");
  printLine("AVAILABLE", "PWM0", "COMPILE_ENABLED");
  printLine("AVAILABLE", "PWM1", "COMPILE_ENABLED");
  printLine("AVAILABLE", "PWM2", "COMPILE_ENABLED");
#if NUM_ANALOG_INPUTS > 0
  printLine("AVAILABLE", "ADC_A0_A3", "LOGICAL_CHANNELS");
#else
  printLine("AVAILABLE", "ADC", "PENDING");
#endif

  printLine("SENSITIVE", "GPIO0", "USB_BOOT");
  printLine("SENSITIVE", "GPIO18_19", "UART1_AND_I2C1_ALT_ROUTE_PRESSURE");
  printLine("SENSITIVE", "GPIO14_15", "WIRE_AND_UART3_ROUTE_PRESSURE");
  printLine("SENSITIVE", "GPIO28_36", "BOARD_SPECIFIC_LCD_CONTROL");
  printLine("SENSITIVE", "PIN62_63_64", "NOT_FIRST_BATCH_DIGITAL_PINS");
  printLine("SENSITIVE", "WAKEUP_POWER_PINS", "NOT_GENERIC_GPIO");
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
