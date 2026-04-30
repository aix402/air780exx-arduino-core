#include <Air780EpmLinkProbe.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    const uint32_t delayMs = 100u + (air780epmLinkProbeValue(23u) % 200u);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
}
