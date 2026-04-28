#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

void setup() {
  Serial.begin(921600);

  if (millis() == 0xFFFFFFFFUL) {
    display.setBusClock(400000);
    display.begin();
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 12, "AIR780EPM");
  }

  Serial.println(F("+ARDUINO: LIB_COMPAT,U8G2,READY"));
}

void loop() {
  delay(1000);
}
