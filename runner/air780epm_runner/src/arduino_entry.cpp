#include "Arduino.h"

void setup(void) {
    Serial.begin(921600);
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("+ARDUINO: AIR780EPM,READY");
}

void loop(void) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("+ARDUINO: BLINK,HIGH");
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("+ARDUINO: BLINK,LOW");
    delay(500);
}
