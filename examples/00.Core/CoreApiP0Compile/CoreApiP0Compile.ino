#include <Arduino.h>
#include <WCharacter.h>
#include <pgmspace.h>

static const char kPgmText[] PROGMEM = "PGM";

void setup() {
  Serial.begin(921600);

  byte low = lowByte(0x1234);
  byte high = highByte(0x1234);
  word combined = word(high, low);

  uint32_t flags = 0;
  bitSet(flags, 2);
  bitWrite(flags, 5, 1);
  bitClear(flags, 2);
  bitToggle(flags, 1);

  char pgmBuffer[8];
  strcpy_P(pgmBuffer, kPgmText);

  randomSeed(1234);
  long randomValue = random(10, 20);
  long mapped = map(5, 0, 10, 0, 100);
  double angle = radians(180.0);

  boolean ok = true;
  ok = ok && (combined == 0x1234);
  ok = ok && (bitRead(flags, 5) == 1);
  ok = ok && (bitRead(flags, 1) == 1);
  ok = ok && (strcmp_P(pgmBuffer, PSTR("PGM")) == 0);
  ok = ok && isAlphaNumeric('A');
  ok = ok && isDigit('7');
  ok = ok && (toLowerCase('Z') == 'z');
  ok = ok && (constrain(12, 0, 10) == 10);
  ok = ok && (min(3, 7) == 3);
  ok = ok && (max(3, 7) == 7);
  ok = ok && (sq(4) == 16);
  ok = ok && (mapped == 50);
  ok = ok && (randomValue >= 10 && randomValue < 20);
  ok = ok && (angle > 3.13 && angle < 3.15);

  Serial.print(F("+ARDUINO: CORE_API_P0,"));
  Serial.print(ok ? F("PASS") : F("FAIL"));
  Serial.print(F(",FLAGS="));
  Serial.print(flags, HEX);
  Serial.print(F(",RAND="));
  Serial.println(randomValue);
}

void loop() {
  delay(1000);
}
