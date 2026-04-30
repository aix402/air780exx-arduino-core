#include <Arduino.h>
#include <Air780EpmComplexLibProbe.h>

constexpr uint32_t kProbeSeed = 11u;
constexpr uint32_t kExpectedProbeValue = 0x78CBCu;

void setup() {
    Serial.begin(921600);
    pinMode(LED_BUILTIN, OUTPUT);
    const uint32_t value = air780epmComplexProbeValue(kProbeSeed);
    Serial.printf("+ARDUINO: COMPLEX_LIB_PROBE,VALUE,%lu\r\n", static_cast<unsigned long>(value));
    Serial.println(value == kExpectedProbeValue ? "+ARDUINO: COMPLEX_LIB_PROBE,PASS" : "+ARDUINO: COMPLEX_LIB_PROBE,FAIL");
}

void loop() {
    const uint32_t delayMs = 80u + (air780epmComplexProbeValue(kProbeSeed) % 240u);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
}
