#include <Arduino.h>

namespace {

constexpr uint8_t kProbePin = PIN_PWM4;
constexpr int kLevels[] = {0, 32, 96, 160, 224, 255, 160, 96, 32, 0};
constexpr unsigned long kStepDelayMs = 900;

void applyLevel(int value) {
  analogWrite(kProbePin, value);
  Serial.printf("+ARDUINO: PWM4,LEVEL,%d\r\n", value);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.printf("+ARDUINO: PWM4,READY\r\n");
  Serial.printf("+ARDUINO: PWM4,PIN,33\r\n");

  analogWriteResolution(8);
  analogWriteFrequency(kProbePin, 1000);

  applyLevel(0);
  delay(1000);
  applyLevel(255);
  delay(1000);
  applyLevel(0);
  delay(1000);
}

void loop() {
  for (size_t i = 0; i < sizeof(kLevels) / sizeof(kLevels[0]); ++i) {
    applyLevel(kLevels[i]);
    delay(kStepDelayMs);
  }
}
