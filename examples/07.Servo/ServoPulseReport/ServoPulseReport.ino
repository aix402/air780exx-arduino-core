#include <Arduino.h>
#include <Servo.h>

namespace {

constexpr pin_size_t kServoPin = PIN_PWM4;
constexpr unsigned long kBootDelayMs = 1200UL;
constexpr unsigned long kStepDelayMs = 800UL;
constexpr int kPulseWidths[] = {1000, 1500, 2000, 1500};

Servo ServoOut;

void writePulse(int pulseUs) {
  ServoOut.writeMicroseconds(pulseUs);
  Serial.print("+ARDUINO: SERVO,PULSE_US,");
  Serial.print((unsigned long)ServoOut.readMicroseconds());
  Serial.print(",ANGLE,");
  Serial.println((unsigned long)ServoOut.read());
}

void runPulseReport() {
  for (size_t index = 0U; index < (sizeof(kPulseWidths) / sizeof(kPulseWidths[0])); ++index) {
    writePulse(kPulseWidths[index]);
    delay(kStepDelayMs);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(kBootDelayMs);

  uint8_t channel = ServoOut.attach((int)kServoPin);
  if (channel == INVALID_SERVO) {
    Serial.println("+ARDUINO: SERVO,ATTACH,FAIL");
    return;
  }

  Serial.print("+ARDUINO: SERVO,READY,PIN,");
  Serial.print((unsigned long)kServoPin);
  Serial.print(",CHANNEL,");
  Serial.println((unsigned long)channel);

  runPulseReport();
}

void loop() {
  runPulseReport();
  delay(2000UL);
}
