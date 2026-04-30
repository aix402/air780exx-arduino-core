#include <ArduinoJson.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    JsonDocument doc;
    doc["base"] = 90;
    doc["step"] = 37;
    doc["enabled"] = true;

    const uint32_t delayMs = doc["base"].as<uint32_t>() + doc["step"].as<uint32_t>();
    digitalWrite(LED_BUILTIN, doc["enabled"].as<bool>() ? HIGH : LOW);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
}
