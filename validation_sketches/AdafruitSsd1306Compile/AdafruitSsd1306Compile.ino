#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(128, 64, &Wire, -1);

static void exerciseAdafruitSsd1306Api() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("AIR780EPM"));
  display.drawPixel(1, 1, SSD1306_WHITE);
  display.drawLine(0, 10, 20, 10, SSD1306_WHITE);
}

void setup() {
  Serial.begin(921600);

  if (millis() == 0xFFFFFFFFUL) {
    exerciseAdafruitSsd1306Api();
  }

  Serial.println(F("+ARDUINO: LIB_COMPAT,ADAFRUIT_SSD1306,READY"));
}

void loop() {
  delay(1000);
}
