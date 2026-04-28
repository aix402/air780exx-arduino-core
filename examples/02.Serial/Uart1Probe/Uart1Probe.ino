#include <Arduino.h>

namespace {

constexpr unsigned long kTickIntervalMs = 1000;
constexpr size_t kLineBufferSize = 96;

char gLineBuffer[kLineBufferSize];
size_t gLineLength = 0;
unsigned long gLastTickMs = 0;
unsigned long gTickCount = 0;

void emitTick() {
  Serial1.printf("+ARDUINO: UART1,TICK,%lu\r\n", gTickCount++);
}

void flushLine() {
  if (gLineLength == 0) {
    return;
  }

  gLineBuffer[gLineLength] = '\0';
  Serial.printf("+ARDUINO: UART1,RX,%s\r\n", gLineBuffer);
  Serial1.printf("+ARDUINO: UART1,ECHO,%s\r\n", gLineBuffer);
  gLineLength = 0;
}

void handleIncomingByte(int value) {
  if (value < 0) {
    return;
  }

  const char ch = static_cast<char>(value);
  if (ch == '\r' || ch == '\n') {
    flushLine();
    return;
  }

  if (gLineLength + 1 < kLineBufferSize) {
    gLineBuffer[gLineLength++] = ch;
  }
}

}  // namespace

void setup() {
  Serial.begin(921600);
  delay(300);

  Serial.printf("+ARDUINO: UART1,READY\r\n");
  Serial.printf("+ARDUINO: UART1,PINS,RX18,TX19\r\n");

  Serial1.begin(115200, SERIAL_8N1);
  delay(100);
  Serial1.printf("+ARDUINO: UART1,ONLINE\r\n");
  gLastTickMs = millis();
}

void loop() {
  while (Serial1.available() > 0) {
    handleIncomingByte(Serial1.read());
  }

  const unsigned long now = millis();
  if (now - gLastTickMs >= kTickIntervalMs) {
    gLastTickMs = now;
    emitTick();
  }

  delay(2);
}
