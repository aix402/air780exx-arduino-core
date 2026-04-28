#include <Arduino.h>

namespace {

struct PinCapabilityRow {
  uint8_t pin;
  const char* tag;
  const char* note;
};

constexpr PinCapabilityRow kPins[] = {
    {PIN_GPIO0, "SENSITIVE", "USB_BOOT"},
    {PIN_GPIO14, "BUS", "WIRE_SCL_OR_UART3_RX"},
    {PIN_GPIO15, "BUS", "WIRE_SDA_OR_UART3_TX"},
    {PIN_GPIO18, "SENSITIVE", "GPIO_OR_UART1_RX_OR_I2C1_SCL_ALT"},
    {PIN_GPIO19, "SENSITIVE", "GPIO_OR_UART1_TX_OR_I2C1_SDA_ALT"},
    {PIN_GPIO27, "VERIFIED", "LED_BUILTIN"},
    {PIN_GPIO28, "BOARD", "LCD_POWER_V1_2"},
    {PIN_GPIO33, "VERIFIED", "PWM4_LED_PROBE"},
    {PIN_GPIO36, "BOARD", "LCD_RESET_V1_2"},
};

void printPinCaps(const PinCapabilityRow& row) {
  Serial.printf(
      "+ARDUINO: PIN_CAPS,PIN=%u,PWM=%u,TAG=%s,NOTE=%s\r\n",
      row.pin,
      digitalPinHasPWM(row.pin) ? 1U : 0U,
      row.tag,
      row.note);
}

void printPwmSummary() {
  const uint8_t pwmPins[] = {PIN_PWM0, PIN_PWM1, PIN_PWM2, PIN_PWM4};
  for (size_t i = 0; i < (sizeof(pwmPins) / sizeof(pwmPins[0])); ++i) {
    const uint8_t pin = pwmPins[i];
    Serial.printf(
        "+ARDUINO: PIN_CAPS,PWM_PIN=%u,TIMER=%d\r\n",
        pin,
        static_cast<int>(digitalPinToTimer(pin)));
  }
}

void printAnalogSummary() {
#if NUM_ANALOG_INPUTS > 0
  Serial.printf("+ARDUINO: PIN_CAPS,ANALOG_ALIAS,A0=%u\r\n", A0);
  Serial.printf("+ARDUINO: PIN_CAPS,ANALOG_ALIAS,A1=%u\r\n", A1);
  Serial.printf("+ARDUINO: PIN_CAPS,ANALOG_ALIAS,A2=%u\r\n", A2);
  Serial.printf("+ARDUINO: PIN_CAPS,ANALOG_ALIAS,A3=%u\r\n", A3);
#else
  Serial.printf("+ARDUINO: PIN_CAPS,ANALOG_ALIAS,NONE\r\n");
#endif
}

void printOnce() {
  Serial.printf("+ARDUINO: PIN_CAPS,READY\r\n");
  for (size_t i = 0; i < (sizeof(kPins) / sizeof(kPins[0])); ++i) {
    printPinCaps(kPins[i]);
  }
  printPwmSummary();
  printAnalogSummary();
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
