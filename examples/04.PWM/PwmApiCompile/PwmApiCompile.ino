#include <Arduino.h>

static const uint8_t pwmPins[] = {
  PIN_PWM0,
  PIN_PWM1,
  PIN_PWM2,
  PIN_PWM4,
};

void setup() {
  Serial.begin(115200);
  Serial.println(F("PWM API compile smoke"));

  analogWriteResolution(8);
  analogWriteFrequency(PIN_PWM4, 1000);
  analogWrite(PIN_PWM4, 128);
  delay(10);
  analogWrite(PIN_PWM4, 0);

  analogWriteFrequency(2000);
  for (size_t i = 0; i < sizeof(pwmPins) / sizeof(pwmPins[0]); ++i) {
    analogWrite(pwmPins[i], 64 + static_cast<int>(i) * 32);
  }

  analogWriteFreq(1000);
  analogWriteResolution(10);
  analogWrite(PIN_PWM4, 512);
}

void loop() {
  static int value = 0;
  static int step = 64;

  analogWrite(PIN_PWM4, value);
  value += step;
  if (value >= 1023) {
    value = 1023;
    step = -64;
  } else if (value <= 0) {
    value = 0;
    step = 64;
  }
  delay(20);
}
