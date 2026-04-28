#include <Arduino.h>

namespace {

struct AnalogAlias {
  const char* name;
  uint8_t pin;
};

constexpr AnalogAlias kAnalogPins[] = {
    {"A0", A0},
    {"A1", A1},
    {"A2", A2},
    {"A3", A3},
};

void printChannel(const AnalogAlias& alias) {
  const int raw = analogRead(alias.pin);
  const uint32_t millivolts = analogReadMilliVolts(alias.pin);
  Serial.printf(
      "+ARDUINO: ADC,%s,RAW=%d,MV=%lu\r\n",
      alias.name,
      raw,
      static_cast<unsigned long>(millivolts));
}

void printOnce() {
  Serial.printf("+ARDUINO: ADC,READY\r\n");
  Serial.printf("+ARDUINO: ADC,NUM_ANALOG,%u\r\n", NUM_ANALOG_INPUTS);
  for (size_t i = 0; i < (sizeof(kAnalogPins) / sizeof(kAnalogPins[0])); ++i) {
    printChannel(kAnalogPins[i]);
  }
}

}  // namespace

void setup() {
  Serial.begin(921600);
  delay(300);
  analogReadResolution(12);
  printOnce();
}

void loop() {
  delay(1000);
  printOnce();
}
